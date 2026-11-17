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
    ui.openFaults();
    check(ui.page() == Page::Faults && !ui.unlocked(), "abrir diagnostico de falhas");
    check(!ui.pressManual(Manual::MotorForward, vira::State::Ready), "diagnostico nao aciona manual");
    vira::Controller c;
    vira::Parameters p; p.continuous = true; p.target = 0;
    c.configure(p); c.setInputs({true, true, true});
    check(!ui.start(c), "iniciar exclusivo da principal");
    ui.home();
    check(ui.start(c), "iniciar comeca ciclo");
    check(!ui.start(c), "segundo iniciar nao reinicia peca em andamento");
    c.setInputs({false, true, true});
    check(!ui.start(c) && c.state() == vira::State::Fault, "iniciar nao rearma falhas");
    c.setInputs({true, true, true}); c.resetFault();
    check(!ui.start(c) && c.state() == vira::State::Recovery, "iniciar nao confirma recuperacao");
    c.confirmRecovery();
    check(ui.start(c), "iniciar apos recuperacao explicita");
    c.requestStop(); c.tick(1000); c.tick(500); c.tick(500);
    check(c.produced() == 1, "preparar contagem para confirmar e cancelar");
    check(!ui.resetCount(c) && c.produced() == 1, "nao zerar sem abrir confirmacao");
    check(ui.openCounter() && ui.page() == Page::ResetCount, "tocar contador abre confirmacao");
    check(!ui.start(c), "modal de contagem nao inicia producao");
    ui.home();
    check(c.produced() == 1, "cancelar preserva contagem");
    check(ui.start(c), "retomar para testar bloqueio ao zerar");
    ui.openCounter();
    check(!ui.resetCount(c) && c.produced() == 1, "nao zerar durante producao");
    c.requestStop(); c.tick(1000); c.tick(500); c.tick(500);
    check(c.produced() == 2 && c.state() == vira::State::Ready, "aguardar peca antes de zerar");
    const auto length = c.parameters().lengthUm;
    check(ui.resetCount(c) && c.produced() == 0 && ui.page() == Page::Home, "confirmacao zera e volta a principal");
    check(c.parameters().lengthUm == length && c.state() == vira::State::Ready,
          "zerar preserva parametros e nao inicia ciclo");
    std::cout << "OK: senha, navegacao, comandos manuais e iniciar/falhas\n";
}
