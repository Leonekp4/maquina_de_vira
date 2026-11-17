# Proposta de ligação — RS-485 e controlador ESP-32U

**Histórico:** o usuário substituiu RS-485 por UART direta, pois não possui o
módulo. Para a montagem atual, seguir [LIGACAO_UART.md](LIGACAO_UART.md).
As sugestões de GPIOs dos atuadores abaixo continuam pendentes de validação.

Status: proposta para montagem, ainda sem validação dos módulos externos.
Nenhum firmware foi gravado com estas saídas habilitadas.

## Arquitetura

IHM SC01 Plus (RS-485 integrado) → cabo de comunicação → módulo RS-485
compatível com lógica 3,3 V → UART do ESP-32U.

Preferir módulo isolado com alimentação isolada do lado do barramento.
É necessário identificar seu modelo antes de definir alimentação e terminais.

## Pinos propostos no ESP-32U

| Função | GPIO | Ligação prevista |
|---|---|---|
| RS-485 TX | 17 | DI/TXD de entrada do transceptor |
| RS-485 RX | 16 | RO/RXD de saída do transceptor |
| Direção RS-485 | 23 | DE e /RE, se o módulo usar direção manual |
| Pulsos do motor | 25 | Interface elétrica → PUL/STEP do driver |
| Direção do motor | 26 | Interface elétrica → DIR do driver |
| Habilitação do motor | 27 | Interface elétrica → ENA do driver |
| Avanço da válvula | 32 | Entrada do módulo de acionamento do relé de avanço |
| Retorno da válvula | 33 | Entrada do módulo de acionamento do relé de retorno |
| Diagnóstico da porta | 34 | Entrada condicionada para 3,3 V |
| Diagnóstico de segurança | 35 | Entrada condicionada para 3,3 V |

Os GPIOs 34 e 35 são somente entradas e não têm pull-up/pull-down interno.
Prever polarização externa e interface adequada ao tipo de sensor/contato.
Essas entradas são operacionais/de diagnóstico, conforme o projeto já documentado.

## Transceptor RS-485

Para módulo com terminais DI, RO, DE e /RE:

- GPIO17 → DI (entrada de dados do transmissor).
- RO (saída de dados do receptor) → GPIO16.
- GPIO23 → DE e /RE unidos, se permitido pelo módulo e sem direção automática.
- GND do ESP → GND do lado lógico do módulo.
- Alimentação do módulo conforme seu manual; a lógica de saída para o ESP deve ser 3,3 V.

A IHM usa internamente TX=42, RX=1 e direção=2, conforme o perfil do fabricante.
Usar o conector RS-485 externo da IHM; não os GPIOs internos como A/B.

## Cabo entre os módulos

| IHM, conector RS-485 | Módulo junto ao controlador |
|---|---|
| A | A, após conferir a convenção dos fabricantes |
| B | B, após conferir a convenção dos fabricantes |
| GND/referência do barramento | COM/GND do lado do barramento |
| +5 V | Sem uso para a primeira ligação de comunicação |

Em módulo isolado, o COM do lado do cabo é distinto do GND lógico do ESP;
não unir os lados da barreira de isolamento. Verificar no manual do módulo.
Para bancada, as placas podem manter suas próprias alimentações USB.

Usar par trançado A/B, com referência de barramento apropriada. Prever
terminação compatível com o cabo (tipicamente 120 ohms entre A e B em cada
extremidade), verificando antes se os equipamentos já possuem terminação.
Manter o cabo de comunicação separado dos condutores do motor e das bobinas.

## Motor, relés e válvulas

A foto do driver identifica entradas PUL+/PUL-, DIR+/DIR- e ENA+/ENA-, mas
não confirma suas tensões, correntes ou temporizações. A ligação dos pares
à interface precisa do modelo/manual; os GPIOs acima não são autorização
para conexão direta aos terminais. A faixa 9–42 V da etiqueta é alimentação
de potência do driver e não informa a tensão das entradas de comando.

GPIO32/33 comandarão entradas de um módulo de relés/acionamento adequado.
Não conectar bobinas de relé ou de válvula diretamente ao ESP. Confirmar
nível ativo, tensão, capacidade dos contatos, proteção e estado ao inicializar.
O circuito dos contatos e a supressão das bobinas dependem da tensão e do
tipo AC/DC das válvulas; detalhar após identificar os componentes.

## Dados necessários para fechar o esquema

1. Foto/modelo do módulo RS-485 (incluindo inscrições dos terminais).
2. Foto/modelo da placa de relés, frente e verso, e tensão das bobinas das válvulas.
3. Modelo/manual do driver e etiqueta do motor; posição das chaves SW1 a SW6.
4. Tipo e tensão do sensor da porta e contato de diagnóstico da segurança.
5. Diâmetro do rolete e eventual transmissão para calibrar pulsos por comprimento.

## Referências

- [Perfil SC01 Plus do fabricante](https://github.com/smartpanle/PanelLan_esp32_arduino/blob/master/src/board/sc01_plus/sc01_plus_pin.h)
- [Datasheet ESP32, Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [RS-485 Design Guide, Texas Instruments](https://www.ti.com/lit/an/slla272d/slla272d.pdf)
