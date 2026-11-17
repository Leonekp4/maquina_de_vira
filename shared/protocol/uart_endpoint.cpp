#include "uart_link.h"

namespace vira::link {
Endpoint::Endpoint(std::uint32_t boot) : boot_(boot ? boot : 1) {
    Parameters p; p.continuous = true; p.target = 0;
    machine_.configure(p);
    machine_.setInputs({false, false, false});
}
bool Endpoint::configure(const Parameters& parameters) {
    if (!settingsValid(parameters)) return false;
    auto p = parameters; p.continuous = true; p.target = 0;
    if (!machine_.configure(p)) return false;
    ++settingsRevision_;
    return true;
}
void Endpoint::tick(std::uint32_t now) {
    if (owner_ && now - lastSeen_ >= TimeoutMs) {
        owner_ = 0; lastSequence_ = 0;
        machine_.setInputs({false, false, false});
    }
}
Frame Endpoint::reply(const Frame& request, Result result, std::uint32_t now) const {
    Frame frame;
    frame.kind = Kind::Status; frame.command = request.command; frame.result = result;
    frame.clientSession = request.clientSession; frame.sequence = request.sequence;
    frame.controllerBoot = boot_; frame.uptimeMs = now;
    frame.lengthUm = machine_.parameters().lengthUm;
    frame.advanceMs = machine_.parameters().advanceMs; frame.returnMs = machine_.parameters().returnMs;
    frame.produced = machine_.produced(); frame.state = static_cast<std::uint8_t>(machine_.state());
    frame.fault = static_cast<std::uint8_t>(machine_.fault());
    // HardwareReady, DoorClosed e SafetyOk permanecem falsos: ainda nao aferidos.
    return frame;
}
bool Endpoint::receive(const Frame& request, std::uint32_t now, Frame& response) {
    tick(now);
    if (request.kind != Kind::Request || !request.clientSession || !request.sequence ||
        request.result != Result::None) return false;
    if (!owner_ || request.clientSession != owner_) {
        if (request.command != Command::Heartbeat || owner_) {
            response = reply(request, Result::Stale, now); return true;
        }
        owner_ = request.clientSession; lastSequence_ = 0;
    }
    if (request.command != Command::Heartbeat && request.controllerBoot != boot_) {
        response = reply(request, Result::Stale, now); return true;
    }
    const auto bytes = encode(request);
    if (request.sequence == lastSequence_) {
        const bool duplicate = bytes == lastRequest_;
        if (duplicate) lastSeen_ = now;
        response = reply(request, duplicate ? lastResult_ : Result::Stale, now);
        return true;
    }
    if (lastSequence_ && request.sequence < lastSequence_) {
        response = reply(request, Result::Stale, now); return true;
    }
    Result result = Result::Rejected;
    switch (request.command) {
    case Command::Heartbeat: result = Result::Accepted; break;
    case Command::Configure: {
        auto p = machine_.parameters();
        p.lengthUm = request.lengthUm; p.advanceMs = request.advanceMs; p.returnMs = request.returnMs;
        result = configure(p) ? Result::Accepted : Result::Rejected;
        break;
    }
    case Command::Stop: machine_.requestStop(); result = Result::Accepted; break;
    case Command::ResetCount: result = machine_.resetProduced() ? Result::Accepted : Result::Rejected; break;
    case Command::Manual:
        result = request.manual == Manual::None ? Result::Accepted : Result::NotConfigured; break;
    case Command::Start:
    case Command::ResetFault:
    case Command::ConfirmRecovery: result = Result::NotConfigured; break;
    }
    lastSequence_ = request.sequence; lastSeen_ = now; lastRequest_ = bytes; lastResult_ = result;
    response = reply(request, result, now);
    return true;
}
}
