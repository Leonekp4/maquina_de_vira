#include "link_protocol.h"
#include <cstdlib>
#include <iostream>
void check(bool ok, const char* reason) {
    if (!ok) { std::cerr << "FALHOU: " << reason << '\n'; std::exit(1); }
}
int main() {
    using namespace vira::link;
    const std::uint8_t reference[] = {'1','2','3','4','5','6','7','8','9'};
    check(crc16(reference, sizeof(reference)) == 0x4b37, "vetor CRC16 conhecido");
    Frame sent;
    sent.kind = Kind::Status; sent.command = Command::Configure; sent.result = Result::Accepted;
    sent.clientSession = 0x12345678; sent.sequence = 0xffffffff; sent.controllerBoot = 78;
    sent.uptimeMs = 100000; sent.lengthUm = 125400; sent.advanceMs = 60000; sent.returnMs = 987;
    sent.produced = 0xffffffff; sent.state = 7; sent.fault = 3;
    sent.manual = Manual::MotorReverse; sent.flags = HardwareReady | DoorClosed;
    const auto packet = encode(sent);
    check(packet[8] == 0x78 && packet[9] == 0x56, "ordem dos bytes explicita");
    Frame received;
    check(decode(packet.data(), packet.size(), received), "decodificar mensagem valida");
    check(encode(received) == packet, "preservar todos os campos");
    for (std::size_t i = 0; i < packet.size(); ++i) {
        auto corrupted = packet; corrupted[i] ^= 1;
        check(!decode(corrupted.data(), corrupted.size(), received), "detectar alteracao de qualquer byte");
    }
    check(!decode(packet.data(), packet.size() - 1, received), "rejeitar mensagem truncada");
    check(!decode(nullptr, packet.size(), received), "rejeitar buffer ausente");
    sent.state = 255;
    const auto invalid = encode(sent);
    received.produced = 42;
    check(!decode(invalid.data(), invalid.size(), received) && received.produced == 42,
          "rejeitar estado invalido sem alterar telemetria");
    std::cout << "OK: protocolo, integridade, campos e mensagens truncadas\n";
}
