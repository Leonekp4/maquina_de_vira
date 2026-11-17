#include <Arduino.h>
#include <PanelLan.h>
#include <cmath>
#include "controller.h"
#include "ui_values.h"
#include "ui_session.h"

// Somente display/touch/backlight. Motor, valvulas e comunicacao seguem simulados.
PanelLan display(BOARD_SC01_PLUS);
vira::Controller controller;
ihm::Session ui;
bool touchHeld = false, stopPending = false;
String input;
const char* notice = "Segure os botoes para mover o motor";
const char* editorError = "";
unsigned long previousMs = 0, lastPaint = 0;
vira::State paintedState = vira::State::Idle;
std::uint32_t paintedCount = 0;
constexpr uint16_t BG = 0x0863, CARD = 0x1926, WHITE = 0xffff;
constexpr uint16_t MUTED = 0xa534, BLUE = 0x1c7f, GREEN = 0x05ad, RED = 0xd986;

void label(const String& text, int x, int y, int size = 2, uint16_t color = WHITE) {
    display.setTextColor(color);
    display.setTextSize(size);
    display.drawString(text, x, y);
}
void button(int x, int y, int w, int h, const String& text, uint16_t color) {
    display.fillRoundRect(x, y, w, h, 6, color);
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.drawString(text, x + (w - display.textWidth(text)) / 2, y + (h - 16) / 2);
}
String lengthText(std::uint32_t um) {
    char text[24];
    snprintf(text, sizeof(text), "%lu.%03lu", static_cast<unsigned long>(um / 1000),
             static_cast<unsigned long>(um % 1000));
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
    display.fillRoundRect(426, 2, 42, 34, 5, CARD);
    constexpr int cx = 447, cy = 19;
    display.fillCircle(cx, cy, 11, WHITE);
    for (int i = 0; i < 8; ++i) {
        const float angle = i * 0.78539816f;
        for (int offset = -1; offset <= 1; ++offset)
            display.drawLine(cx + std::lround(9 * std::cos(angle)) + offset,
                             cy + std::lround(9 * std::sin(angle)),
                             cx + std::lround(15 * std::cos(angle)) + offset,
                             cy + std::lround(15 * std::sin(angle)), WHITE);
    }
    display.fillCircle(cx, cy, 5, CARD);
}
void banner() {
    display.fillRoundRect(12, 38, 456, 34, 5,
                          controller.state() == vira::State::Fault ? RED : BLUE);
    label(ui.manual() == ihm::Manual::None ? stateText() : ihm::manualText(ui.manual()), 22, 47);
}
void card(int x, int y, const char* title, const String& value) {
    display.fillRoundRect(x, y, 222, 56, 6, CARD);
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
    display.fillScreen(BG);
    label("CORTE DE VIRA", 12, 10);
    label("SIMULACAO", 322, 16, 1, 0xff40);
    gear(); banner();
    const auto& p = controller.parameters();
    card(12, 82, "COMPRIMENTO (mm)", lengthText(p.lengthUm));
    card(246, 82, "QUANTIDADE DO LOTE", String(p.target));
    manualButton(12, 146, "MOTOR AVANCO", ihm::Manual::MotorForward);
    manualButton(246, 146, "MOTOR RETORNO", ihm::Manual::MotorReverse);
    label("Produzidas: " + String(controller.produced()) + " / " + String(p.target), 12, 212);
    label(notice, 12, 236, 1, MUTED);
    const auto s = controller.state();
    const char* action = s == vira::State::Complete ? "NOVO LOTE" :
                         s == vira::State::Fault ? "REARMAR" :
                         s == vira::State::Recovery ? "CONFIRMAR" : "INICIAR";
    button(12, 260, 144, 48, action, GREEN);
    button(168, 260, 144, 48, "PARAR", RED);
    button(324, 260, 144, 48, "FALHA", CARD);
    rememberPaint();
}
void drawConfiguration() {
    display.fillScreen(BG);
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
const char* fieldTitle() {
    if (passwordPage()) return "CONFIGURACAO - SENHA";
    switch (ui.field()) {
    case ihm::Field::Length: return "COMPRIMENTO EM mm";
    case ihm::Field::Quantity: return "TOTAL DE PECAS DO LOTE";
    case ihm::Field::Advance: return "TEMPO AVANCO VALVULA (ms)";
    case ihm::Field::Return: return "TEMPO RETORNO VALVULA (ms)";
    }
    return "";
}
void drawEditor() {
    display.fillScreen(BG);
    label(fieldTitle(), 12, 10);
    label(passwordPage() ? "Digite a senha para acessar os ajustes" :
          "SIMULACAO CONTINUA DURANTE A EDICAO", 12, 35, 1, MUTED);
    display.fillRoundRect(12, 52, 456, 46, 6, CARD);
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
        label(ui.field() == ihm::Field::Length ? "0.001..1000000 mm" :
              ui.field() == ihm::Field::Quantity ? "1..1000000 pecas" : "1..60000 ms", 352, 262, 1, MUTED);
    }
}
void drawPage() {
    if (ui.page() == ihm::Page::Home) drawHome();
    else if (ui.page() == ihm::Page::Configuration) drawConfiguration();
    else drawEditor();
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
        else if (input.length() < (passwordPage() ? 4u : 12u)) input += keys[i];
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
    else if (inside(x, y, 246, 82, 222, 56)) openEditor(ihm::Field::Quantity);
    else if (inside(x, y, 12, 146, 222, 50)) beginManual(ihm::Manual::MotorForward);
    else if (inside(x, y, 246, 146, 222, 50)) beginManual(ihm::Manual::MotorReverse);
    else if (inside(x, y, 12, 260, 144, 48)) {
        const auto state = controller.state();
        if (state == vira::State::Fault) {
            controller.setInputs({true, true, true});
            controller.resetFault(); notice = "Confirme resolucao da peca interrompida";
        } else if (state == vira::State::Recovery) {
            controller.confirmRecovery(); notice = "Recuperado. Toque INICIAR para continuar";
        } else {
            if (state == vira::State::Complete) controller.configure(controller.parameters());
            if (controller.start()) { stopPending = false; notice = "Producao simulada; sem saidas fisicas"; }
            else notice = "Partida indisponivel no estado atual";
        }
        drawPage();
    } else if (inside(x, y, 168, 260, 144, 48)) {
        const auto state = controller.state();
        if (state == vira::State::Feeding || state == vira::State::Cutting || state == vira::State::Returning) {
            controller.requestStop(); stopPending = true;
            notice = "Parada solicitada: concluindo a peca";
        } else notice = "Sem ciclo em andamento";
        drawPage();
    } else if (inside(x, y, 324, 260, 144, 48)) {
        const auto state = controller.state();
        if (state == vira::State::Feeding || state == vira::State::Cutting || state == vira::State::Returning) {
            controller.setInputs({false, true, true});
            stopPending = false; notice = "Porta aberta SIMULADA. Rearme necessario";
        } else notice = "Inicie um ciclo para injetar falha";
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
    Serial.println("IHM_SIM_READY SC01_PLUS 480x320; UI=2 CONFIG_SENHA; SEM ATUADORES");
    Serial.printf("Tela=%dx%d PSRAM=%u FLASH=%u ESTADO=%s PAGINA=%u MANUAL=%u\n",
                  display.width(), display.height(), ESP.getPsramSize(), ESP.getFlashChipSize(),
                  vira::name(controller.state()), static_cast<unsigned>(ui.page()), static_cast<unsigned>(ui.manual()));
}
void setup() {
    Serial.begin(115200);
    display.begin(); display.setRotation(1); display.setBrightness(180);
    display.setFont(&fonts::Font0); display.setTextDatum(lgfx::textdatum_t::top_left);
    controller.configure({});
    controller.setInputs({true, true, true}); // Somente permissivos virtuais.
    drawPage(); reportStatus(); previousMs = millis();
}
void loop() {
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
        else handleEditor(x, y);
    }
    touchHeld = touched;
    const bool dashboard = ui.page() == ihm::Page::Home || ui.page() == ihm::Page::Configuration;
    if (dashboard && now - lastPaint >= 100 &&
        (paintedState != controller.state() || paintedCount != controller.produced())) {
        drawPage(); lastPaint = now;
        Serial.printf("STATE %s COUNT %lu\n", vira::name(controller.state()),
                      static_cast<unsigned long>(controller.produced()));
    }
    delay(5);
}
