# Máquina de Corte de Vira — Especificação Técnica e Plano de Desenvolvimento

**Versão:** 1.2 (preliminar)

**Ambiente:** VS Code + PlatformIO

**Linguagem proposta:** C++

**Status:** arquitetura em definição; não liberada para operação industrial.

**Atualização para desenvolvimento:** consultar [decisões de implementação](DECISOES_DESENVOLVIMENTO.md).
**Comunicação atual:** UART direta de 3,3 V entre as placas, conforme
[ligação e estado da integração](LIGACAO_UART.md). As propostas RS-485 abaixo
pertencem à concepção anterior. Atuadores ainda não habilitados.
O usuário confirmou navalha sem sensores de posição, com avanço e retorno por
tempos ajustáveis. Portanto, as referências abaixo à confirmação por sensores
representam a concepção anterior e não são implementadas na simulação inicial.
Tempo decorrido não será apresentado como confirmação física de posição.
As placas identificadas e as demais escolhas atuais constam no documento vinculado.

## 1. Objetivo

Desenvolver uma máquina automática para alimentação e corte de vira, com comprimento e quantidade configuráveis por uma IHM baseada em ESP32-S3. Um segundo ESP32 executará o controle operacional do motor de passo e do cilindro pneumático da navalha.

## 2. Arquitetura

```text
ESP32-S3 com display (IHM)
  |  parâmetros, comandos, alarmes e produção
  |  UART / RS-485 (proposta)
  v
ESP32 controlador (modelo exato a confirmar; referido como ESP32-U)
  |-- STEP / DIR / ENA --> Driver Microstep --> Motor de passo --> Roletes
  |-- RELAY_ADVANCE --> Bobina de avanço da válvula pneumática
  |-- RELAY_RETURN  --> Bobina de retorno da válvula pneumática
  |<-- DOOR_SENSOR  <-- Sensor magnético comum da porta (diagnóstico)
  |<-- SAFETY_OK    <-- Contato auxiliar de diagnóstico do sistema de segurança
  |<-- (proposto) sensores de posição avançada/recolhida do cilindro

SISTEMA DE SEGURANÇA INDEPENDENTE DO ESP32
  Chave de intertravamento de segurança da porta
  + parada de emergência e demais dispositivos exigidos pela análise de riscos
  --> relé/controlador de segurança
  --> atuação segura sobre energias perigosas do motor e da pneumática
```

**Importante:** um sensor magnético comum não deve ser a única proteção de uma porta que dá acesso à navalha. O ESP32 não substitui relé/controlador de segurança. A arquitetura e o desempenho requerido das funções de segurança devem ser definidos por análise de riscos, considerando NR-12 e normas técnicas aplicáveis (por exemplo, ABNT NBR ISO 12100, ISO 13849-1 e ISO 14119). A remoção de energia não garante, por si só, a imobilização do cilindro: avaliar energia pneumática armazenada, tipo da válvula, movimento residual e necessidade de bloqueio da porta.

## 3. Entradas, saídas e comunicação

| Sinal lógico | Tipo | Finalidade | Observações |
|---|---|---|---|
| `DOOR_SENSOR` | Entrada digital | Estado do sensor magnético comum | Monitoramento operacional; não é função de segurança |
| `SAFETY_OK` | Entrada digital | Diagnóstico do circuito de segurança | Não deve executar a função de segurança |
| `STEP` | Saída | Pulsos do motor | Temporização adequada ao driver |
| `DIR` | Saída | Sentido de rotação | Confirmar requisitos elétricos |
| `ENA` | Saída | Habilitação operacional do driver | Não equivale a STO nem parada segura |
| `RELAY_ADVANCE` | Saída | Comando da bobina de avanço | Interface elétrica e proteção adequadas |
| `RELAY_RETURN` | Saída | Comando da bobina de retorno | Interface elétrica e proteção adequadas |
| `UART_TX` / `UART_RX` | Comunicação | IHM ↔ controlador | Preferência inicial por transceptores RS-485 |
| `CYL_EXTENDED` | Entrada proposta | Confirma cilindro avançado | Sensor a especificar |
| `CYL_RETRACTED` | Entrada proposta | Confirma cilindro recolhido | Sensor a especificar |

**Não atribuir GPIOs definitivos** antes de confirmar os modelos das placas e as pinagens ocupadas pelo display.

## 4. Sensor magnético comum da porta

### 4.1 Hipótese de circuito para contato seco de dois fios

Somente se for confirmado que o sensor é um **contato seco**, sem alimentação própria:

```text
3,3 V --- resistor pull-up externo (ex.: 10 kΩ) ---+--- GPIO DOOR_SENSOR
                                                   |
                                             contato magnético
                                                   |
                                                  GND
```

Se o contato fechar com a porta fechada:

| Contato | GPIO | Interpretação operacional |
|---|---|---|
| Fechado | LOW | Porta fechada |
| Aberto | HIGH | Porta aberta ou circuito interrompido |

A interpretação precisa ser confirmada por teste do sensor e sua instalação. Para sensores eletrônicos PNP/NPN de 12/24 V, **não conectar diretamente ao GPIO de 3,3 V**: projetar interface apropriada, preferencialmente isolada quando necessário. Considerar ruído, comprimento do cabo, proteção contra surtos e filtragem.

### 4.2 Regras operacionais

- Porta aberta antes da partida: bloquear o início do ciclo.
- Porta aberta durante o ciclo: registrar falha, cancelar a sequência operacional e impedir novos comandos; **a reação física segura deve ser executada pelo circuito independente de segurança**.
- Porta fechada novamente: não reiniciar automaticamente; exigir condições de segurança, rearme quando aplicável e novo comando de partida.
- Leitura inválida ou falha detectada: bloquear operação e exibir alarme.
- `DOOR_SENSOR` sozinho não autoriza partida: também verificar `SAFETY_OK`, estado do cilindro e demais permissivos.

### 4.3 Exemplo didático PlatformIO (somente leitura; sem movimento)

```cpp
#include <Arduino.h>

// GPIO provisório: confirmar placa e pinagem.
constexpr uint8_t DOOR_SENSOR_PIN = 27;
constexpr uint8_t DOOR_CLOSED_LEVEL = LOW;
constexpr unsigned long DEBOUNCE_MS = 30;

bool doorClosed = false;
bool doorStateValidated = false;
bool lastRawState = false;
unsigned long lastChangeTime = 0;

void setup() {
    Serial.begin(115200);
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    lastRawState = digitalRead(DOOR_SENSOR_PIN) == DOOR_CLOSED_LEVEL;
    lastChangeTime = millis();
}

void loop() {
    const bool rawClosed =
        digitalRead(DOOR_SENSOR_PIN) == DOOR_CLOSED_LEVEL;

    if (rawClosed != lastRawState) {
        lastRawState = rawClosed;
        lastChangeTime = millis();
    }

    if (millis() - lastChangeTime >= DEBOUNCE_MS &&
        (!doorStateValidated || doorClosed != rawClosed)) {
        doorClosed = rawClosed;
        doorStateValidated = true;
        Serial.println(doorClosed ? "PORTA_FECHADA" : "PORTA_ABERTA");
    }

    // Somente diagnóstico. Não acionar motor nem válvulas aqui.
}
```

O debounce de 30 ms é apenas tratamento de sinal operacional, **não uma função de segurança**. O GPIO 27 e o uso de `INPUT_PULLUP` são provisórios.

## 5. Comandos pneumáticos

Os dois relés comandam as duas bobinas da válvula:

| Relé avanço | Relé retorno | Comando lógico |
|---|---|---|
| OFF | OFF | Nenhuma bobina comandada |
| ON | OFF | Avanço |
| OFF | ON | Retorno |
| ON | ON | **Proibido pelo firmware** |

Implementar exclusão mútua dos comandos e avaliar intertravamento elétrico. O efeito físico de ambos os relés desligados depende do tipo de válvula (por exemplo, uma 5/2 biestável pode manter a última posição). Confirmar modelo, tensão das bobinas, circuito de acionamento e comportamento em falhas antes de energizar.

## 6. Ciclo automático preliminar

1. Inicializar em estado sem autorização de movimento.
2. Validar condições operacionais, sinal de segurança e cilindro recolhido.
3. Aguardar partida explícita da IHM.
4. Avançar a fita com o motor de passo até o comprimento programado.
5. Parar o avanço da fita.
6. Comandar o avanço do cilindro para cortar.
7. Confirmar avanço por sensor, se previsto, com timeout de falha.
8. Descomandar avanço e comandar retorno conforme o tipo de válvula.
9. Confirmar cilindro recolhido com timeout de falha.
10. Incrementar contador e repetir até alcançar a quantidade programada.

Tratar estados como `IDLE`, `READY`, `FEEDING`, `CUTTING`, `RETURNING`, `COMPLETE` e `FAULT`. Toda transição deve definir permissivos e condições de falha. Não confiar apenas em temporizações para inferir posição do cilindro.

## 7. Comunicação IHM ↔ controlador

**Proposta:** UART sobre RS-485 para ambiente industrial. Usar transceptores compatíveis e avaliar isolamento galvânico, terminação, polarização e topologia dos cabos.

O protocolo deverá especificar:

- Comandos: partida, parada operacional, configuração de comprimento e quantidade, rearme operacional quando permitido.
- Telemetria: estado da porta, `SAFETY_OK`, estado da máquina, posição do cilindro, produção e alarmes.
- Identificador de mensagem, sequência, verificação de integridade, confirmação e timeout.
- Tratamento de perda de comunicação sem executar comandos antigos; a parada de segurança é independente da comunicação.

A IHM poderá utilizar LVGL se o hardware de display/touch for compatível. Não escolher drivers ou bibliotecas antes de identificar a placa.

## 8. Precisão do motor de passo

Estimativa teórica de pulsos por milímetro:

```text
pulsos_por_mm = (passos_por_volta * fator_microstepping) / (pi * diametro_efetivo_rolete_mm)
```

Exemplo **ilustrativo**, não especificação de hardware: motor de 200 passos/volta, microstepping 1/16, rolete de 40 mm:

```text
pulsos_por_mm = (200 * 16) / (pi * 40) ≈ 25,46
```

Calibrar com medições reais: microstepping não garante precisão proporcional, e o deslizamento da fita afeta o comprimento.

## 9. Estrutura sugerida no VS Code

```text
maquina-corte-vira/
├── firmware/
│   ├── ihm-esp32s3/
│   │   ├── platformio.ini
│   │   ├── src/main.cpp
│   │   ├── include/
│   │   └── test/
│   └── controle-esp32/
│       ├── platformio.ini
│       ├── src/main.cpp
│       ├── include/
│       └── test/
├── shared/
│   └── protocol/
├── docs/
│   ├── arquitetura/
│   ├── eletrica/
│   ├── seguranca/
│   └── testes/
└── README.md
```

Exemplo **provisório** de `firmware/controle-esp32/platformio.ini` para placa ESP32 genérica (substituir conforme hardware real):

```ini
[env:esp32-controller]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

## 10. Plano de ação

- [ ] **Fase 1 — Requisitos e hardware:** identificar placas, display, motor, driver, válvula, sensores e fontes; levantar velocidade, precisão e produtividade; iniciar análise de riscos.
- [ ] **Fase 2 — Elétrica e protocolo:** esquemas de alimentação, interfaces 3,3 V/24 V, RS-485, GPIOs, proteção EMC e especificação das mensagens.
- [ ] **Fase 3 — Firmware controlador:** drivers de GPIO, motor, relés, sensores, máquina de estados, timeouts e testes unitários/integração em bancada sem cargas perigosas.
- [ ] **Fase 4 — Firmware IHM:** telas de parâmetros, operação, produção, diagnósticos, alarmes e comunicação.
- [ ] **Fase 5 — Integração operacional:** alimentação da fita, corte, retorno, contagem, calibração e recuperação de falhas.
- [ ] **Fase 6 — Segurança:** concluir análise de riscos, dimensionar e validar funções de segurança, intertravamentos, parada de emergência e prevenção de partida inesperada; atividade transversal desde a Fase 1.
- [ ] **Fase 7 — Comissionamento:** testes documentados de precisão, repetibilidade, falhas de sensores, perda de comunicação, energização e segurança antes de operação real.

## 11. Pendências para fechar o projeto elétrico

1. Modelo exato da placa IHM ESP32-S3 e controlador do LCD/touch.
2. Modelo exato da placa/módulo denominado “ESP32-U”.
3. Marca e modelo do driver Microstep, tensão/corrente das entradas STEP/DIR/ENA.
4. Modelo do motor de passo, corrente nominal e alimentação.
5. Tipo da válvula pneumática (5/2 biestável, 5/3 etc.), tensão das bobinas e comportamento sem energia.
6. Sensor magnético: contato seco de dois fios ou eletrônico de três fios? NA/NF? Tensão nominal?
7. Sensores de posição avançada e recolhida do cilindro: existem?
8. Características da porta e dispositivos de intertravamento de segurança previstos.
9. Fontes de alimentação, comprimento dos cabos e condições de instalação.
10. Comprimento mínimo/máximo de corte, tolerância admissível e produtividade desejada.

## 12. Critérios de aceitação preliminares

- IHM transmite parâmetros e recebe estado com integridade e tratamento de timeout.
- Controlador não permite comandos simultâneos de avanço e retorno.
- Abertura da porta bloqueia novos ciclos no software; sistema independente realiza a função de segurança definida na análise de riscos.
- Fechamento da porta não causa reinício automático.
- Falhas de comunicação, sensores e timeout geram estado de falha observável.
- Calibração de comprimento é documentada com resultados de testes.
- Segurança e comissionamento são validados antes da operação industrial.

---

> **Aviso de engenharia:** este documento é uma especificação preliminar de desenvolvimento, não um projeto elétrico executivo nem uma certificação de conformidade NR-12. Nenhum exemplo de firmware aqui deve ser utilizado como único mecanismo de proteção de pessoas.
