#pragma once
#include <cstdint>

namespace vira {
enum class State { Idle, Ready, Feeding, Cutting, Returning, Complete, Fault, Recovery };
enum class Fault { None, DoorOpen, SafetyUnavailable, CommunicationLost };

struct Parameters {
    // Limites de representação para a simulação; não são limites da máquina real.
    std::uint32_t lengthUm = 100000;
    std::uint32_t target = 3;
    std::uint32_t feedUmPerSecond = 100000;
    std::uint32_t advanceMs = 500;
    std::uint32_t returnMs = 500;
};
struct Inputs {
    bool doorClosed = false;
    bool safetyOk = false;
    bool connected = false;
};
struct Outputs {
    bool feedRequested = false;
    bool advanceRequested = false;
    bool returnRequested = false;
};
struct Checkpoint {
    Parameters parameters;
    std::uint32_t produced = 0;
    bool interruptedPiece = false;
};

class Controller {
public:
    bool configure(const Parameters& parameters);
    void setInputs(Inputs inputs);
    bool start();
    void requestStop();
    bool resetFault();
    void tick(std::uint32_t elapsedMs);
    Checkpoint checkpoint() const;
    bool restore(const Checkpoint& checkpoint);
    // Confirmação externa: a peça interrompida foi resolvida e o conjunto está pronto.
    bool confirmRecovery();
    State state() const { return state_; }
    Fault fault() const { return fault_; }
    Outputs outputs() const;
    std::uint32_t produced() const { return produced_; }
    const Parameters& parameters() const { return pending_; }
    std::uint32_t activeLengthUm() const { return active_.lengthUm; }
    static bool valid(const Parameters& parameters);
private:
    bool permitted() const;
    bool moving() const;
    void beginPiece();
    void fail(Fault fault);
    State state_ = State::Idle;
    Fault fault_ = Fault::None;
    Inputs inputs_;
    Parameters pending_;
    Parameters active_;
    bool configured_ = false;
    bool stopRequested_ = false;
    std::uint32_t produced_ = 0;
    std::uint64_t remainingMs_ = 0;
};
const char* name(State state);
const char* name(Fault fault);
}
