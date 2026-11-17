#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace vira::link {
// Formato independente de transporte. Nunca enviar structs C++ diretamente.
constexpr std::size_t PacketSize = 64;
constexpr std::uint8_t ProtocolVersion = 1;
enum class Kind : std::uint8_t { Request = 1, Status = 2 };
enum class Command : std::uint8_t {
    Heartbeat, Configure, Start, Stop, ResetCount, ResetFault, ConfirmRecovery, Manual
};
enum class Result : std::uint8_t { None, Accepted, Rejected, NotConfigured, Stale };
enum class Manual : std::uint8_t { None, MotorForward, MotorReverse, ValveAdvance, ValveReturn };
enum Flags : std::uint8_t { HardwareReady = 1, DoorClosed = 2, SafetyOk = 4, StopPending = 8 };
struct Frame {
    Kind kind = Kind::Request;
    Command command = Command::Heartbeat;
    Result result = Result::None;
    std::uint32_t clientSession = 0;
    std::uint32_t sequence = 0;
    std::uint32_t controllerBoot = 0;
    std::uint32_t uptimeMs = 0;
    std::uint32_t lengthUm = 100000;
    std::uint32_t advanceMs = 500;
    std::uint32_t returnMs = 500;
    std::uint32_t produced = 0;
    std::uint8_t state = 0;
    std::uint8_t fault = 0;
    Manual manual = Manual::None;
    std::uint8_t flags = 0;
};
using Packet = std::array<std::uint8_t, PacketSize>;
std::uint16_t crc16(const std::uint8_t* data, std::size_t size);
Packet encode(const Frame& frame);
bool decode(const std::uint8_t* data, std::size_t size, Frame& frame);
class StreamDecoder {
public:
    bool push(std::uint8_t byte, Frame& frame);
private:
    Packet buffer_{};
    std::size_t used_ = 0;
};
}
