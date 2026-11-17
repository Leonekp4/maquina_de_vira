#include <Arduino.h>
#include <Preferences.h>
#include "uart_link.h"
#include "uart_pins.h"

HardwareSerial ihmSerial(2);
vira::link::Endpoint endpoint(esp_random());
vira::link::StreamDecoder decoder;
Preferences preferences;
bool storageReady = false;
std::uint32_t savedRevision = 0;
void reportStatus() {
    Serial.printf("CONTROLE_UART_READY BAUD=%lu TX=%d RX=%d LINK=%s HARDWARE=PENDENTE SEM_SIMULACAO\n",
                  static_cast<unsigned long>(vira::link::BaudRate), vira::link::ControlTx,
                  vira::link::ControlRx, endpoint.connected() ? "ONLINE" : "OFFLINE");
    const auto& p = endpoint.parameters();
    Serial.printf("COMPRIMENTO_UM=%lu AVANCO_MS=%lu RETORNO_MS=%lu\n",
                  static_cast<unsigned long>(p.lengthUm), static_cast<unsigned long>(p.advanceMs),
                  static_cast<unsigned long>(p.returnMs));
}
void saveSettings() {
    if (!storageReady || savedRevision == endpoint.settingsRevision()) return;
    vira::link::Frame frame;
    const auto& p = endpoint.parameters();
    frame.lengthUm = p.lengthUm; frame.advanceMs = p.advanceMs; frame.returnMs = p.returnMs;
    const auto packet = vira::link::encode(frame);
    if (preferences.putBytes("settings", packet.data(), packet.size()) == packet.size())
        savedRevision = endpoint.settingsRevision();
    else Serial.println("ERRO: parametros confirmados em RAM, falha ao salvar NVS");
}
void setup() {
    Serial.begin(115200);
    storageReady = preferences.begin("vira-control", false);
    if (storageReady && preferences.getBytesLength("settings") == vira::link::PacketSize) {
        vira::link::Packet packet;
        vira::link::Frame frame;
        if (preferences.getBytes("settings", packet.data(), packet.size()) == packet.size() &&
            vira::link::decode(packet.data(), packet.size(), frame)) {
            auto p = endpoint.parameters();
            p.lengthUm = frame.lengthUm; p.advanceMs = frame.advanceMs; p.returnMs = frame.returnMs;
            endpoint.configure(p);
        }
    }
    savedRevision = endpoint.settingsRevision();
    ihmSerial.begin(vira::link::BaudRate, SERIAL_8N1, vira::link::ControlRx, vira::link::ControlTx);
    // Nenhum GPIO de atuador ou permissivo é inicializado nesta etapa.
    reportStatus();
}
void loop() {
    const auto now = millis();
    endpoint.tick(now);
    while (ihmSerial.available()) {
        vira::link::Frame request, response;
        if (decoder.push(static_cast<std::uint8_t>(ihmSerial.read()), request) &&
            endpoint.receive(request, now, response)) {
            const auto packet = vira::link::encode(response);
            ihmSerial.write(packet.data(), packet.size());
            if (request.command != vira::link::Command::Heartbeat)
                Serial.printf("UART COMANDO=%u RESULTADO=%u SEQ=%lu\n", static_cast<unsigned>(request.command),
                              static_cast<unsigned>(response.result), static_cast<unsigned long>(request.sequence));
        }
    }
    saveSettings();
    if (Serial.available() && Serial.read() == '?') reportStatus();
    delay(1);
}
