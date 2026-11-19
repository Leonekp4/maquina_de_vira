#pragma once
#include <Arduino.h>
#include "link_protocol.h"
#include "soc/gpio_struct.h"

namespace vira::relays {
constexpr std::uint8_t AdvancePin = 32;
constexpr std::uint8_t ReturnPin = 33;
// Logica invertida por solicitacao do usuario: HIGH em repouso, LOW no comando.
constexpr std::uint8_t InactiveLevel = HIGH;
constexpr std::uint8_t ActiveLevel = LOW;

inline const char* levelName(std::uint8_t level) { return level == HIGH ? "HIGH" : "LOW"; }
inline std::uint8_t outputLatch(std::uint8_t pin) {
    // Le o registrador de saida dos GPIO32/33; nao mede tensao nem contato do rele.
    return (GPIO.out1.val >> (pin - 32)) & 1u;
}

inline void initializeInactive() {
    // Preparar o latch antes de habilitar as saídas evita um pulso ativo
    // causado pela própria troca de INPUT para OUTPUT no início do aplicativo.
    digitalWrite(AdvancePin, InactiveLevel);
    digitalWrite(ReturnPin, InactiveLevel);
    pinMode(AdvancePin, OUTPUT);
    pinMode(ReturnPin, OUTPUT);
    digitalWrite(AdvancePin, InactiveLevel);
    digitalWrite(ReturnPin, InactiveLevel);
}
inline void apply(vira::link::Manual command) {
    static auto applied = vira::link::Manual::None;
    if (command == applied) return;
    digitalWrite(AdvancePin, InactiveLevel);
    digitalWrite(ReturnPin, InactiveLevel);
    // O endpoint intercala 20 ms sem comando ao trocar de direcao.
    if (command == vira::link::Manual::ValveAdvance) digitalWrite(AdvancePin, ActiveLevel);
    else if (command == vira::link::Manual::ValveReturn) digitalWrite(ReturnPin, ActiveLevel);
    applied = command;
}
}
