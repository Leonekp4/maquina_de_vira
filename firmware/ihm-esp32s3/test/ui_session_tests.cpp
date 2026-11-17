#include "ui_session.h"
#include <cstdlib>
#include <iostream>
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FALHOU: " << message << '\n'; std::exit(1); }
}
int main() {
    using namespace ihm;
    Session ui;
    check(!ui.edit(Field::Advance), "tempos fora da tela principal");
    check(!ui.submitPassword("1234"), "senha so na pagina de acesso");
    check(!ui.pressManual(Manual::ValveAdvance, vira::State::Ready), "valvulas protegidas");
    check(ui.pressManual(Manual::MotorForward, vira::State::Ready), "motor na principal");
    ui.release(); check(ui.manual() == Manual::None, "soltar interrompe manual");
    check(!ui.pressManual(Manual::MotorReverse, vira::State::Feeding), "bloquear durante alimentacao");
    ui.openSettings();
    for (const auto* password : {"", "123", "12345", "0000", "1234 "})
        check(!ui.submitPassword(password) && !ui.unlocked(), "senha incorreta nao libera");
    check(!ui.edit(Field::Return), "nao editar pela pagina de senha");
    check(ui.submitPassword("1234"), "senha correta libera configuracao");
    check(ui.pressManual(Manual::ValveAdvance, vira::State::Ready), "avanco de valvula autorizado");
    check(ui.pressManual(Manual::ValveReturn, vira::State::Ready) &&
          ui.manual() == Manual::ValveReturn, "somente um comando manual por vez");
    check(ui.edit(Field::Return) && ui.manual() == Manual::None, "editor cancela comando");
    check(!ui.pressManual(Manual::ValveAdvance, vira::State::Ready), "manual indisponivel no editor");
    ui.closeEditor();
    check(ui.page() == Page::Configuration && ui.unlocked(), "voltar do editor mantem configuracao");
    for (auto state : {vira::State::Feeding, vira::State::Cutting, vira::State::Returning,
                       vira::State::Fault, vira::State::Recovery, vira::State::Idle})
        check(!ui.pressManual(Manual::ValveAdvance, state), "manual bloqueado em ciclo ou falha");
    check(ui.pressManual(Manual::MotorReverse, vira::State::Complete), "manual no fim do lote");
    ui.sync(vira::State::Fault);
    check(ui.manual() == Manual::None, "falha retira comando manual");
    ui.home();
    check(!ui.unlocked() && !ui.edit(Field::Return), "sair bloqueia novamente");
    ui.openSettings();
    check(ui.page() == Page::Password && !ui.unlocked(), "novo acesso exige nova senha");
    ui.home();
    check(ui.edit(Field::Length), "comprimento continua acessivel");
    ui.closeEditor();
    check(ui.page() == Page::Home, "editor de comprimento volta para principal");
    std::cout << "OK: senha, navegacao e comandos manuais exclusivos\n";
}
