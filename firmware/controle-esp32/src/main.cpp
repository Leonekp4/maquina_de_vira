#include <Arduino.h>
#include "controller.h"

vira::Controller controller;
unsigned long previousMs = 0;
vira::State previousState = vira::State::Idle;
void reportStatus() {
    Serial.printf("CONTROLE_SIM_READY | %s | pecas=%lu | falha=%s | FLASH=%u | SEM GPIO\n",
                  vira::name(controller.state()), static_cast<unsigned long>(controller.produced()),
                  vira::name(controller.fault()), ESP.getFlashChipSize());
}
void setup() {
    Serial.begin(115200);
    controller.configure({});
    Serial.println("SIMULACAO SEM GPIO: h=permissivos simulados; s=iniciar; p=parar apos peca; f=falha; r=rearme; c=confirmar recuperacao");
    reportStatus();
    previousMs = millis();
}
void loop() {
    const auto now = millis();
    const auto elapsed = now - previousMs;
    previousMs = now;
    // Contabiliza somente o intervalo anterior ao novo comando.
    controller.tick(elapsed);
    if (Serial.available()) {
        switch (Serial.read()) {
        case '?': reportStatus(); break;
        case 'h': controller.setInputs({true, true, true}); break;
        case 's': controller.start(); break;
        case 'p': controller.requestStop(); break;
        case 'f': controller.setInputs({false, false, false}); break;
        case 'r': controller.resetFault(); break;
        case 'c': controller.confirmRecovery(); break;
        }
    }
    if (controller.state() != previousState) {
        previousState = controller.state();
        Serial.printf("%s | pecas=%lu | falha=%s\n", vira::name(previousState),
                      static_cast<unsigned long>(controller.produced()), vira::name(controller.fault()));
    }
}
