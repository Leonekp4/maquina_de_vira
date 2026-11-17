# Integração da IHM com o controlador

## Situação

O usuário substituiu RS-485 por UART direta, sem módulos adicionais.
Motor e relés ainda não estão ligados. A ligação atual está em
[LIGACAO_UART.md](LIGACAO_UART.md).

O protocolo comum está integrado ao transporte UART. A IHM deixa de executar
uma simulação local. Os atuadores permanecem bloqueados no controlador,
aguardando confirmação das interfaces elétricas e calibração.

Antes de habilitar movimento real, confirmar:

1. Conferir a ligação direta TX/RX/GND entre os GPIOs definidos.
2. Ligações efetivas de PUL/STEP, DIR, ENA e relés de avanço e retorno.
3. Níveis ativos e interfaces elétricas dos módulos de relé e do driver.
4. Entrada da porta e diagnóstico do sistema independente de segurança.
5. Pulsos por comprimento, velocidade e aceleração, além do comportamento
   da válvula com as bobinas energizadas/desenergizadas.

## Responsabilidades na integração

- O controlador será a fonte de estado, contagem e parâmetros confirmados.
- A IHM enviará solicitações e apresentará a resposta do controlador.
- Contagem, início, parada, zerar e comandos manuais deixarão de ser locais à IHM.
- Perda de conexão deve bloquear novos comandos; comandos manuais exigirão
  renovação periódica e expiração, inclusive se a mensagem de soltar se perder.
- Parada operacional deve terminar o corte/retorno em condições normais.
  Falhas de comunicação ou de permissivos exigem tratamento próprio, definido
  com a configuração final do equipamento.
- Reconectar ou reiniciar uma das placas não deve iniciar movimentos.

## Formato preparado

Implementação: `shared/protocol/link_protocol.h` e `link_protocol.cpp`.
Pacotes de 64 bytes, campos inteiros little-endian, cabeçalho VIRA, versão 1
e CRC16 para detectar corrupção. O CRC não autentica a origem: o transporte
deverá validar os participantes e aplicar os recursos de proteção apropriados.

| Campo | Conteúdo |
|---|---|
| Tipo | Solicitação ou estado/resposta |
| Comando | Heartbeat, configurar, iniciar, parar, zerar, rearme, confirmação, manual |
| Resultado | Sem resultado, aceito, rejeitado, não configurado ou obsoleto |
| Identificação | Sessão da IHM, sequência da solicitação e identificação de boot do controle |
| Parâmetros | Comprimento em micrômetros e tempos em milissegundos |
| Telemetria | Contagem, estado, falha e comando manual |
| Permissivos | Hardware configurado, porta fechada, diagnóstico de segurança e parada pendente |

Identificadores de sessão/sequência serão usados para confirmar a aplicação
dos comandos e impedir repetição de ações ao retransmitir mensagens.
Os testes no computador validam serialização, tamanho, integridade, confirmação,
duplicatas, prioridades, timeout, reboot e ressincronização do fluxo. A conexão
física por fios exige conferência nas placas. Execução física continua pendente.
