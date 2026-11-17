#include "ui_values.h"
#include <cstdlib>
#include <iostream>
void check(bool ok) { if (!ok) { std::cerr << "Conversao de entrada falhou\n"; std::exit(1); } }
int main() {
    vira::Parameters p;
    using ihm::Field;
    check(ihm::applyValue(p, Field::Length, "125.375") && p.lengthUm == 125375);
    check(ihm::applyValue(p, Field::Length, "0.001") && p.lengthUm == 1);
    check(ihm::applyValue(p, Field::Length, "1000000") && p.lengthUm == 1000000000);
    for (const auto* text : {"", "0", "-1", ".5", "1.", "1.0001", "1.2.3", "1a", "1000001", "99999999999999999999"})
        check(!ihm::applyValue(p, Field::Length, text));
    check(ihm::applyValue(p, Field::Quantity, "12") && p.target == 12);
    check(!ihm::applyValue(p, Field::Quantity, "1.5"));
    check(!ihm::applyValue(p, Field::Quantity, "1000001"));
    check(ihm::applyValue(p, Field::Advance, "60000") && p.advanceMs == 60000);
    check(!ihm::applyValue(p, Field::Return, "60001"));
    vira::Controller c;
    c.configure({}); c.setInputs({true, true, true}); c.start();
    auto edited = c.parameters();
    check(ihm::applyValue(edited, Field::Length, "150.25") && c.configure(edited));
    check(c.activeLengthUm() == 100000);
    c.tick(1000); c.tick(500); c.tick(500);
    check(c.activeLengthUm() == 150250 && c.produced() == 1);
    std::cout << "OK: entradas da IHM e aplicacao na proxima peca\n";
}
