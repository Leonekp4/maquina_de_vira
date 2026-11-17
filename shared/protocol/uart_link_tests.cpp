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
    std::cout << "OK: sincronizacao, confirmacao, duplicatas, prioridade, timeout e reboot\n";
}
