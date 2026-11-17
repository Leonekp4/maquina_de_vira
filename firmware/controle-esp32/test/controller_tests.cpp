#include "controller.h"
#include <cstdlib>
#include <iostream>
#include <limits>

void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FALHOU: " << message << '\n'; std::exit(1); }
}
vira::Controller ready() {
    vira::Controller c;
    check(c.configure({}), "configurar parametros validos");
    c.setInputs({true, true, true});
    return c;
}
void finishPiece(vira::Controller& c) {
    c.tick(1000);
    check(c.state() == vira::State::Cutting, "alimentacao termina antes do corte");
    auto out = c.outputs();
    check(out.advanceRequested && !out.returnRequested && !out.feedRequested, "somente avanco");
    c.tick(500);
    check(c.state() == vira::State::Returning, "retorno apos tempo de avanco");
    out = c.outputs();
    check(!out.advanceRequested && out.returnRequested && !out.feedRequested, "somente retorno");
    c.tick(499);
    check(c.produced() == 0, "nao contar antes do tempo de retorno");
    c.tick(1);
}
int main() {
    {
        vira::Controller c;
        check(!c.start(), "nao iniciar sem lote");
        c.configure({});
        check(!c.start(), "permissivos inicialmente negados");
        auto p = c.parameters(); p.lengthUm = 0;
        check(!c.configure(p), "rejeitar comprimento zero");
        p = c.parameters(); p.advanceMs = 0;
        check(!c.configure(p), "rejeitar tempo zero");
    }
    {
        auto c = ready();
        c.start(); c.requestStop(); finishPiece(c);
        check(c.state() == vira::State::Ready && c.produced() == 1, "parar apos peca completa");
        c.tick(10000);
        check(c.produced() == 1, "nao continuar sozinho apos parada");
        check(c.start(), "retomar lote parado");
        for (int i = 0; i < 6; ++i) c.tick(1000);
        check(c.state() == vira::State::Complete && c.produced() == 3, "completar lote exato");
        check(!c.start(), "nao repetir lote concluido");
    }
    {
        auto c = ready(); c.start();
        auto p = c.parameters(); p.lengthUm = 200000;
        check(c.configure(p), "aceitar ajuste durante producao");
        check(c.activeLengthUm() == 100000, "preservar comprimento da peca atual");
        finishPiece(c);
        check(c.activeLengthUm() == 200000, "novo comprimento na proxima peca");
        c.tick(1000);
        check(c.state() == vira::State::Feeding, "respeitar novo tempo de alimentacao");
    }
    for (int phase = 0; phase < 3; ++phase) {
        auto c = ready(); c.start();
        for (int i = 0; i < phase; ++i) c.tick(1000);
        c.setInputs({false, true, true});
        check(c.state() == vira::State::Fault, "falhar em todas as fases com porta aberta");
        auto out = c.outputs();
        check(!out.feedRequested && !out.advanceRequested && !out.returnRequested, "retirar intencoes operacionais");
        check(!c.resetFault(), "nao rearmar com porta aberta");
        c.setInputs({true, true, true});
        check(c.state() == vira::State::Fault && !c.start(), "fechar porta nao reinicia");
        check(c.resetFault(), "rearme operacional");
        check(c.state() == vira::State::Recovery && !c.start(), "exigir resolucao da peca interrompida");
        check(c.confirmRecovery() && c.produced() == 0, "nao contar corte interrompido");
        check(c.state() == vira::State::Ready, "confirmacao nao inicia movimento");
    }
    {
        auto c = ready(); c.start(); c.setInputs({true, true, false});
        check(c.fault() == vira::Fault::CommunicationLost, "perda de comunicacao");
        c = ready(); c.start(); c.setInputs({true, false, true});
        check(c.fault() == vira::Fault::SafetyUnavailable, "perda de permissivo");
    }
    {
        auto c = ready(); c.start(); c.requestStop(); finishPiece(c); c.start();
        auto saved = c.checkpoint();
        check(saved.interruptedPiece && saved.produced == 1, "checkpoint preserva contagem confirmada");
        vira::Controller rebooted;
        check(rebooted.restore(saved), "restaurar checkpoint");
        check(!rebooted.confirmRecovery(), "confirmacao depende de permissivos");
        rebooted.setInputs({true, true, true});
        check(!rebooted.start(), "reinicio exige confirmacao");
        check(rebooted.confirmRecovery() && rebooted.produced() == 1, "recuperar contagem");
        check(rebooted.start(), "partida explicita apos recuperacao");
        check(!rebooted.restore(saved), "nao restaurar durante movimento");
        saved.produced = 4;
        vira::Controller invalid;
        check(!invalid.restore(saved), "rejeitar checkpoint inconsistente");
    }
    {
        auto c = ready();
        auto p = c.parameters(); p.lengthUm = std::numeric_limits<std::uint32_t>::max();
        p.feedUmPerSecond = 1;
        c.configure(p); c.start(); c.tick(std::numeric_limits<std::uint32_t>::max());
        check(c.state() == vira::State::Feeding, "calculo de tempo nao transborda 32 bits");
    }
    std::cout << "OK: ciclo, parada, parametros, falhas e recuperacao\n";
}
