# Decisões para a primeira implementação

## Hardware identificado

| Função | Hardware | Evidência |
|---|---|---|
| IHM | Panlee ZX3D50CE08S-V16-USRC / SC01 Plus | Inscrição na foto |
| Módulo IHM | WT32-S3-WROVER; ESP32-S3 rev. 0.2 | Foto e consulta USB |
| Memória IHM | Flash 16 MB; PSRAM integrada 2 MB | Consulta USB |
| Controle | Módulo marcado ESP-32U, placa de 38 pinos, CP2102 | Foto |
| Chip do controle | ESP32-D0WD-V3 rev. 3.1; flash 4 MB | Consulta USB |
| Alimentação da vira | Driver marcado Microstep Driver, PUL/DIR/ENA | Foto; modelo exato pendente |
| Navalha | Cilindro pneumático, duas saídas de relé | Informação do usuário |

Portas observadas: IHM `/dev/ttyACM0`; controle `/dev/ttyUSB0`. Podem mudar.
A denominação informal “servo” não confirma o tipo do motor. A etiqueta do
motor continua pendente; o dispositivo mostrado é um driver de microstepping.

Referências da IHM:

- [Datasheet Panlee da família SC01 Plus](https://img03.71360.com/w3/4e1e0o/20241022/49b7bc08380acfd89eda802ed27ace56.pdf)
- [Biblioteca do fabricante](https://github.com/smartpanle/PanelLan_esp32_arduino)

O datasheet descreve LCD 480×320, 3,5 polegadas, ST7796UI/8080 de 8 bits e
touch FT6336U/I2C. Validar pinagem para V16 antes da integração física.

## Escolhas confirmadas pelo usuário

- Começar pelo controlador simulado.
- Parada operacional conclui a peça atual antes de interromper a repetição.
- Recuperar dados do lote após reinicialização, exigindo confirmação.
- Alterações durante produção passam a valer na próxima peça.
- Exibir contagem somente do lote atual.
- IHM básica de parâmetros, operação, produção e alarmes.
- Comprimento ajustável durante o uso.
- Navalha sem sensores de posição: avanço e retorno por tempos ajustáveis na IHM.
- Comunicação entre placas ainda será comparada e escolhida.

## Regras provisórias da simulação

Estas regras completam a implementação inicial; não são novas confirmações do usuário:

- Quantidade representa o total desejado do lote. Não aceitar total inferior à
  contagem confirmada, nem total já alcançado durante uma peça em execução.
- Congelar comprimento, velocidade e tempos da peça ao iniciar sua alimentação;
  edições ficam pendentes para a próxima peça.
- Incrementar contagem após terminar o tempo de retorno, sem afirmar que o corte
  ou o retorno físico foram confirmados. Não há medição de qualidade da peça.
- Porta, diagnóstico de segurança e comunicação são entradas simuladas, inicialmente
  falsas. A perda de qualquer uma durante o ciclo gera falha e retira as intenções
  operacionais de saída. A reação física pertence ao projeto independente de segurança.
- Rearme vai para recuperação; confirmação explícita resolve a pendência da peça
  interrompida, preserva somente a contagem concluída e não inicia movimento.
  O procedimento físico de recuperação ainda precisa ser definido.
- Novo lote configurado após conclusão zera a contagem. Uma parada entre peças
  preserva a contagem e exige nova partida.
- Checkpoints existem apenas em memória. Persistência real, integridade e política
  de gravação serão implementadas antes de prometer recuperação após queda de energia.

## Pendências de integração

1. Identificar motor, driver, posições das chaves e interfaces elétricas de comando.
2. Confirmar válvula e módulos de relé: níveis ativos, estado inicial, duração de
   energização das bobinas e comportamento com ambos os comandos desligados.
3. Definir interface física da porta e do diagnóstico do sistema de segurança.
4. Validar GPIOs, pinagem V16 da IHM, bibliotecas e configuração de compilação.
5. Definir calibração, limites, velocidade, aceleração e tolerância de comprimento.
6. Comparar UART/RS-485 e protocolos; implementar confirmação e supervisão de conexão.
7. Definir recuperação da peça interrompida e comportamento físico após falhas.
8. Integrar armazenamento e validar recuperação com quedas de energia.
9. Detalhar avaliação de riscos e validação para a configuração física final.

Os tempos iniciais de 500 ms não são valores aprovados para o equipamento.
O firmware desta etapa não aciona os componentes físicos.

## IHM de teste implementada

A pasta `firmware/ihm-esp32s3` contém uma interface Arduino com PanelLan e
LovyanGFX, perfil SC01 Plus do fabricante. A aplicação reutiliza o núcleo do
controlador para simulação local e não inicializa comunicação entre placas.
Há teclado numérico, quatro parâmetros, estados, contagem, início/parada e
injeção de falha de porta com rearme e confirmação separados.

Os valores são validados antes de chegar ao controlador. Os limites da IHM
são provisórios: comprimento 0,001 a 1.000.000 mm, lote 1 a 1.000.000 peças,
tempos 1 a 60.000 ms. Não representam a capacidade mecânica da máquina.
O tamanho de flash segue a consulta física de 16 MB, não o valor genérico
de 8 MB encontrado no exemplo da biblioteca. Dependências foram fixadas
no `platformio.ini` para tornar a compilação reproduzível.

### Ajustes de navegação e operação manual

A principal ganhou engrenagem no canto superior direito. O acesso à
configuração exige a senha solicitada `1234`, com entrada mascarada.
Os campos de tempo da válvula foram movidos para a configuração; seu lugar
na principal agora contém avanço e retorno manuais do motor. A configuração
também contém avanço/retorno do motor e avanço/retorno da válvula.

Para a versão de teste, os comandos manuais apenas alteram a indicação e o
diagnóstico serial local: nenhuma atuação física ou comunicação foi adicionada.
São momentâneos enquanto pressionados, encerram ao soltar ou arrastar para
fora, são mutuamente exclusivos e bloqueados durante ciclo, falha ou recuperação.
