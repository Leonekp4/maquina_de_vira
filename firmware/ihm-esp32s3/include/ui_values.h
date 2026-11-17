#pragma once
#include "controller.h"
#include <cstdint>
#include <cstdio>

namespace ihm {
enum class Field { Length, Quantity, Advance, Return };

inline void formatLengthCm(char* buffer, std::size_t size, std::uint32_t um) {
    const auto hundredths = (static_cast<std::uint64_t>(um) + 50) / 100;
    std::snprintf(buffer, size, "%lu.%02lu", static_cast<unsigned long>(hundredths / 100),
                  static_cast<unsigned long>(hundredths % 100));
}

// Conversão decimal exata: centímetros com até 2 casas para micrômetros.
// Limites apenas para teste da interface, não especificações da máquina.
inline bool applyValue(vira::Parameters& p, Field field, const char* text) {
    if (!text || !*text) return false;
    std::uint64_t value = 0;
    unsigned decimals = 0;
    bool dot = false, digit = false, fractionDigit = false;
    for (const char* at = text; *at; ++at) {
        if (*at == '.') {
            if (field != Field::Length || dot || !digit) return false;
            dot = true;
        } else {
            if (*at < '0' || *at > '9') return false;
            digit = true;
            value = value * 10 + static_cast<unsigned>(*at - '0');
            if (dot) { ++decimals; fractionDigit = true; }
            if (decimals > 2 || value > 1000000000ULL) return false;
        }
    }
    if (!digit || (dot && !fractionDigit)) return false;
    if (field == Field::Length) {
        for (unsigned i = decimals; i < 4; ++i) value *= 10;
        if (!value || value > 1000000000ULL) return false;
        p.lengthUm = static_cast<std::uint32_t>(value);
    } else if (field == Field::Quantity) {
        if (!value || value > 1000000) return false;
        p.target = static_cast<std::uint32_t>(value);
    } else {
        if (!value || value > 60000) return false;
        if (field == Field::Advance) p.advanceMs = static_cast<std::uint32_t>(value);
        else p.returnMs = static_cast<std::uint32_t>(value);
    }
    return true;
}
}
