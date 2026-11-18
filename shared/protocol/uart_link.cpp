#include "uart_link.h"

namespace vira::link {
bool settingsValid(const Parameters& p) {
    return p.lengthUm >= 100 && p.lengthUm <= 1000000000 && p.lengthUm % 100 == 0 &&
           p.advanceMs >= 1 && p.advanceMs <= 60000 && p.returnMs >= 1 && p.returnMs <= 60000;
}
void Client::begin(std::uint32_t session, std::uint32_t now) {
    *this = Client();
    session_ = session ? session : 1;
    now_ = now;
    parameters_.continuous = true; parameters_.target = 0;
    enqueue(Command::Heartbeat);
}
void Client::complete(Result result) {
    if (request_.command != Command::Heartbeat) {
        eventCommand_ = request_.command; eventResult_ = result; hasEvent_ = true;
        ++revision_;
    }
    pending_ = false;
}
void Client::tick(std::uint32_t now) {
    now_ = now;
    if (connected_ && now - receivedAt_ >= TimeoutMs) {
        connected_ = false; ++revision_;
        if (pending_) complete(Result::Stale);
    }
    if (pending_ && now - queuedAt_ >= TimeoutMs) complete(Result::Stale);
}
bool Client::enqueue(Command command, bool priority) {
    if (command != Command::Heartbeat && !connected_) return false;
    if (busy() && !priority) return false;
    request_ = Frame();
    request_.command = command;
    request_.clientSession = session_;
    request_.sequence = ++sequence_;
    if (!request_.sequence) request_.sequence = ++sequence_;
    request_.controllerBoot = snapshot_.controllerBoot;
    queuedAt_ = now_; pending_ = true; sent_ = false;
    return true;
}
bool Client::outgoing(Frame& frame) {
    if (!pending_ && now_ - sentAt_ >= PollMs) enqueue(Command::Heartbeat);
    if (!pending_ || (sent_ && now_ - sentAt_ < PollMs)) return false;
    frame = request_; sent_ = true; sentAt_ = now_;
    return true;
}
bool Client::receive(const Frame& frame) {
    if (!pending_ || !sent_ || frame.kind != Kind::Status ||
        frame.clientSession != session_ || frame.sequence != request_.sequence ||
        frame.command != request_.command || !frame.controllerBoot || frame.result == Result::None) return false;
    if (frame.result == Result::Stale) {
        connected_ = false; complete(Result::Stale); return true;
    }
    if (request_.command != Command::Heartbeat && frame.controllerBoot != request_.controllerBoot) {
        connected_ = false; complete(Result::Stale);
        return false;
    }
    Parameters p;
    p.lengthUm = frame.lengthUm; p.advanceMs = frame.advanceMs; p.returnMs = frame.returnMs;
    p.continuous = true; p.target = 0;
    if (!settingsValid(p)) return false;
    const bool reconnecting = !connected_;
    const bool changed = reconnecting || frame.state != snapshot_.state || frame.fault != snapshot_.fault ||
                         frame.flags != snapshot_.flags || frame.produced != snapshot_.produced ||
                         frame.manual != snapshot_.manual || frame.lengthUm != snapshot_.lengthUm ||
                         frame.advanceMs != snapshot_.advanceMs || frame.returnMs != snapshot_.returnMs;
    snapshot_ = frame; parameters_ = p; receivedAt_ = now_; connected_ = true;
    if (changed) ++revision_;
    complete(frame.result);
    // Na reconexao somente cancelar um manual expirado; nunca retomar movimento.
    if (reconnecting && (frame.flags & ManualExpired)) manualCommand(Manual::None);
    return true;
}
bool Client::configure(const Parameters& p) {
    if (!settingsValid(p) || !enqueue(Command::Configure)) return false;
    request_.lengthUm = p.lengthUm; request_.advanceMs = p.advanceMs; request_.returnMs = p.returnMs;
    return true;
}
bool Client::start() { return enqueue(Command::Start); }
bool Client::requestStop() { return enqueue(Command::Stop, true); }
bool Client::canResetProduced() const {
    return connected_ && !busy() && (state() == State::Ready || state() == State::Complete);
}
bool Client::resetProduced() { return canResetProduced() && enqueue(Command::ResetCount); }
bool Client::resetFault() { return enqueue(Command::ResetFault); }
bool Client::confirmRecovery() { return enqueue(Command::ConfirmRecovery); }
bool Client::manualCommand(Manual manual) {
    if (!enqueue(Command::Manual, true)) return false;
    request_.manual = manual;
    return true;
}
bool Client::event(Command& command, Result& result) {
    if (!hasEvent_) return false;
    command = eventCommand_; result = eventResult_; hasEvent_ = false;
    return true;
}
}
