#include "controller.h"
#include <limits>

namespace vira {
bool Controller::valid(const Parameters& p) {
    return p.lengthUm > 0 && (p.continuous || p.target > 0) && p.feedUmPerSecond > 0 &&
           p.advanceMs > 0 && p.returnMs > 0;
}
bool Controller::permitted() const {
    return inputs_.doorClosed && inputs_.safetyOk && inputs_.connected;
}
bool Controller::moving() const {
    return state_ == State::Feeding || state_ == State::Cutting || state_ == State::Returning;
}
void Controller::fail(Fault fault) {
    fault_ = fault;
    state_ = State::Fault;
    remainingMs_ = 0;
}
void Controller::setInputs(Inputs inputs) {
    inputs_ = inputs;
    if (!moving()) return;
    if (!inputs.doorClosed) fail(Fault::DoorOpen);
    else if (!inputs.safetyOk) fail(Fault::SafetyUnavailable);
    else if (!inputs.connected) fail(Fault::CommunicationLost);
}
bool Controller::configure(const Parameters& p) {
    if (!valid(p)) return false;
    // No lote em andamento, quantidade significa total desejado.
    if (!p.continuous && state_ != State::Idle && state_ != State::Complete &&
        (p.target < produced_ || (moving() && p.target == produced_))) return false;
    pending_ = p;
    configured_ = true;
    if (state_ == State::Idle || state_ == State::Complete) {
        produced_ = 0;
        state_ = State::Ready;
    }
    return true;
}
void Controller::beginPiece() {
    active_ = pending_;
    remainingMs_ = (static_cast<std::uint64_t>(active_.lengthUm) * 1000 +
                    active_.feedUmPerSecond - 1) / active_.feedUmPerSecond;
    state_ = State::Feeding;
}
bool Controller::start() {
    if (state_ != State::Ready || !configured_ || !permitted() ||
        (!pending_.continuous && produced_ >= pending_.target)) return false;
    stopRequested_ = false;
    beginPiece();
    return true;
}
void Controller::requestStop() {
    if (moving()) stopRequested_ = true;
}
bool Controller::canResetProduced() const {
    return configured_ && (state_ == State::Ready || state_ == State::Complete);
}
bool Controller::resetProduced() {
    if (!canResetProduced()) return false;
    produced_ = 0;
    stopRequested_ = false;
    remainingMs_ = 0;
    state_ = State::Ready;
    return true;
}
bool Controller::resetFault() {
    if (state_ != State::Fault || !permitted()) return false;
    fault_ = Fault::None;
    state_ = State::Recovery;
    return true;
}
bool Controller::confirmRecovery() {
    if (state_ != State::Recovery || !permitted()) return false;
    state_ = !pending_.continuous && produced_ >= pending_.target ? State::Complete : State::Ready;
    stopRequested_ = false;
    return true;
}
void Controller::tick(std::uint32_t elapsedMs) {
    // Uma transição por chamada: etapas curtas continuam observáveis na simulação.
    if (!moving()) return;
    if (elapsedMs < remainingMs_) {
        remainingMs_ -= elapsedMs;
        return;
    }
    if (state_ == State::Feeding) {
        state_ = State::Cutting;
        remainingMs_ = active_.advanceMs;
    } else if (state_ == State::Cutting) {
        state_ = State::Returning;
        remainingMs_ = active_.returnMs;
    } else {
        // Saturar a contagem em vez de voltar a zero por overflow.
        if (produced_ < std::numeric_limits<std::uint32_t>::max()) ++produced_;
        if (!pending_.continuous && produced_ >= pending_.target) state_ = State::Complete;
        else if (stopRequested_) state_ = State::Ready;
        else beginPiece();
    }
}
Outputs Controller::outputs() const {
    return {state_ == State::Feeding, state_ == State::Cutting, state_ == State::Returning};
}
Checkpoint Controller::checkpoint() const {
    return {pending_, produced_, moving() || state_ == State::Fault || state_ == State::Recovery};
}
bool Controller::restore(const Checkpoint& saved) {
    if ((state_ != State::Idle && state_ != State::Ready && state_ != State::Complete) ||
        !valid(saved.parameters) ||
        (!saved.parameters.continuous && saved.produced > saved.parameters.target)) return false;
    pending_ = saved.parameters;
    produced_ = saved.produced;
    configured_ = true;
    stopRequested_ = false;
    fault_ = Fault::None;
    state_ = State::Recovery;
    return true;
}
const char* name(State s) {
    switch (s) {
    case State::Idle: return "SEM_LOTE";
    case State::Ready: return "PRONTO";
    case State::Feeding: return "ALIMENTANDO";
    case State::Cutting: return "CORTANDO_POR_TEMPO";
    case State::Returning: return "RETORNANDO_POR_TEMPO";
    case State::Complete: return "LOTE_CONCLUIDO";
    case State::Fault: return "FALHA";
    case State::Recovery: return "AGUARDANDO_CONFIRMACAO";
    }
    return "DESCONHECIDO";
}
const char* name(Fault f) {
    switch (f) {
    case Fault::None: return "NENHUMA";
    case Fault::DoorOpen: return "PORTA_ABERTA";
    case Fault::SafetyUnavailable: return "SEGURANCA_INDISPONIVEL";
    case Fault::CommunicationLost: return "SEM_COMUNICACAO";
    }
    return "DESCONHECIDA";
}
}
