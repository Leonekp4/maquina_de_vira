#include <Arduino.h>
#include <PanelLan.h>
#include <cmath>
#include "controller.h"
#include "ui_values.h"
#include "ui_session.h"

// Somente display/touch/backlight. Motor, valvulas e comunicacao seguem simulados.
PanelLan display(BOARD_SC01_PLUS);
lgfx::LGFX_Sprite canvas(&display);
vira::Controller controller;
ihm::Session ui;
bool touchHeld = false, stopPending = false;
bool displayReady = false, redrawRequested = false;
std::uint32_t renderedFrames = 0;
String input;
const char* notice = "Segure os botoes para mover o motor";
const char* editorError = "";
unsigned long previousMs = 0, lastPaint = 0;
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
    switch (controller.state()) {
    case vira::State::Idle: return "SEM LOTE";
    case vira::State::Ready: return "PRONTO";
    case vira::State::Feeding: return "ALIMENTANDO";
    case vira::State::Cutting: return "CORTE TEMPORIZADO";
    case vira::State::Returning: return "RETORNO TEMPORIZADO";
    case vira::State::Complete: return "LOTE CONCLUIDO";
    case vira::State::Fault: return "FALHA SIMULADA";
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
                          controller.state() == vira::State::Fault ? RED : BLUE);
    label(ui.manual() == ihm::Manual::None ? stateText() : ihm::manualText(ui.manual()), 22, 47);
}
void card(int x, int y, const char* title, const String& value) {
    canvas.fillRoundRect(x, y, 222, 56, 6, CARD);
    label(title, x + 10, y + 7, 1, MUTED);
    label(value, x + 10, y + 27);
}
void manualButton(int x, int y, const char* text, ihm::Manual command) {
    const bool allowed = ihm::Session::manualAllowed(controller.state());
    button(x, y, 222, 50, text,
           ui.manual() == command ? GREEN : allowed ? BLUE : CARD);
}
void rememberPaint() {
    paintedState = controller.state();
    paintedCount = controller.produced();
}
void drawHome() {
    canvas.fillScreen(BG);
    label("CORTE DE VIRA", 12, 10);
    label("SIMULACAO", 322, 16, 1, 0xff40);
    gear(); banner();
    const auto& p = controller.parameters();
    card(12, 82, "COMPRIMENTO (cm)", lengthText(p.lengthUm));
    card(246, 82, "QUANTIDADE DO LOTE", String(controller.produced()));
    manualButton(12, 146, "MOTOR AVANCO", ihm::Manual::MotorForward);
    manualButton(246, 146, "MOTOR RETORNO", ihm::Manual::MotorReverse);
    label(notice, 12, 236, 1, MUTED);
    button(12, 260, 144, 48, "INICIAR", GREEN);
    button(168, 260, 144, 48, "PARAR", RED);
    button(324, 260, 144, 48, "FALHAS", CARD);
    rememberPaint();
}
void drawConfiguration() {
    canvas.fillScreen(BG);
    label("CONFIGURACAO", 12, 10);
    label("SIMULACAO", 382, 16, 1, 0xff40);
    banner();
    manualButton(12, 82, "MOTOR AVANCO", ihm::Manual::MotorForward);
    manualButton(246, 82, "MOTOR RETORNO", ihm::Manual::MotorReverse);
    manualButton(12, 142, "VALVULA AVANCO", ihm::Manual::ValveAdvance);
    manualButton(246, 142, "VALVULA RETORNO", ihm::Manual::ValveReturn);
    card(12, 202, "TEMPO AVANCO VALVULA (ms)", String(controller.parameters().advanceMs));
    card(246, 202, "TEMPO RETORNO VALVULA (ms)", String(controller.parameters().returnMs));
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
    card(12, 92, "PECAS PRODUZIDAS", String(controller.produced()));
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
    const char* reason = "SEM FALHAS ATIVAS";
    switch (controller.fault()) {
    case vira::Fault::None: break;
    case vira::Fault::DoorOpen: reason = "PORTA ABERTA"; break;
    case vira::Fault::SafetyUnavailable: reason = "SEGURANCA INDISPONIVEL"; break;
    case vira::Fault::CommunicationLost: reason = "SEM COMUNICACAO"; break;
    }
    label(reason, 24, 118);
    button(12, 170, 222, 48, "SIMULAR FALHA", CARD);
    const auto s = controller.state();
    button(246, 170, 222, 48, s == vira::State::Recovery ? "CONFIRMAR" : "REARMAR",
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
          "SIMULACAO CONTINUA DURANTE A EDICAO", 12, 35, 1, MUTED);
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
        label("Limites de teste", 352, 245, 1, MUTED);
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
                editorError = "Valor invalido / total ja produzido";
            } else {
                notice = "Ajuste salvo; aplica na proxima peca";
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
    if (ui.pressManual(command, controller.state())) {
        notice = "Manual simulado: solte para parar";
        Serial.printf("MANUAL_SIM %s\n", ihm::manualText(command));
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
        if (ui.start(controller)) { stopPending = false; notice = "Producao iniciada"; }
        else notice = "Partida indisponivel. Consulte o estado e as falhas";
        drawPage();
    } else if (inside(x, y, 168, 260, 144, 48)) {
        const auto state = controller.state();
        if (state == vira::State::Feeding || state == vira::State::Cutting || state == vira::State::Returning) {
            controller.requestStop(); stopPending = true;
            notice = "Parada solicitada: concluindo a peca";
        } else notice = "Sem ciclo em andamento";
        drawPage();
    } else if (inside(x, y, 324, 260, 144, 48)) {
        ui.openFaults(); drawPage();
    }
}
void handleFaults(int x, int y) {
    if (inside(x, y, 12, 260, 456, 48)) { ui.home(); drawPage(); }
    else if (inside(x, y, 246, 170, 222, 48)) {
        if (controller.state() == vira::State::Fault) {
            controller.setInputs({true, true, true});
            controller.resetFault(); notice = "Confirme resolucao da peca interrompida";
        } else if (controller.state() == vira::State::Recovery) {
            controller.confirmRecovery(); notice = "Recuperado. Volte e toque INICIAR";
        } else notice = "Sem falha para rearmar";
        drawPage();
    } else if (inside(x, y, 12, 170, 222, 48)) {
        const auto state = controller.state();
        if (state == vira::State::Feeding || state == vira::State::Cutting || state == vira::State::Returning) {
            controller.setInputs({false, true, true});
            stopPending = false; notice = "Porta aberta SIMULADA. Rearme necessario";
        } else notice = "Inicie um ciclo para injetar falha";
        drawPage();
    }
}
void handleCounter(int x, int y) {
    if (inside(x, y, 246, 260, 222, 48)) { ui.home(); drawPage(); }
    else if (inside(x, y, 12, 260, 222, 48)) {
        if (ui.resetCount(controller)) {
            notice = "Contagem zerada";
            Serial.println("CONTAGEM_ZERADA");
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
    Serial.println("IHM_SIM_READY SC01_PLUS 480x320; UI=7 ZERAR_CONTAGEM; SEM ATUADORES");
    Serial.printf("Tela=%dx%d PSRAM=%u FLASH=%u ESTADO=%s PAGINA=%u MANUAL=%u\n",
                  display.width(), display.height(), ESP.getPsramSize(), ESP.getFlashChipSize(),
                  vira::name(controller.state()), static_cast<unsigned>(ui.page()), static_cast<unsigned>(ui.manual()));
    Serial.printf("RENDER=PSRAM_RGB565 BUFFER=%ux%u FRAMES=%lu\n", canvas.width(), canvas.height(),
                  static_cast<unsigned long>(renderedFrames));
    Serial.printf("COMPRIMENTO_CM=%s\n", lengthText(controller.parameters().lengthUm).c_str());
    Serial.printf("MODO=CONTINUO PECAS=%lu BOTOES=INICIAR,PARAR,FALHAS\n",
                  static_cast<unsigned long>(controller.produced()));
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
    vira::Parameters parameters;
    parameters.continuous = true;
    parameters.target = 0;
    controller.configure(parameters);
    controller.setInputs({true, true, true}); // Somente permissivos virtuais.
    drawPage(); renderPage(); reportStatus(); previousMs = millis();
}
void loop() {
    if (!displayReady) { delay(100); return; }
    const unsigned long now = millis();
    controller.tick(now - previousMs); previousMs = now;
    ui.sync(controller.state());
    if (Serial.available() && Serial.read() == '?') reportStatus();
    if (stopPending && controller.state() == vira::State::Ready) {
        stopPending = false; notice = "Parado apos concluir a peca";
    }
    uint16_t x = 0, y = 0;
    const bool touched = display.getTouch(&x, &y);
    if (ui.manual() != ihm::Manual::None && (!touched || !manualTouchInside(x, y))) {
        ui.release(); notice = "Comando manual encerrado";
        Serial.println("MANUAL_SIM PARADO"); drawPage();
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
    const bool dashboard = ui.page() == ihm::Page::Home || ui.page() == ihm::Page::Configuration ||
                           ui.page() == ihm::Page::Faults || ui.page() == ihm::Page::ResetCount;
    if (dashboard && now - lastPaint >= 100 &&
        (paintedState != controller.state() || paintedCount != controller.produced())) {
        drawPage();
        Serial.printf("STATE %s COUNT %lu\n", vira::name(controller.state()),
                      static_cast<unsigned long>(controller.produced()));
    }
    renderPage();
    delay(5);
}
