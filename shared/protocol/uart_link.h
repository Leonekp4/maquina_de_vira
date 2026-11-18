#pragma once
#include "link_protocol.h"
#include "controller.h"

namespace vira::link {
constexpr std::uint32_t PollMs = 200;
constexpr std::uint32_t TimeoutMs = 1200;
constexpr std::uint32_t ManualLeaseMs = 600;
constexpr std::uint32_t RelayBreakMs = 20;
bool settingsValid(const Parameters& parameters);

// Cliente puro: nenhuma contagem ou sequencia de movimento e executada na IHM.
class Client {
public:
    void begin(std::uint32_t session, std::uint32_t now);
    void tick(std::uint32_t now);
    bool outgoing(Frame& frame);
    bool receive(const Frame& frame);
    bool configure(const Parameters& parameters);
    bool start();
    bool requestStop();
    bool canResetProduced() const;
    bool resetProduced();
    bool resetFault();
    bool confirmRecovery();
    bool manualCommand(Manual manual);
    bool connected() const { return connected_; }
    bool hardwareReady() const { return connected_ && (snapshot_.flags & HardwareReady); }
    bool valveTestReady() const { return connected_ && (snapshot_.flags & ValveTestReady); }
    Manual activeManual() const { return connected_ ? snapshot_.manual : Manual::None; }
    bool busy() const { return pending_ && request_.command != Command::Heartbeat; }
    State state() const { return static_cast<State>(snapshot_.state); }
    Fault fault() const { return connected_ ? static_cast<Fault>(snapshot_.fault) : Fault::CommunicationLost; }
    std::uint32_t produced() const { return snapshot_.produced; }
    const Parameters& parameters() const { return parameters_; }
    bool event(Command& command, Result& result);
    std::uint32_t revision() const { return revision_; }
private:
    bool enqueue(Command command, bool priority = false);
    void complete(Result result);
    Frame snapshot_, request_;
    Parameters parameters_;
    std::uint32_t session_ = 1, sequence_ = 0, now_ = 0;
    std::uint32_t receivedAt_ = 0, sentAt_ = 0, queuedAt_ = 0, revision_ = 0;
    bool connected_ = false, pending_ = false, sent_ = false, hasEvent_ = false;
    Command eventCommand_ = Command::Heartbeat;
    Result eventResult_ = Result::None;
};

// Endpoint real: motor/ciclo bloqueados; teste dos reles sem cargas habilitavel.
// Nao chama tick() da simulacao.
class Endpoint {
public:
    explicit Endpoint(std::uint32_t boot = 1);
    bool receive(const Frame& request, std::uint32_t now, Frame& response);
    void tick(std::uint32_t now);
    const Parameters& parameters() const { return machine_.parameters(); }
    bool configure(const Parameters& parameters);
    std::uint32_t settingsRevision() const { return settingsRevision_; }
    bool connected() const { return owner_ != 0; }
    void enableValveTest(bool enabled);
    Manual activeManual() const { return manual_; }
private:
    void stopManual(bool expired = false);
    Frame reply(const Frame& request, Result result, std::uint32_t now) const;
    Controller machine_;
    std::uint32_t boot_ = 1, owner_ = 0, lastSequence_ = 0, lastSeen_ = 0;
    std::uint32_t settingsRevision_ = 0;
    Packet lastRequest_{};
    Result lastResult_ = Result::None;
    bool valveTestReady_ = false, manualExpired_ = false;
    Manual manual_ = Manual::None, pendingManual_ = Manual::None;
    std::uint32_t manualRenewedAt_ = 0, manualChangedAt_ = 0;
};
}
