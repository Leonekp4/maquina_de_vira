# Máquina de corte de vira

Primeira etapa: controlador operacional simulado em C++17, sem acesso a GPIOs.
A IHM de teste e o controlador simulado foram gravados nas respectivas placas.
As duas simulações ainda funcionam de forma independente, sem protocolo entre elas.

## Executar no computador

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/simulador --demo
./build/simulador
```

Exemplo interativo (tempo virtual, sem espera real):

```text
inputs 1 1 1
config 100000 3 100000 500 500
start
tick 1000
tick 500
tick 500
status
```

Comprimento e velocidade usam micrômetros e micrômetros/segundo: 100000 µm =
100 mm. Os tempos usam milissegundos. Todos os valores iniciais são exemplos
para simulação, sujeitos a dimensionamento e testes. Cada `tick` executa no
máximo uma transição, sem transferir tempo excedente para a etapa seguinte.

## Organização

- `firmware/controle-esp32/include/controller.h`: estados e contrato da lógica.
- `firmware/controle-esp32/src/controller.cpp`: núcleo independente do hardware.
- `firmware/controle-esp32/sim/`: simulador interativo para computador.
- `firmware/controle-esp32/test/`: testes de comportamento.
- `firmware/controle-esp32/src/main.cpp`: adaptação Arduino somente simulada.
- `firmware/ihm-esp32s3/`: IHM de teste com teclado e controlador local simulado.
- `doc/MAQUINA_CORTE_VIRA.md`: concepção original e decisões atualizadas.
- `doc/DECISOES_DESENVOLVIMENTO.md`: comportamento implementado e pendências.

## Simulação no ESP32 (opcional)

Com PlatformIO instalado:

```sh
cd firmware/controle-esp32
pio run -e controle-simulado
```

A compilação Arduino e a execução no ESP-32U foram verificadas por USB.
O firmware não chama `pinMode`, `digitalWrite` nem gera pulsos. Pelo monitor
serial a 115200: `h` libera permissivos simulados; `s` inicia; `p` solicita
parada após a peça; `f` injeta falha; `h`, `r`, `c`, `s` recuperam e reiniciam.

## Limites desta etapa

O movimento da alimentação é representado por duração calculada. Não há
driver físico, protocolo entre placas, detecção real de desconexão ou
persistência em flash. O checkpoint é uma estrutura em memória testada para
futura integração com armazenamento. Nenhum tempo decorrido confirma a
posição física da navalha. A simulação não implementa funções de segurança.

## IHM de teste

A tela de teste pode ser compilada separadamente para a SC01 Plus. Ela roda
o mesmo núcleo simulado localmente, sem comandar o ESP32 de controle.
Consulte [instruções e roteiro de teste](firmware/ihm-esp32s3/README.md).
