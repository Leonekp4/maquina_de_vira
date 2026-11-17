# Controlador ESP-32U — UART

Placa de 38 pinos, módulo ESP-32U, ESP32-D0WD-V3 rev. 3.1, flash de 4 MB,
conversor CP2102. Porta USB observada: `/dev/ttyUSB0`.

## Comunicação

UART2 a 115200, 8N1, RX=16, TX=17, GND comum com a IHM.
Seguir [ligação direta](../../doc/LIGACAO_UART.md).
A porta USB continua disponível para diagnóstico; enviar `?` a 115200.
Não há mais comandos de simulação `h`, `s`, `p`, `f`, `r` ou `c` pela USB.

## Implementação atual

- Atende pedidos da IHM com verificação de integridade, sessão, sequência e boot.
- Configura e confirma comprimento e tempos, com armazenamento NVS quando disponível.
- Confirma zeramento e parada sem inventar movimento ou contagem.
- Rejeita partida e manual com resultado HARDWARE PENDENTE.
- Não inicializa GPIOs de motor/relés/sensores nem chama o ciclo temporizado simulado.
- Perda de heartbeat expira a sessão em 1200 ms; nova sessão não inicia movimento.
- Os testes do núcleo de produção contínua continuam disponíveis no computador.

## Compilar/gravar

```sh
pio run -d firmware/controle-esp32 -e controle-uart
pio run -d firmware/controle-esp32 -e controle-uart -t upload --upload-port /dev/ttyUSB0
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

O próximo passo de atuação física exige identificar as interfaces elétricas e
calibrar os pulsos. A UART não executa funções de segurança de pessoas.
O backup original está em `backups/controle-original-704bca4617c0.bin` (4 MB),
com SHA-256 ao lado, fora do Git.
