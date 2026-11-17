# Máquina de corte de vira

IHM SC01 Plus e controlador ESP-32U com comunicação UART direta de 3,3 V.
A IHM apresenta dados confirmados pelo controlador, sem executar ciclo local.
A atuação física continua bloqueada até identificar interfaces, permissivos e calibração.

## Ligação e integração

O mapa completo de comunicação, alimentação, driver, relés, entradas e
pneumática está em [LIGACOES_GERAIS.md](doc/LIGACOES_GERAIS.md).

Seguir [LIGACAO_UART.md](doc/LIGACAO_UART.md):

| IHM | ESP-32U |
|---|---|
| GPIO10 TX | GPIO16 RX |
| GPIO11 RX | GPIO17 TX |
| GND | GND |

Usar o conector de I/O da IHM, não os terminais A/B da RS-485.
Conferir a posição dos GPIOs no conector da revisão V16 antes de montar.

## Compilar

```sh
pio run -d firmware/ihm-esp32s3 -e ihm-uart
pio run -d firmware/controle-esp32 -e controle-uart
```

## Testes no computador

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

O simulador antigo permanece disponível como ferramenta de desenvolvimento:

```sh
./build/simulador --demo
./build/simulador
```

## Organização

- `firmware/ihm-esp32s3/`: tela, teclado, senha e cliente UART.
- `firmware/controle-esp32/`: endpoint UART, parâmetros e núcleo de controle.
- `shared/protocol/`: pacotes, CRC, decoder de fluxo, confirmações e supervisão de conexão.
- `doc/`: especificação, decisões, ligação e pendências de hardware.

## Estado funcional

Comprimento em cm com duas casas, tempos e zeramento são solicitados pela IHM
com confirmação do ESP-32U. Comprimento e tempos são salvos no NVS do controlador,
quando disponível. A contagem é de responsabilidade do controle e ainda não tem
persistência; permanece zero enquanto os atuadores estiverem bloqueados.
Não há permissivos falsos, contagem fictícia ou partida automática na reconexão.
A operação real do motor e das válvulas ainda depende da integração física.
