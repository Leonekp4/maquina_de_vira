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
    if ((manual_ != Manual::None || pendingManual_ != Manual::None) &&
        now - manualRenewedAt_ >= ManualLeaseMs) stopManual(true);
    if (pendingManual_ != Manual::None && now - manualChangedAt_ >= RelayBreakMs) {
        manual_ = pendingManual_; pendingManual_ = Manual::None;
    }
    if (owner_ && now - lastSeen_ >= TimeoutMs) {
        stopManual(true);
        owner_ = 0; lastSequence_ = 0;
        machine_.setInputs({false, false, false});
    }
}
void Endpoint::stopManual(bool expired) {
    const bool wasActive = manual_ != Manual::None || pendingManual_ != Manual::None;
    manual_ = pendingManual_ = Manual::None;
    if (expired) manualExpired_ = manualExpired_ || wasActive;
    else manualExpired_ = false;
}
void Endpoint::enableValveTest(bool enabled) {
    stopManual(); valveTestReady_ = enabled;
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
    frame.manual = manual_;
    if (valveTestReady_) frame.flags |= ValveTestReady;
    if (manualExpired_) frame.flags |= ManualExpired;
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
    case Command::Stop: stopManual(); machine_.requestStop(); result = Result::Accepted; break;
    case Command::ResetCount: result = machine_.resetProduced() ? Result::Accepted : Result::Rejected; break;
    case Command::Manual:
        if (request.manual == Manual::None) {
            stopManual(); result = Result::Accepted;
        } else if (request.manual == Manual::MotorForward || request.manual == Manual::MotorReverse || !valveTestReady_) {
            result = Result::NotConfigured;
        } else if (manualExpired_ || (machine_.state() != State::Ready && machine_.state() != State::Complete)) {
            result = Result::Rejected;
        } else {
            if (manual_ != request.manual && pendingManual_ != request.manual) {
                manual_ = Manual::None; pendingManual_ = request.manual; manualChangedAt_ = now;
            }
            manualRenewedAt_ = now;
            result = Result::Accepted;
        }
        break;
    case Command::Start:
    case Command::ResetFault:
    case Command::ConfirmRecovery: result = Result::NotConfigured; break;
    }
    lastSequence_ = request.sequence; lastSeen_ = now; lastRequest_ = bytes; lastResult_ = result;
    response = reply(request, result, now);
    return true;
}
}
