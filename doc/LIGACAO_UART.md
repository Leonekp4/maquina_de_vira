# Ligação direta IHM ↔ ESP-32U

## Três fios de comunicação

| IHM SC01 Plus, conector de I/O | ESP-32U |
|---|---|
| GPIO10 — TX | GPIO16 — RX |
| GPIO11 — RX | GPIO17 — TX |
| GND | GND |

Configuração: UART, 115200 baud, 8 bits, sem paridade, 1 bit de parada.
Os GPIO10/11 correspondem aos sinais externos do perfil SC01 Plus do
fabricante. A posição física no conector deve ser conferida pelas inscrições
ou esquema da revisão V16; número de GPIO não é posição do conector.

Desligar as placas para montar os fios. Usar fios curtos para a primeira
montagem de bancada, afastados da potência. Manter alimentação USB própria
de cada placa e não interligar seus pinos de 5 V/3,3 V nesta etapa.
Os sinais são de 3,3 V. Não usar os terminais A/B do conector RS-485 da IHM:
eles não são UART TTL. Os GPIOs 42/1/2 internos da RS-485 não são usados.

## O que os firmwares fazem

- IHM: apresenta somente estado, contagem e parâmetros recebidos do controlador.
  Não executa mais ciclo/contagem local e não injeta falhas ou permissivos falsos.
- Controle: atende à UART, valida solicitações e confirma parâmetros/zeramento.
  Os ajustes de comprimento e tempos são armazenados em NVS, quando disponível.
- Pacotes têm sessão, sequência, identificação de boot e CRC. Retransmissões
  do mesmo comando não repetem a ação. Parada tem prioridade sobre pedido pendente.
- Heartbeat a cada 200 ms. Sem resposta por 1200 ms, a IHM bloqueia novos pedidos,
  mostra SEM COMUNICAÇÃO e não repete automaticamente comandos de movimento.
- GPIO32/33 iniciam em LOW; o teste manual usa HIGH para ligar. A lógica foi
  invertida a pedido do usuário após relato de relés ativos na partida.
  Confirmar o resultado com cargas desconectadas. Nenhum GPIO de motor ou sensor é inicializado. O controle responde
  HARDWARE PENDENTE para início e motor manual. Os relés podem ser testados
  manualmente sem cargas, conforme [TESTE_RELES.md](TESTE_RELES.md). Não há movimento nem
  contagem simulada para apresentar o equipamento como operacional.
- A contagem ainda não é persistida em flash; como a atuação está bloqueada,
  permanece zero nesta etapa. Não há restauração de produção real implementada.

## Conferência

1. Depois de ligar TX/RX cruzados e GND, a IHM deve mostrar UART ONLINE.
2. Alterar comprimento/tempos e conferir a mensagem de confirmação do controle.
3. Reiniciar somente a IHM: os valores devem vir novamente do ESP-32U.
4. Reiniciar o controle: os valores devem voltar do NVS, sem partida automática.
5. Desconectar o cabo: deve aparecer SEM COMUNICAÇÃO, sem dados fictícios.
6. INICIAR/motor manual devem ser rejeitados como hardware pendente. O teste
   dos relés permanece limitado aos módulos sem cargas.

No monitor USB de cada placa, enviar `?` a 115200 para consultar o diagnóstico.
Na IHM: `IHM_UART_READY`, TX=10/RX=11. No controle: `CONTROLE_UART_READY`, TX=17/RX=16.

## Compilar e gravar

```sh
pio run -d firmware/ihm-esp32s3 -e ihm-uart
pio run -d firmware/controle-esp32 -e controle-uart
pio run -d firmware/ihm-esp32s3 -e ihm-uart -t upload --upload-port /dev/ttyACM0
pio run -d firmware/controle-esp32 -e controle-uart -t upload --upload-port /dev/ttyUSB0
```

As portas podem mudar. Backups dos programas originais estão em `backups/`,
fora do Git. Testar a montagem física antes de afirmar que as placas estão sincronizadas.

## Próxima integração

Confirmar interfaces de STEP/DIR/ENA e relés, níveis ativos, alimentação das
válvulas, calibração de pulsos e sinais reais da porta/segurança. Só então
substituir o bloqueio de atuação por drivers físicos e validar os ciclos.
O protocolo de comunicação não executa funções de segurança de pessoas.

## Validação desta etapa

- Cinco suítes de testes no computador aprovadas.
- Os dois ambientes PlatformIO compilaram e foram gravados, com verificação por hash.
- Diagnósticos USB confirmaram `IHM_UART_READY` e `CONTROLE_UART_READY`, sem simulação.
- Em 18/11/2026, após mover os fios do CTRL dos terminais RX/TX para GPIO16/17,
  os diagnósticos da IHM e do CTRL confirmaram LINK ONLINE a 115200 baud.
  Os dois lados ainda informam HARDWARE PENDENTE para os acionamentos.

Referências: [GPIOs externos SC01 Plus](https://github.com/smartpanle/PanelLan_esp32_arduino/blob/master/src/board/sc01_plus/sc01_plus_pin.h),
[documentação ESP32](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf).
