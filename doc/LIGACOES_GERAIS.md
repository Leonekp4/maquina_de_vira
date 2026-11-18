# Máquina de corte de vira — ligações gerais

**Data:** 17/11/2026

**Escopo:** comunicação, alimentação, driver/motor, relés/válvulas,
entradas de diagnóstico e conexões pneumáticas previstas.

**Estado:** UART definida no firmware; GPIOs de atuadores e sensores propostos,
ainda não habilitados. Este documento reúne o que já é conhecido e marca os
trechos que precisam de identificação dos componentes para fechar a montagem.

## 1. Equipamentos

| Identificação neste documento | Equipamento |
|---|---|
| IHM | Panlee SC01 Plus, ZX3D50CE08S-V16-USRC, módulo WT32-S3-WROVER |
| CTRL | Placa ESP-32U, ESP32-D0WD-V3, 38 pinos, flash de 4 MB |
| DRIVER | Dispositivo fotografado com inscrição Microstep Driver; modelo pendente |
| MOTOR | Motor que movimenta os roletes; etiqueta e características pendentes |
| K1 | Canal de relé destinado ao avanço da navalha |
| K2 | Canal de relé destinado ao retorno da navalha |
| VÁLVULA | Válvula pneumática comandada pelos dois canais; modelo pendente |
| CILINDRO | Cilindro pneumático que movimenta a navalha |

K1/K2 e os identificadores L01 etc. são referências deste documento,
não inscrições confirmadas dos componentes ou números de bornes.

## 2. Visão geral

```text
IHM com tela
  │ UART 3,3 V: TX, RX e GND
  ▼
CTRL ESP-32U
  ├── Interface de pulsos/direção/habilitação ── DRIVER ── MOTOR ── Roletes
  ├── Módulo de acionamento ── K1 ── Bobina de avanço da VÁLVULA
  ├── Módulo de acionamento ── K2 ── Bobina de retorno da VÁLVULA
  ├── Interface de entrada ◀── Sensor/contato de diagnóstico da porta
  └── Interface de entrada ◀── Contato auxiliar de diagnóstico de segurança

Ar comprimido ── Preparação/controle de ar ── VÁLVULA ── CILINDRO ── Navalha

Circuito independente de segurança ── Atuação sobre energias perigosas
                                     conforme projeto de segurança
```

## 3. Comunicação entre a IHM e o CTRL — definida no firmware

| Ligação | Origem | Destino | Função |
|---|---|---|---|
| L01 | IHM GPIO10, TX | CTRL GPIO16, RX | Dados da IHM para o controle |
| L02 | CTRL GPIO17, TX | IHM GPIO11, RX | Respostas do controle para a IHM |
| L03 | IHM GND | CTRL GND | Referência elétrica comum |

**Parâmetros:** UART, 115200 baud, 8 bits, sem paridade, 1 bit de parada.

- Montar os fios com as placas desligadas.
- Na IHM, usar os sinais GPIO10/11 do conector de **I/O**. Conferir sua
  posição no esquema/inscrições da revisão V16: GPIO10 não significa pino 10.
- Os terminais **A/B da RS-485 não são usados** e não são equivalentes a TX/RX TTL.
- Esta ligação não usa módulo RS-485 nem os GPIOs internos 42/1/2 dessa interface.
- Usar fios curtos na montagem de bancada, afastados de motor e bobinas.
- Não levar 5 V, 12 V ou 24 V às entradas de comunicação dos ESPs.

Os valores acima correspondem a [uart_pins.h](../shared/protocol/uart_pins.h).
O procedimento de conferência está em [LIGACAO_UART.md](LIGACAO_UART.md).

## 4. Alimentação das placas — montagem atual de bancada

| Ligação | Origem | Destino | Estado |
|---|---|---|---|
| L04 | USB do computador/fonte USB apropriada | USB-C da IHM | Alimentação da IHM |
| L05 | USB do computador/fonte USB apropriada | USB-C do CTRL | Alimentação do CTRL |
| L06 | IHM GND | CTRL GND | Mesmo fio de referência L03 |

Manter cada placa com sua alimentação USB. **Não unir os pinos de 5 V ou
3,3 V das duas placas nesta montagem.** A comunicação usa somente L01–L03.

A alimentação definitiva no painel, fontes, proteções, distribuição de
retornos, aterramento e eventual coexistência com USB ainda serão detalhados.
O ESP não fornece alimentação de potência ao motor nem às válvulas.

## 5. Mapa geral do CTRL

| GPIO | Sinal | Direção no CTRL | Situação |
|---|---|---|---|
| 16 | UART RX | Entrada | Implementado |
| 17 | UART TX | Saída | Implementado |
| 25 | STEP/PUL do motor | Saída | Proposto; desabilitado |
| 26 | DIR do motor | Saída | Proposto; desabilitado |
| 27 | ENA do driver | Saída | Proposto; desabilitado |
| 32 | Comando de K1, avanço | Saída | LOW inicial; HIGH durante teste manual sem cargas |
| 33 | Comando de K2, retorno | Saída | LOW inicial; HIGH durante teste manual sem cargas |
| 34 | Diagnóstico da porta | Entrada | Proposto; desabilitado |
| 35 | Diagnóstico de segurança | Entrada | Proposto; desabilitado |

GPIO23, proposto anteriormente para direção RS-485, não é utilizado na UART atual.
Os GPIOs 34/35 são somente entradas e não possuem pull-up/pull-down interno.

## 6. Comandos do driver do motor — proposta pendente de interface elétrica

| Ligação | Origem | Caminho até o driver | Informação necessária |
|---|---|---|---|
| L07 | CTRL GPIO25 | Interface elétrica → PUL+/PUL- | Tensão, corrente e temporização das entradas |
| L08 | CTRL GPIO26 | Interface elétrica → DIR+/DIR- | Tipo de ligação e nível de direção |
| L09 | CTRL GPIO27 | Interface elétrica → ENA+/ENA- | Polaridade e comportamento de habilitação |

Os pares `PUL+/PUL-`, `DIR+/DIR-` e `ENA+/ENA-` são visíveis na foto.
Ainda **não está definida** a associação de cada terminal +/− à alimentação,
retorno ou interface. Ela depende do manual do driver e do circuito escolhido.
Não interpretar esta tabela como ligação direta de GPIO aos pares do driver.

O texto **9–42 VDC** da etiqueta descreve alimentação de potência do driver;
não especifica a tensão das entradas PUL/DIR/ENA.

### Potência e motor

| Ligação | Origem | Destino | Estado |
|---|---|---|---|
| L10 | Positivo da fonte do motor, através das proteções definidas no projeto | DRIVER VCC | Tensão da fonte a dimensionar |
| L11 | Retorno da fonte do motor | DRIVER GND | Distribuição a detalhar |
| L12 | Par de fios de uma fase do MOTOR | DRIVER A+ e A- | Identificar o par e a corrente nominal |
| L13 | Par de fios da outra fase do MOTOR | DRIVER B+ e B- | Identificar o par e a corrente nominal |

As associações L12/L13 pressupõem motor compatível de duas fases, conforme
as saídas mostradas no driver. Confirmar a etiqueta e as ligações do motor;
não escolher os pares apenas pela cor dos fios.

Antes de habilitar movimento, registrar posições SW1–SW6, corrente do motor,
pulsos por volta, diâmetro efetivo dos roletes e eventual transmissão.
Dimensionar a fonte dentro das especificações efetivas de driver e motor;
a faixa impressa não determina uma tensão única de operação.

## 7. Acionamento dos relés — proposta

| Ligação | Origem | Destino previsto | Estado |
|---|---|---|---|
| L14 | CTRL GPIO32 | Entrada de comando do canal K1 | Confirmar compatibilidade com 3,3 V e nível ativo |
| L15 | CTRL GPIO33 | Entrada de comando do canal K2 | Confirmar compatibilidade com 3,3 V e nível ativo |
| L16 | Fonte apropriada ao módulo | Alimentação do módulo de relés | Tensão/modelo pendentes |
| L17 | Referência do circuito lógico | Referência da entrada do módulo | Definir conforme isolamento e esquema do módulo |

Os nomes IN1/IN2, VCC, GND ou JD-VCC variam por placa. Não atribuir esses
bornes sem identificar o módulo. Não conectar bobinas de relés diretamente
aos GPIOs. Confirmar se a entrada é ativa em LOW ou HIGH e o estado ao ligar.

Em módulo com isolamento, não unir referências arbitrariamente: seguir o
esquema do fabricante para preservar a separação elétrica prevista.

## 8. Contatos dos relés e bobinas das válvulas — a detalhar

| Ligação funcional | Caminho previsto | Pendência |
|---|---|---|
| L18 | Circuito de alimentação autorizado → contatos de K1 → bobina de avanço | Tensão/tipo AC ou DC, corrente, bornes e proteção |
| L19 | Circuito de alimentação autorizado → contatos de K2 → bobina de retorno | Tensão/tipo AC ou DC, corrente, bornes e proteção |
| L20 | Outro terminal de cada bobina → retorno da respectiva alimentação | Polaridade e circuito exatos a confirmar |

A escolha de COM/NA/NF, fusíveis e supressão dos transientes depende dos
componentes e do comportamento requerido. Não há valores nem ligações de
potência definitivos sem esses dados. Bobinas DC com acessórios eletrônicos
podem ter polaridade; proteção para bobina AC pode ser diferente da usada em DC.

Avanço e retorno não devem ser comandados simultaneamente. O efeito de
desligar ambas as bobinas depende da válvula; não presumir recolhimento ou
imobilização do cilindro nessa condição. Essa definição integra o projeto pneumático.

## 9. Entradas da porta e do sistema de segurança — proposta

| Ligação | Origem | Caminho previsto | Situação |
|---|---|---|---|
| L21 | Sensor/contato de diagnóstico da porta | Interface de entrada → CTRL GPIO34 | Tipo, tensão e lógica pendentes |
| L22 | Contato auxiliar de diagnóstico da segurança | Interface de entrada → CTRL GPIO35 | Interface e lógica pendentes |

As entradas devem receber níveis compatíveis com 3,3 V. Para contato seco,
definir polarização externa; para sensor eletrônico, projetar a interface
de acordo com saída PNP/NPN, tensão e corrente. Não conectar saída industrial
de 12/24 V diretamente ao ESP.

O circuito independente de segurança, sua parada de emergência, intertravamento
da porta e atuação sobre as energias perigosas devem ser detalhados e validados
no projeto correspondente. GPIO34/35 são diagnósticos; não substituem esse circuito.

## 10. Ligações pneumáticas previstas

| Ligação funcional | Origem | Destino | Pendência |
|---|---|---|---|
| P01 | Rede de ar comprimido | Conjunto de preparação/controle do ar | Pressão, vazão e componentes |
| P02 | Saída do conjunto de ar | Alimentação da VÁLVULA | Modelo e identificação do pórtico |
| P03 | Saída de trabalho da VÁLVULA | Uma câmara do CILINDRO | Identificar avanço/retorno no conjunto real |
| P04 | Outra saída de trabalho | Outra câmara do CILINDRO | Confirmar cilindro de dupla ação e válvula compatível |
| P05 | Escapes da VÁLVULA | Tratamento/descarga previsto no projeto | Portas, silenciadores e controle de fluxo |

Esta é uma descrição funcional. Não atribuir números de portas, mangueiras,
restritores ou sentido de movimento sem o símbolo pneumático da válvula e
os dados do cilindro. Considerar energia armazenada e comportamento em falhas.
Não estão previstos sensores de posição do cilindro: avanço e retorno serão
temporizados; tempo decorrido não confirma posição física.

## 11. Estado atual e conferência da montagem

1. Conferir fisicamente os GPIO10/11 no I/O da IHM e os GPIO16/17 no CTRL.
2. Com tudo desligado, montar somente comunicação L01–L03.
3. Alimentar as placas por USB e conferir UART ONLINE e respostas a `?`.
4. Alterar comprimento/tempos e verificar confirmação do controle.
5. Conferir perda/reconexão de comunicação sem início automático.
6. Identificar componentes e fechar os circuitos L07–L22/P01–P05 antes da atuação.

Os firmwares UART foram gravados. Em 18/11/2026, após corrigir a ligação do CTRL
para GPIO16/17, os diagnósticos das duas placas confirmaram LINK ONLINE.
Após novo relato de acionamento na partida, GPIO32/33 passaram a iniciar em
LOW no aplicativo, com HIGH durante o comando manual. Confirmar visualmente
o resultado com os módulos sem cargas.
Os demais GPIOs de atuadores/sensores não são inicializados, e início/motor
manual continuam respondendo HARDWARE PENDENTE. Os botões de válvula agora
permitem [teste dos relés sem cargas](TESTE_RELES.md), com renovação e expiração.

## 12. Informações para concluir as ligações pendentes

- Foto aproximada do I/O da IHM ou esquema da revisão V16 para identificar posições físicas.
- Marca/modelo/manual do driver, etiqueta do motor e posição das chaves SW1–SW6.
- Foto/modelo da placa de relés e seus terminais, alimentação e níveis ativos.
- Modelo e símbolo da válvula, tensão/corrente e tipo AC/DC de cada bobina.
- Dados do cilindro, pressão/vazão e requisitos de controle pneumático.
- Tipo e tensão do sensor da porta e dados do contato de diagnóstico da segurança.
- Fontes, proteções, aterramento e definição do circuito independente de segurança.

## 13. Referências

- [Ligação UART e diagnóstico](LIGACAO_UART.md)
- [Decisões de desenvolvimento](DECISOES_DESENVOLVIMENTO.md)
- [Proposta anterior RS-485 — histórico](PINAGEM_PROPOSTA.md)
- [GPIOs do perfil SC01 Plus, fabricante](https://github.com/smartpanle/PanelLan_esp32_arduino/blob/master/src/board/sc01_plus/sc01_plus_pin.h)
- [Datasheet ESP32, Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

Este documento não define um projeto executivo de alimentação, segurança ou
pneumática para os componentes ainda não identificados.
