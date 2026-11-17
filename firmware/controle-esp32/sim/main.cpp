#include "controller.h"
#include <iostream>
#include <sstream>
#include <string>

void show(const vira::Controller& c) {
    const auto out = c.outputs();
    std::cout << vira::name(c.state()) << " produzidas=" << c.produced()
              << '/' << c.parameters().target << " falha=" << vira::name(c.fault())
              << " intencoes_simuladas[alimentar,avancar,retornar]="
              << out.feedRequested << ',' << out.advanceRequested << ',' << out.returnRequested << '\n';
}
int main(int argc, char** argv) {
    vira::Controller c;
    vira::Inputs inputs;
    c.configure({});
    if (argc == 2 && std::string(argv[1]) == "--demo") {
        c.setInputs({true, true, true});
        c.start();
        show(c);
        for (unsigned i = 0; i < 100 && c.state() != vira::State::Complete; ++i) {
            auto before = c.state();
            c.tick(100);
            if (before != c.state()) show(c);
        }
        return c.state() == vira::State::Complete ? 0 : 1;
    }
    std::cout << "SIMULACAO SEM GPIO. Comandos:\n"
                 "inputs <porta 0/1> <safety 0/1> <conexao 0/1>\n"
                 "config <comprimento_um> <total> <velocidade_um_s> <avanco_ms> <retorno_ms>\n"
                 "start | stop | tick <ms> | reset | confirm | status | quit\n";
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string command;
        stream >> command;
        bool ok = true;
        if (command == "quit") break;
        if (command == "inputs") {
            int door, safety, connection;
            ok = bool(stream >> door >> safety >> connection);
            ok = ok && (door == 0 || door == 1) && (safety == 0 || safety == 1) &&
                 (connection == 0 || connection == 1);
            if (ok) { inputs = {door != 0, safety != 0, connection != 0}; c.setInputs(inputs); }
        } else if (command == "config") {
            vira::Parameters p;
            ok = line.find('-') == std::string::npos &&
                 bool(stream >> p.lengthUm >> p.target >> p.feedUmPerSecond >> p.advanceMs >> p.returnMs);
            if (ok) ok = c.configure(p);
        } else if (command == "start") ok = c.start();
        else if (command == "stop") c.requestStop();
        else if (command == "tick") {
            std::uint32_t ms;
            ok = line.find('-') == std::string::npos && bool(stream >> ms);
            if (ok) c.tick(ms);
        } else if (command == "reset") ok = c.resetFault();
        else if (command == "confirm") ok = c.confirmRecovery();
        else if (command != "status") ok = false;
        if (!ok) std::cout << "Comando rejeitado\n";
        show(c);
    }
}
