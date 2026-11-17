#include <Arduino.h>
#include <PanelLan.h>
#include <cmath>
#include "controller.h"
#include "ui_values.h"
#include "ui_session.h"
#include "uart_link.h"
#include "uart_pins.h"

// A IHM apresenta somente estados confirmados pelo controlador via UART.
PanelLan display(BOARD_SC01_PLUS);
lgfx::LGFX_Sprite canvas(&display);
HardwareSerial controlSerial(1);
vira::link::Client controller;
vira::link::StreamDecoder decoder;
ihm::Session ui;
bool touchHeld = false;
bool displayReady = false, redrawRequested = false;
std::uint32_t renderedFrames = 0;
String input;
const char* notice = "Aguardando comunicacao com o controle";
const char* editorError = "";
unsigned long lastPaint = 0, lastManualRenew = 0;
std::uint32_t paintedRevision = 0;
vira::State paintedState = vira::State::Idle;
std::uint32_t paintedCount = 0;
constexpr uint16_t BG = 0x0863, CARD = 0x1926, WHITE = 0xffff;
constexpr uint16_t MUTED = 0xa534, BLUE = 0x1c7f, GREEN = 0x05ad, RED = 0xd986;

void label(const String& text, int x, int y, int size = 2, uint16_t color = WHITE) {
    canvas.setTextColor(color);
    canvas.setTextSize(size);
    canvas.drawString(text, x, y);
}
void button(int x, int y, int w, int h, const String& text, uint16_t color) {
    canvas.fillRoundRect(x, y, w, h, 6, color);
    canvas.setTextColor(WHITE);
    canvas.setTextSize(2);
    canvas.drawString(text, x + (w - canvas.textWidth(text)) / 2, y + (h - 16) / 2);
}
String lengthText(std::uint32_t um) {
    char text[24];
    ihm::formatLengthCm(text, sizeof(text), um);
    return text;
}
const char* stateText() {
    if (!controller.connected()) return "SEM COMUNICACAO";
    if (!controller.hardwareReady()) return "HARDWARE NAO CONFIGURADO";
    switch (controller.state()) {
    case vira::State::Idle: return "SEM LOTE";
    case vira::State::Ready: return "PRONTO";
    case vira::State::Feeding: return "ALIMENTANDO";
    case vira::State::Cutting: return "CORTE TEMPORIZADO";
    case vira::State::Returning: return "RETORNO TEMPORIZADO";
    case vira::State::Complete: return "LOTE CONCLUIDO";
    case vira::State::Fault: return "FALHA";
    case vira::State::Recovery: return "CONFIRMAR RECUPERACAO";
    }
    return "ESTADO DESCONHECIDO";
}
void gear() {
    canvas.fillRoundRect(426, 2, 42, 34, 5, CARD);
    constexpr int cx = 447, cy = 19;
    canvas.fillCircle(cx, cy, 11, WHITE);
    for (int i = 0; i < 8; ++i) {
        const float angle = i * 0.78539816f;
        for (int offset = -1; offset <= 1; ++offset)
            canvas.drawLine(cx + std::lround(9 * std::cos(angle)) + offset,
                             cy + std::lround(9 * std::sin(angle)),
                             cx + std::lround(15 * std::cos(angle)) + offset,
                             cy + std::lround(15 * std::sin(angle)), WHITE);
    }
    canvas.fillCircle(cx, cy, 5, CARD);
}
void banner() {
    canvas.fillRoundRect(12, 38, 456, 34, 5,
                          !controller.connected() || controller.state() == vira::State::Fault ? RED : BLUE);
    label(controller.activeManual() == vira::link::Manual::None ? stateText() :
          ihm::manualText(static_cast<ihm::Manual>(controller.activeManual())), 22, 47);
}
void card(int x, int y, const char* title, const String& value) {
    canvas.fillRoundRect(x, y, 222, 56, 6, CARD);
    label(title, x + 10, y + 7, 1, MUTED);
    label(value, x + 10, y + 27);
}
void manualButton(int x, int y, const char* text, ihm::Manual command) {
    const bool allowed = controller.hardwareReady() && ihm::Session::manualAllowed(controller.state());
    button(x, y, 222, 50, text,
           controller.activeManual() == static_cast<vira::link::Manual>(command) ? GREEN : allowed ? BLUE : CARD);
}
void rememberPaint() {
    paintedState = controller.state();
    paintedCount = controller.produced();
    paintedRevision = controller.revision();
}
void drawHome() {
    canvas.fillScreen(BG);
    label("CORTE DE VIRA", 12, 10);
    label(controller.connected() ? "UART ONLINE" : "UART OFFLINE", 322, 16, 1, 0xff40);
    gear(); banner();
    const auto& p = controller.parameters();
    card(12, 82, "COMPRIMENTO (cm)", controller.connected() ? lengthText(p.lengthUm) : "--");
    card(246, 82, "QUANTIDADE DO LOTE", controller.connected() ? String(controller.produced()) : "--");
    manualButton(12, 146, "MOTOR AVANCO", ihm::Manual::MotorForward);
    manualButton(246, 146, "MOTOR RETORNO", ihm::Manual::MotorReverse);
    label(notice, 12, 236, 1, MUTED);
    button(12, 260, 144, 48, "INICIAR", controller.hardwareReady() ? GREEN : CARD);
    button(168, 260, 144, 48, "PARAR", RED);
    button(324, 260, 144, 48, "FALHAS", CARD);
    rememberPaint();
}
void drawConfiguration() {
    canvas.fillScreen(BG);
    label("CONFIGURACAO", 12, 10);
    label(controller.connected() ? "UART ONLINE" : "UART OFFLINE", 382, 16, 1, 0xff40);
    banner();
    manualButton(12, 82, "MOTOR AVANCO", ihm::Manual::MotorForward);
    manualButton(246, 82, "MOTOR RETORNO", ihm::Manual::MotorReverse);
    manualButton(12, 142, "VALVULA AVANCO", ihm::Manual::ValveAdvance);
    manualButton(246, 142, "VALVULA RETORNO", ihm::Manual::ValveReturn);
    card(12, 202, "TEMPO AVANCO VALVULA (ms)", controller.connected() ? String(controller.parameters().advanceMs) : "--");
    card(246, 202, "TEMPO RETORNO VALVULA (ms)", controller.connected() ? String(controller.parameters().returnMs) : "--");
    button(12, 270, 200, 40, "VOLTAR", CARD);
    label("Segure para mover.", 228, 277, 1, MUTED);
    label("Manual so com ciclo parado.", 228, 293, 1, MUTED);
    rememberPaint();
}
bool passwordPage() { return ui.page() == ihm::Page::Password; }
void drawCounter() {
    canvas.fillScreen(BG);
    label("QUANTIDADE DO LOTE", 12, 10);
    banner();
    card(12, 92, "PECAS PRODUZIDAS", controller.connected() ? String(controller.produced()) : "--");
    label("Deseja zerar a contagem?", 24, 174);
    label(controller.canResetProduced() ? "O comprimento e os tempos serao mantidos." :
          "Volte, pare a producao e resolva eventuais falhas.", 24, 204, 1, MUTED);
    button(12, 260, 222, 48, "ZERAR", controller.canResetProduced() ? RED : CARD);
    button(246, 260, 222, 48, "CANCELAR", BLUE);
    rememberPaint();
}
void drawFaults() {
    canvas.fillScreen(BG);
    label("FALHAS", 12, 10);
    banner();
    canvas.fillRoundRect(12, 82, 456, 70, 6, CARD);
    label("DIAGNOSTICO ATUAL", 24, 92, 1, MUTED);
    const char* reason = controller.hardwareReady() ? "SEM FALHAS ATIVAS" : "HARDWARE PENDENTE";
    switch (controller.fault()) {
    case vira::Fault::None: break;
    case vira::Fault::DoorOpen: reason = "PORTA ABERTA"; break;
    case vira::Fault::SafetyUnavailable: reason = "SEGURANCA INDISPONIVEL"; break;
    case vira::Fault::CommunicationLost: reason = "SEM COMUNICACAO"; break;
    }
    label(reason, 24, 118);
    label("Rearme sujeito aos permissivos do controle.", 12, 180, 1, MUTED);
    const auto s = controller.state();
    button(246, 200, 222, 32, s == vira::State::Recovery ? "CONFIRMAR" : "REARMAR",
           s == vira::State::Fault || s == vira::State::Recovery ? GREEN : CARD);
    label(notice, 12, 234, 1, MUTED);
    button(12, 260, 456, 48, "VOLTAR", BLUE);
    rememberPaint();
}
const char* fieldTitle() {
    if (passwordPage()) return "CONFIGURACAO - SENHA";
    switch (ui.field()) {
    case ihm::Field::Length: return "COMPRIMENTO EM cm";
    case ihm::Field::Quantity: return "TOTAL DE PECAS DO LOTE";
    case ihm::Field::Advance: return "TEMPO AVANCO VALVULA (ms)";
    case ihm::Field::Return: return "TEMPO RETORNO VALVULA (ms)";
    }
    return "";
}
void drawEditor() {
    canvas.fillScreen(BG);
    label(fieldTitle(), 12, 10);
    label(passwordPage() ? "Digite a senha para acessar os ajustes" :
          "Alteracao depende da confirmacao do controle", 12, 35, 1, MUTED);
    canvas.fillRoundRect(12, 52, 456, 46, 6, CARD);
    String shown = input;
    if (passwordPage()) {
        shown = "";
        for (unsigned i = 0; i < input.length(); ++i) shown += '*';
    }
    label(shown.length() ? shown : "_", 24, 66);
    label(editorError, 12, 100, 1, 0xff40);
    const char* keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "<"};
    for (int i = 0; i < 12; ++i)
        button(12 + (i % 3) * 112, 110 + (i / 3) * 49, 104, 42,
               i == 9 && passwordPage() ? "LIMPAR" : keys[i], CARD);
    button(352, 110, 116, 56, passwordPage() ? "ENTRAR" : "SALVAR", GREEN);
    button(352, 176, 116, 56, "VOLTAR", RED);
    if (!passwordPage()) {
        label("Faixa de valores", 352, 245, 1, MUTED);
        label(ui.field() == ihm::Field::Length ? "0.01..100000 cm" :
              ui.field() == ihm::Field::Quantity ? "1..1000000 pecas" : "1..60000 ms", 352, 262, 1, MUTED);
    }
}
void drawPage() {
    // Os eventos apenas solicitam atualizacao. Renderizar uma vez no fim do loop.
    redrawRequested = true;
}
void renderPage() {
    if (!displayReady || !redrawRequested) return;
    if (ui.page() == ihm::Page::Home) drawHome();
    else if (ui.page() == ihm::Page::Configuration) drawConfiguration();
    else if (ui.page() == ihm::Page::Faults) drawFaults();
    else if (ui.page() == ihm::Page::ResetCount) drawCounter();
    else drawEditor();
    // Nunca limpar a tela fisica entre elementos. Enviar a imagem ja pronta.
    canvas.pushSprite(0, 0);
    ++renderedFrames;
    redrawRequested = false;
    lastPaint = millis();
}
bool inside(int x, int y, int left, int top, int width, int height) {
    return x >= left && x < left + width && y >= top && y < top + height;
}
void openEditor(ihm::Field field) {
    if (!ui.edit(field)) return;
    input = ""; editorError = ""; drawPage();
}
void handleEditor(int x, int y) {
    if (inside(x, y, 352, 176, 116, 56)) {
        if (passwordPage()) ui.home(); else ui.closeEditor();
        input = ""; editorError = ""; drawPage(); return;
    }
    if (inside(x, y, 352, 110, 116, 56)) {
        if (passwordPage()) {
            const bool ok = ui.submitPassword(input.c_str());
            input = "";
            editorError = ok ? "" : "Senha incorreta";
        } else {
            auto p = controller.parameters();
            if (!ihm::applyValue(p, ui.field(), input.c_str()) || !controller.configure(p)) {
                editorError = "Valor invalido, sem conexao ou comando pendente";
            } else {
                notice = "Aguardando confirmacao dos parametros";
                ui.closeEditor(); input = ""; editorError = "";
            }
        }
        drawPage(); return;
    }
    const char* keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "<"};
    for (int i = 0; i < 12; ++i) {
        if (!inside(x, y, 12 + (i % 3) * 112, 110 + (i / 3) * 49, 104, 42)) continue;
        editorError = "";
        if (i == 11) { if (input.length()) input.remove(input.length() - 1); }
        else if (i == 9 && passwordPage()) input = "";
        else if (input.length() < (passwordPage() ? 4u : 12u)) {
            const int dot = input.indexOf('.');
            const bool fractionalLimit = !passwordPage() && ui.field() == ihm::Field::Length &&
                                         i != 9 && dot >= 0 && input.length() - dot - 1 >= 2;
            if (!fractionalLimit) input += keys[i];
        }
        drawPage(); return;
    }
}
void beginManual(ihm::Manual command) {
    if (!controller.hardwareReady()) {
        notice = controller.connected() ? "Comandos bloqueados: hardware pendente" : "Sem comunicacao com o controle";
    } else if (ui.pressManual(command, controller.state()) &&
               controller.manualCommand(static_cast<vira::link::Manual>(command))) {
        notice = "Solicitacao manual enviada; solte para parar";
        lastManualRenew = millis();
        Serial.printf("MANUAL_UART %s\n", ihm::manualText(command));
    } else notice = "Manual disponivel somente com ciclo parado";
    drawPage();
}
void handleConfiguration(int x, int y) {
    if (inside(x, y, 12, 82, 222, 50)) beginManual(ihm::Manual::MotorForward);
    else if (inside(x, y, 246, 82, 222, 50)) beginManual(ihm::Manual::MotorReverse);
    else if (inside(x, y, 12, 142, 222, 50)) beginManual(ihm::Manual::ValveAdvance);
    else if (inside(x, y, 246, 142, 222, 50)) beginManual(ihm::Manual::ValveReturn);
    else if (inside(x, y, 12, 202, 222, 56)) openEditor(ihm::Field::Advance);
    else if (inside(x, y, 246, 202, 222, 56)) openEditor(ihm::Field::Return);
    else if (inside(x, y, 12, 270, 200, 40)) { ui.home(); drawPage(); }
}
void handleHome(int x, int y) {
    if (inside(x, y, 426, 2, 42, 34)) {
        ui.openSettings(); input = ""; editorError = ""; drawPage();
    } else if (inside(x, y, 12, 82, 222, 56)) openEditor(ihm::Field::Length);
    else if (inside(x, y, 246, 82, 222, 56)) { ui.openCounter(); drawPage(); }
    else if (inside(x, y, 12, 146, 222, 50)) beginManual(ihm::Manual::MotorForward);
    else if (inside(x, y, 246, 146, 222, 50)) beginManual(ihm::Manual::MotorReverse);
    else if (inside(x, y, 12, 260, 144, 48)) {
        if (ui.start(controller)) notice = "Aguardando confirmacao de inicio";
        else notice = "Partida indisponivel. Consulte o estado e as falhas";
        drawPage();
    } else if (inside(x, y, 168, 260, 144, 48)) {
        notice = controller.requestStop() ? "Aguardando confirmacao da parada" : "Sem comunicacao com o controle";
        drawPage();
    } else if (inside(x, y, 324, 260, 144, 48)) {
        ui.openFaults(); drawPage();
    }
}
void handleFaults(int x, int y) {
    if (inside(x, y, 12, 260, 456, 48)) { ui.home(); drawPage(); }
    else if (inside(x, y, 246, 200, 222, 32)) {
        if (controller.state() == vira::State::Fault) {
            if (controller.resetFault()) notice = "Aguardando confirmacao do rearme";
        } else if (controller.state() == vira::State::Recovery) {
            if (controller.confirmRecovery()) notice = "Aguardando confirmacao da recuperacao";
        } else notice = "Sem falha para rearmar";
        drawPage();
    }
}
void handleCounter(int x, int y) {
    if (inside(x, y, 246, 260, 222, 48)) { ui.home(); drawPage(); }
    else if (inside(x, y, 12, 260, 222, 48)) {
        if (ui.resetCount(controller)) {
            notice = "Aguardando confirmacao de zerar";
        }
        drawPage();
    }
}
bool manualTouchInside(int x, int y) {
    const bool forward = ui.manual() == ihm::Manual::MotorForward || ui.manual() == ihm::Manual::ValveAdvance;
    const int left = forward ? 12 : 246;
    const bool valve = ui.manual() == ihm::Manual::ValveAdvance || ui.manual() == ihm::Manual::ValveReturn;
    const int top = ui.page() == ihm::Page::Home ? 146 : valve ? 142 : 82;
    return inside(x, y, left, top, 222, 50);
}
void reportStatus() {
    Serial.println("IHM_UART_READY SC01_PLUS 480x320; UI=8 UART_REMOTO; SEM_SIMULACAO");
    Serial.printf("Tela=%dx%d PSRAM=%u FLASH=%u ESTADO=%s PAGINA=%u MANUAL=%u\n",
                  display.width(), display.height(), ESP.getPsramSize(), ESP.getFlashChipSize(),
                  stateText(), static_cast<unsigned>(ui.page()), static_cast<unsigned>(ui.manual()));
    Serial.printf("RENDER=PSRAM_RGB565 BUFFER=%ux%u FRAMES=%lu\n", canvas.width(), canvas.height(),
                  static_cast<unsigned long>(renderedFrames));
    Serial.printf("COMPRIMENTO_CM=%s\n", lengthText(controller.parameters().lengthUm).c_str());
    Serial.printf("MODO=CONTINUO PECAS=%lu BOTOES=INICIAR,PARAR,FALHAS\n",
                  static_cast<unsigned long>(controller.produced()));
    Serial.printf("UART TX=%d RX=%d BAUD=%lu LINK=%s HARDWARE=%s\n", vira::link::IhmTx, vira::link::IhmRx,
                  static_cast<unsigned long>(vira::link::BaudRate), controller.connected() ? "ONLINE" : "OFFLINE",
                  controller.hardwareReady() ? "PRONTO" : "PENDENTE");
}
void setup() {
    Serial.begin(115200);
    display.begin(); display.setRotation(1); display.setBrightness(180);
    canvas.setColorDepth(16);
    canvas.setPsram(true);
    if (!canvas.createSprite(display.width(), display.height())) {
        Serial.println("FRAMEBUFFER_FAILED: memoria insuficiente para a IHM");
        display.fillScreen(RED);
        display.setTextColor(WHITE, RED);
        display.drawString("Falha de memoria da tela. Reinicie a IHM.", 12, 40);
        return;
    }
    displayReady = true;
    canvas.setFont(&fonts::Font0); canvas.setTextDatum(lgfx::textdatum_t::top_left);
    controlSerial.begin(vira::link::BaudRate, SERIAL_8N1, vira::link::IhmRx, vira::link::IhmTx);
    controller.begin(esp_random(), millis());
    drawPage(); renderPage(); reportStatus();
}
void loop() {
    if (!displayReady) { delay(100); return; }
    const unsigned long now = millis();
    controller.tick(now);
    while (controlSerial.available()) {
        vira::link::Frame frame;
        if (decoder.push(static_cast<std::uint8_t>(controlSerial.read()), frame)) controller.receive(frame);
    }
    vira::link::Command completed;
    vira::link::Result result;
    if (controller.event(completed, result)) {
        if (result == vira::link::Result::Accepted) {
            switch (completed) {
            case vira::link::Command::Configure: notice = "Parametros confirmados pelo controle"; break;
            case vira::link::Command::Start: notice = "Inicio confirmado pelo controle"; break;
            case vira::link::Command::Stop: notice = "Parada confirmada pelo controle"; break;
            case vira::link::Command::ResetCount: notice = "Contagem zerada pelo controle"; break;
            case vira::link::Command::ResetFault: notice = "Rearme confirmado. Confirme recuperacao"; break;
            case vira::link::Command::ConfirmRecovery: notice = "Recuperacao confirmada. Toque INICIAR"; break;
            case vira::link::Command::Manual: notice = "Comando manual confirmado pelo controle"; break;
            default: break;
            }
        } else {
            notice = result == vira::link::Result::NotConfigured ? "Hardware pendente: motor, reles e entradas" :
                     result == vira::link::Result::Stale ? "Sem confirmacao. Verifique a conexao" : "Comando rejeitado pelo controle";
            ui.release();
        }
        drawPage();
    }
    ui.sync(controller.state());
    if (!controller.hardwareReady()) ui.release();
    if (Serial.available() && Serial.read() == '?') reportStatus();
    uint16_t x = 0, y = 0;
    const bool touched = display.getTouch(&x, &y);
    if (ui.manual() != ihm::Manual::None && (!touched || !manualTouchInside(x, y))) {
        ui.release(); controller.manualCommand(vira::link::Manual::None);
        notice = "Solicitacao de encerrar manual enviada";
        drawPage();
    }
    if (touched && !touchHeld) {
        if (!passwordPage()) Serial.printf("TOUCH %u %u\n", x, y);
        if (ui.page() == ihm::Page::Home) handleHome(x, y);
        else if (ui.page() == ihm::Page::Configuration) handleConfiguration(x, y);
        else if (ui.page() == ihm::Page::Faults) handleFaults(x, y);
        else if (ui.page() == ihm::Page::ResetCount) handleCounter(x, y);
        else handleEditor(x, y);
    }
    touchHeld = touched;
    if (ui.manual() != ihm::Manual::None && now - lastManualRenew >= vira::link::PollMs) {
        controller.manualCommand(static_cast<vira::link::Manual>(ui.manual()));
        lastManualRenew = now;
    }
    vira::link::Frame outgoing;
    if (controller.outgoing(outgoing)) {
        const auto packet = vira::link::encode(outgoing);
        controlSerial.write(packet.data(), packet.size());
    }
    const bool dashboard = ui.page() == ihm::Page::Home || ui.page() == ihm::Page::Configuration ||
                           ui.page() == ihm::Page::Faults || ui.page() == ihm::Page::ResetCount;
    if (dashboard && now - lastPaint >= 100 &&
        (paintedState != controller.state() || paintedCount != controller.produced() ||
         paintedRevision != controller.revision())) {
        drawPage();
        Serial.printf("STATE %s COUNT %lu\n", stateText(),
                      static_cast<unsigned long>(controller.produced()));
    }
    renderPage();
    delay(5);
}
