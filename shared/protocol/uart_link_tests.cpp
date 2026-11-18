#include "uart_link.h"
#include <cstdlib>
#include <iostream>
void check(bool condition, const char* reason) {
    if (!condition) { std::cerr << "FALHOU: " << reason << '\n'; std::exit(1); }
}
int main() {
    using namespace vira::link;
    Client client;
    Endpoint server(42);
    client.begin(123, 0);
    Frame request, response;
    check(!client.start(), "partida bloqueada desconectado");
    check(client.outgoing(request) && server.receive(request, 0, response) && client.receive(response), "sincronizar heartbeat");
    check(client.connected() && !client.hardwareReady(), "comunicacao nao libera hardware");
    auto parameters = client.parameters(); parameters.lengthUm = 253400;
    check(client.configure(parameters) && client.outgoing(request), "enviar configuracao");
    check(client.parameters().lengthUm == 100000, "nao aplicar configuracao otimisticamente");
    check(server.receive(request, 1, response), "controle aceita parametro");
    const auto revision = server.settingsRevision();
    client.tick(200);
    Frame retry;
    check(client.outgoing(retry) && encode(retry) == encode(request), "retransmitir mesma solicitacao");
    check(server.receive(retry, 200, response) && server.settingsRevision() == revision, "retransmissao nao repete acao");
    check(client.receive(response) && client.parameters().lengthUm == 253400, "aplicar apenas parametro confirmado");
    Command command; Result result;
    check(client.event(command, result) && command == Command::Configure && result == Result::Accepted, "retornar confirmacao");
    check(client.start() && client.outgoing(request) && server.receive(request, 201, response), "solicitar inicio real");
    check(response.result == Result::NotConfigured && response.produced == 0 && !(response.flags & HardwareReady), "sem simular movimento ou contagem");
    client.receive(response);
    auto altered = request; altered.command = Command::ResetCount;
    check(server.receive(altered, 202, response) && response.result == Result::Stale, "mesmo ID com outra acao e rejeitado");
    check(client.configure(parameters), "parametro pendente");
    check(client.requestStop() && client.outgoing(request) && request.command == Command::Stop, "parada tem prioridade");
    check(server.receive(request, 203, response) && client.receive(response), "confirmar parada");
    client.tick(2000); server.tick(2000);
    check(!client.connected() && !server.connected() && !client.start(), "expirar comunicacao e comandos");
    check(client.outgoing(request) && request.command == Command::Heartbeat, "reconectar sem repetir partida");
    Endpoint rebooted(43);
    check(rebooted.receive(request, 2000, response) && client.receive(response), "reconhecer novo boot por heartbeat");
    check(client.start() && client.outgoing(request), "partida vinculada ao novo boot");
    Endpoint anotherBoot(44);
    check(anotherBoot.receive(request, 2001, response) && response.result == Result::Stale &&
          client.receive(response) && !client.connected(), "reinicio nao executa comando pendente");
    StreamDecoder stream;
    Frame decoded;
    for (int i = 0; i < 17; ++i) stream.push(0xff, decoded);
    auto packet = encode(response); auto corrupted = packet; corrupted[30] ^= 1;
    for (auto byte : corrupted) check(!stream.push(byte, decoded), "nao aceitar pacote corrompido");
    unsigned accepted = 0;
    for (auto byte : packet) accepted += stream.push(byte, decoded);
    check(accepted == 1 && encode(decoded) == packet, "ressincronizar fluxo apos ruido");
    Endpoint relays(100);
    relays.enableValveTest(true);
    Frame msg;
    msg.clientSession = 500; msg.sequence = 1;
    check(relays.receive(msg, 0, response) && (response.flags & ValveTestReady) &&
          !(response.flags & HardwareReady), "teste de reles independente do motor");
    msg.controllerBoot = 100; msg.command = Command::Manual; msg.manual = Manual::ValveAdvance;
    msg.sequence = 2;
    check(relays.receive(msg, 1, response) && response.result == Result::Accepted &&
          relays.activeManual() == Manual::None, "intercalar desligamento antes do avanco");
    relays.tick(20); check(relays.activeManual() == Manual::None, "respeitar intervalo de troca");
    relays.tick(21); check(relays.activeManual() == Manual::ValveAdvance, "acionar avanco apos intervalo");
    check(relays.receive(msg, 300, response), "retransmissao do comando recebido");
    relays.tick(601);
    check(relays.activeManual() == Manual::None, "retransmissao nao renova comando manual");
    msg.sequence = 3;
    check(relays.receive(msg, 602, response) && response.result == Result::Rejected &&
          (response.flags & ManualExpired), "expiracao exige liberar antes de reativar");
    msg.manual = Manual::None; msg.sequence = 4;
    check(relays.receive(msg, 603, response) && response.result == Result::Accepted, "soltar limpa expiracao");
    msg.manual = Manual::ValveAdvance; msg.sequence = 5;
    relays.receive(msg, 604, response); relays.tick(624);
    check(relays.activeManual() == Manual::ValveAdvance, "nova pressao inicia avanco");
    msg.sequence = 6; relays.receive(msg, 900, response); relays.tick(1200);
    check(relays.activeManual() == Manual::ValveAdvance, "renovacao nova mantem rele");
    msg.sequence = 7; msg.manual = Manual::ValveReturn;
    relays.receive(msg, 1201, response);
    check(relays.activeManual() == Manual::None, "trocar direcao desliga o canal anterior");
    relays.tick(1221);
    check(relays.activeManual() == Manual::ValveReturn, "retorno exclusivo apos intervalo");
    msg.sequence = 8; msg.manual = Manual::None; relays.receive(msg, 1222, response);
    check(relays.activeManual() == Manual::None, "soltar para imediatamente");
    msg.sequence = 9; msg.manual = Manual::MotorForward;
    check(relays.receive(msg, 1223, response) && response.result == Result::NotConfigured, "motor continua bloqueado");
    msg.sequence = 10; msg.manual = Manual::ValveReturn;
    relays.receive(msg, 1224, response); relays.tick(1244);
    msg.sequence = 11; msg.command = Command::Stop; relays.receive(msg, 1245, response);
    check(relays.activeManual() == Manual::None, "parar encerra manual");
    msg.sequence = 12; msg.command = Command::Manual; relays.receive(msg, 1250, response);
    relays.tick(1270);
    msg.sequence = 13; msg.command = Command::Heartbeat;
    relays.receive(msg, 1500, response); relays.tick(1850);
    check(relays.activeManual() == Manual::None && relays.connected(), "heartbeat nao mantem rele sem renovacao manual");
    msg.sequence = 14; msg.command = Command::Manual; msg.manual = Manual::None;
    relays.receive(msg, 1851, response);
    msg.sequence = 15; msg.manual = Manual::ValveReturn;
    relays.receive(msg, 1852, response); relays.tick(1872); relays.enableValveTest(false);
    check(relays.activeManual() == Manual::None, "retirar capacidade desliga reles");
    msg.sequence = 16;
    check(relays.receive(msg, 1873, response) && response.result == Result::NotConfigured,
          "capacidade retirada bloqueia novo acionamento");
    std::cout << "OK: link, expiracao, reles exclusivos, soltura e bloqueio do motor\n";
}
