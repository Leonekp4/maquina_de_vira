# Controlador ESP-32U — firmware simulado

Placa de 38 pinos, módulo marcado ESP-32U, chip ESP32-D0WD-V3 revisão 3.1,
flash de 4 MB e conversor CP2102. Porta observada: `/dev/ttyUSB0`.

Esta versão roda o ciclo com tempos virtuais no próprio ESP32, sem configurar
GPIOs de motor, relés, sensores ou comunicação com a IHM. Não há persistência
em flash dos dados do lote. A IHM e o controlador permanecem simulações locais
independentes até a implementação do protocolo entre placas.

## Compilar e gravar

Na raiz do repositório, com PlatformIO:

```sh
pio run -d firmware/controle-esp32
pio run -d firmware/controle-esp32 -t upload --upload-port /dev/ttyUSB0
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

## Testar pelo monitor serial

| Caractere | Ação |
|---|---|
| `?` | Consultar identificação, estado, contagem, falha e flash |
| `h` | Liberar os três permissivos simulados |
| `s` | Iniciar/retomar lote |
| `p` | Parar após terminar a peça atual |
| `f` | Injetar falha durante o ciclo |
| `r` | Rearmar após restabelecer permissivos |
| `c` | Confirmar resolução da peça interrompida |

Parâmetros iniciais de demonstração: três peças de 100 mm, alimentação
simulada a 100 mm/s, avanço e retorno de 500 ms cada.

Para testar uma peça: enviar `h`, `s` e `p`; aguardar estado PRONTO com uma
peça contada. Para testar falha: iniciar com `s`, enviar `f`; restabelecer
permissivos com `h`, rearmar com `r`, confirmar com `c` e iniciar com `s`.
Não há partida automática após rearme ou confirmação.

A cópia do firmware anterior deve ser preservada em `backups/` antes de gravar.

## Validação realizada na placa

- Firmware compilado com Espressif32 6.10.0 e gravado pelo PlatformIO.
- Chip/MAC conferidos na gravação: ESP32-D0WD-V3 / `70:4b:ca:46:17:c0`.
- Conteúdo gravado verificado por hash pela ferramenta de upload.
- Diagnóstico serial identificou `CONTROLE_SIM_READY`, flash 4 MB e SEM GPIO.
- Comandos `hsp` concluíram uma peça e pararam com contagem 1.
- Comandos `sf` geraram falha de porta; `hrc?` recuperaram o estado PRONTO,
  preservando a contagem e sem reiniciar o ciclo automaticamente.
- Programa anterior preservado integralmente em
  `backups/controle-original-704bca4617c0.bin` (4 MB), com arquivo SHA-256 ao lado.
