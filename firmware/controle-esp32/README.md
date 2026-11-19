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
- Rejeita partida e manual do motor com resultado HARDWARE PENDENTE.
- Permite somente teste manual dos dois relés sem cargas, com capacidade específica
  e expiração de renovação em 600 ms; ver [TESTE_RELES.md](../../doc/TESTE_RELES.md).
- Não inicializa GPIOs de motor/sensores nem chama o ciclo temporizado simulado.
- Inicialização dos relés: GPIO32/33 em HIGH antes de habilitar OUTPUT e antes
  de inicializar USB/UART/NVS; LOW durante o comando. Lógica invertida a pedido do usuário.
  O estado físico dos relés ainda aguarda confirmação visual.
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

O teste da inicialização dos relés foi autorizado com as cargas dos contatos
desconectadas. O sintoma de acionamento com GPIOs não configurados não
confirma a polaridade do módulo. A inicialização do aplicativo também não
garante o nível durante alimentação/reset/bootloader; caso necessário, definir
polarização externa compatível após identificar o circuito de entrada.

O diagnóstico `?` informa o nível de repouso configurado e `RELES_LATCH`,
que lê o registrador de saída dos GPIOs; essa leitura não mede tensão nem
confirma o estado dos contatos do módulo.
O backup original está em `backups/controle-original-704bca4617c0.bin` (4 MB),
com SHA-256 ao lado, fora do Git.
