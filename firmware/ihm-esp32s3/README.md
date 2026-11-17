# IHM SC01 Plus — controlador remoto por UART

Panlee ZX3D50CE08S-V16-USRC, ESP32-S3, flash de 16 MB, PSRAM de 2 MB.
A IHM não executa mais simulação local: contagem, estados e parâmetros vêm
exclusivamente do ESP-32U. Seguir [ligação UART](../../doc/LIGACAO_UART.md).

## Interface

- Comprimento em cm com duas casas decimais, por exemplo `10.00`.
- Quantidade do lote mostra peças concluídas recebidas do controlador.
- Tocar na quantidade abre ZERAR/CANCELAR. Só indicar zeramento após confirmação.
- Botões fixos INICIAR, PARAR e FALHAS enviam solicitações ao controlador.
- Engrenagem com senha `1234` abre configuração de tempos e comandos manuais.
- Configuração só muda os valores mostrados após resposta válida do controle.
- Falhas reais/estado da comunicação substituem a antiga injeção de falha simulada.
- UART OFFLINE: dados numéricos substituídos por `--`, comandos bloqueados.
- UART ONLINE com HARDWARE NÃO CONFIGURADO: comunicação disponível, atuação pendente.

Nesta etapa, início/manual são recusados pelo ESP-32U: os acionamentos ainda
não foram identificados nem calibrados. A interface não finge executar o ciclo.
A senha protege a navegação da tela; não é uma função de segurança da máquina.

## Desenho

A imagem é montada em buffer RGB565 de 480×320 na PSRAM (307.200 bytes) e
enviada pronta ao display, sem apagar a tela física entre os elementos.
Atualizações de eventos são agrupadas por loop. Heartbeats sem mudança de
dados não provocam redesenho periódico. O comando USB `?` informa estado do link,
pinos, tamanho do buffer e número de quadros, sem gerar comandos de movimento.

## Compilar/gravar

```sh
pio run -d firmware/ihm-esp32s3 -e ihm-uart
pio run -d firmware/ihm-esp32s3 -e ihm-uart -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 --baud 115200
```

UART1: TX=GPIO10, RX=GPIO11, 115200 8N1. Confirmar os pinos no conector de I/O;
não ligar esses fios em A/B. A configuração não usa a RS-485 integrada.
A perda de respostas por 1200 ms bloqueia pedidos; comandos antigos não
são repetidos automaticamente ao reconectar. Solicitações retransmitidas
antes de expirar têm o mesmo ID para não repetir ações no controlador.

## Referências

- [Perfil SC01 Plus do fabricante](https://github.com/smartpanle/PanelLan_esp32_arduino/tree/2bc0d086c9a55f5288d85da6c16968c22744fe5f/src/board/sc01_plus)
- PanelLan no commit `2bc0d086c9a55f5288d85da6c16968c22744fe5f`.
- LovyanGFX 1.2.7; Espressif32 6.10.0 / Arduino.

O backup original de 16 MB está em `backups/ihm-original-4827e21f29b0.bin`,
com SHA-256 ao lado, fora do Git. A validação final do link precisa da ligação física.
