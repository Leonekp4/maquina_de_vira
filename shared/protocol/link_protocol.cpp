#include "link_protocol.h"
#include <cstring>

namespace vira::link {
namespace {
void put32(Packet& data, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) data[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
std::uint32_t get32(const std::uint8_t* data, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(data[offset + i]) << (8 * i);
    return value;
}
}
std::uint16_t crc16(const std::uint8_t* data, std::size_t size) {
    std::uint16_t crc = 0xffff;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc & 1) ? static_cast<std::uint16_t>((crc >> 1) ^ 0xa001) : crc >> 1;
    }
    return crc;
}
Packet encode(const Frame& f) {
    Packet data{};
    data[0] = 'V'; data[1] = 'I'; data[2] = 'R'; data[3] = 'A';
    data[4] = ProtocolVersion;
    data[5] = static_cast<std::uint8_t>(f.kind);
    data[6] = static_cast<std::uint8_t>(f.command);
    data[7] = static_cast<std::uint8_t>(f.result);
    put32(data, 8, f.clientSession); put32(data, 12, f.sequence);
    put32(data, 16, f.controllerBoot); put32(data, 20, f.uptimeMs);
    put32(data, 24, f.lengthUm); put32(data, 28, f.advanceMs);
    put32(data, 32, f.returnMs); put32(data, 36, f.produced);
    data[40] = f.state; data[41] = f.fault;
    data[42] = static_cast<std::uint8_t>(f.manual); data[43] = f.flags;
    const auto crc = crc16(data.data(), PacketSize - 2);
    data[62] = static_cast<std::uint8_t>(crc);
    data[63] = static_cast<std::uint8_t>(crc >> 8);
    return data;
}
bool decode(const std::uint8_t* data, std::size_t size, Frame& frame) {
    if (!data || size != PacketSize || data[0] != 'V' || data[1] != 'I' ||
        data[2] != 'R' || data[3] != 'A' || data[4] != ProtocolVersion) return false;
    const auto crc = static_cast<std::uint16_t>(data[62] | (static_cast<std::uint16_t>(data[63]) << 8));
    if (crc16(data, PacketSize - 2) != crc || data[5] < 1 || data[5] > 2 ||
        data[6] > static_cast<std::uint8_t>(Command::Manual) ||
        data[7] > static_cast<std::uint8_t>(Result::Stale) || data[40] > 7 || data[41] > 3 ||
        data[42] > static_cast<std::uint8_t>(Manual::ValveReturn) || (data[43] & 0xc0)) return false;
    for (std::size_t i = 44; i < PacketSize - 2; ++i) if (data[i] != 0) return false;
    Frame result;
    result.kind = static_cast<Kind>(data[5]); result.command = static_cast<Command>(data[6]);
    result.result = static_cast<Result>(data[7]);
    result.clientSession = get32(data, 8); result.sequence = get32(data, 12);
    result.controllerBoot = get32(data, 16); result.uptimeMs = get32(data, 20);
    result.lengthUm = get32(data, 24); result.advanceMs = get32(data, 28);
    result.returnMs = get32(data, 32); result.produced = get32(data, 36);
    result.state = data[40]; result.fault = data[41];
    result.manual = static_cast<Manual>(data[42]); result.flags = data[43];
    frame = result;
    return true;
}
bool StreamDecoder::push(std::uint8_t byte, Frame& frame) {
    buffer_[used_++] = byte;
    if (used_ < PacketSize) return false;
    if (decode(buffer_.data(), buffer_.size(), frame)) { used_ = 0; return true; }
    std::memmove(buffer_.data(), buffer_.data() + 1, PacketSize - 1);
    used_ = PacketSize - 1;
    return false;
}
}
