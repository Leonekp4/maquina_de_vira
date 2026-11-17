#include "ui_values.h"
#include <cstdlib>
#include <iostream>
#include <cstring>
void check(bool ok) { if (!ok) { std::cerr << "Conversao de entrada falhou\n"; std::exit(1); } }
int main() {
    vira::Parameters p;
    using ihm::Field;
    check(ihm::applyValue(p, Field::Length, "12.54") && p.lengthUm == 125400);
    check(ihm::applyValue(p, Field::Length, "0.01") && p.lengthUm == 100);
    check(ihm::applyValue(p, Field::Length, "100000") && p.lengthUm == 1000000000);
    check(ihm::applyValue(p, Field::Length, "10") && p.lengthUm == 100000);
    for (auto um : {100u, 1000u, 10000u, 125400u, 1000000000u}) {
        char displayed[24];
        ihm::formatLengthCm(displayed, sizeof(displayed), um);
        check(ihm::applyValue(p, Field::Length, displayed) && p.lengthUm == um);
    }
    char rounded[24];
    ihm::formatLengthCm(rounded, sizeof(rounded), 99999);
    check(std::strcmp(rounded, "10.00") == 0);
    ihm::formatLengthCm(rounded, sizeof(rounded), 125375);
    check(std::strcmp(rounded, "12.54") == 0);
    for (const auto* text : {"", "0", "-1", ".5", "1.", "1.001", "0.001", "1.2.3", "1a", "100001", "99999999999999999999"})
        check(!ihm::applyValue(p, Field::Length, text));
    check(ihm::applyValue(p, Field::Quantity, "12") && p.target == 12);
    check(!ihm::applyValue(p, Field::Quantity, "1.5"));
    check(!ihm::applyValue(p, Field::Quantity, "1000001"));
    check(ihm::applyValue(p, Field::Advance, "60000") && p.advanceMs == 60000);
    check(!ihm::applyValue(p, Field::Return, "60001"));
    vira::Controller c;
    c.configure({}); c.setInputs({true, true, true}); c.start();
    auto edited = c.parameters();
    check(ihm::applyValue(edited, Field::Length, "15.02") && c.configure(edited));
    check(c.activeLengthUm() == 100000);
    c.tick(1000); c.tick(500); c.tick(500);
    check(c.activeLengthUm() == 150200 && c.produced() == 1);
    std::cout << "OK: entradas da IHM e aplicacao na proxima peca\n";
}
