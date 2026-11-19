# Teste manual dos módulos de relés — sem cargas

O usuário confirmou módulos sem válvulas ou outras cargas nos contatos.
Esta etapa permite testar os relés pela IHM sem liberar motor ou ciclo automático.

## Motivo do bloqueio anterior

A IHM exigia a capacidade global HardwareReady para todos os comandos e o
endpoint rejeitava qualquer manual diferente de parar. Inicializar GPIO32/33
em HIGH não implementava o acionamento dos botões.

## Comportamento implementado

- Capacidade específica ValveTestReady para os dois canais de relé.
- Avanço: GPIO32 em LOW, GPIO33 em HIGH.
- Retorno: GPIO32 em HIGH, GPIO33 em LOW.
- Soltar, arrastar para fora ou comando PARAR: ambos em HIGH.
- Ao trocar de direção, o endpoint intercala 20 ms com ambos sem comando.
- A IHM renova o manual a cada 200 ms. Sem renovação válida por 600 ms,
  o controlador desliga o comando, mesmo que os heartbeats continuem chegando.
- Retransmitir o mesmo ID não renova o tempo. Após expiração, exigir liberação
  antes de novo acionamento; a reconexão cancela o manual, sem retomá-lo.
- A IHM destaca somente o canal confirmado pelo controle, não apenas o botão tocado.
- Motor, alimentação automática e permissivos físicos permanecem bloqueados.

Na inversão mais recente solicitada pelo usuário, a configuração passou de
LOW em repouso para HIGH em repouso, com LOW durante o comando.
Confirmar o resultado físico sem cargas.
O intervalo de troca é parâmetro de
teste; não valida tempos mecânicos de relés nem constitui função de segurança.

## Conferir na bancada

1. Manter os contatos sem cargas e verificar UART ONLINE.
2. Engrenagem → senha 1234 → configuração.
3. Segurar VÁLVULA AVANÇO: conferir apenas o canal GPIO32.
4. Soltar: conferir ambos desativados.
5. Segurar VÁLVULA RETORNO: conferir apenas o canal GPIO33.
6. Soltar: conferir ambos desativados.
7. Se o comportamento físico estiver invertido, interromper o teste e identificar
   a polaridade do módulo antes de conectar qualquer carga.

Não foram implementados acionamento pneumático automático ou funções de
segurança para válvulas conectadas. O teste atual atua nos módulos isolados de cargas.

## Validação realizada

- Testes no computador de exclusão dos canais, troca de direção, expiração,
  duplicatas, soltura e bloqueio do motor aprovados.
- Firmwares dos dois ESPs compilados e gravados com verificação por hash.
- Após estabilizar a conexão, a IHM informou LINK=ONLINE,
  TESTE_RELES=DISPONIVEL e MANUAL_CONFIRMADO=0.
- O acionamento físico dos módulos ainda depende de observação pelo usuário.
- Na bancada Linux, HUPCL foi desabilitado em `/dev/ttyUSB0` durante o diagnóstico.
  A [Espressif documenta esse ajuste](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html)
  para evitar resets ligados ao fechamento do monitor USB. A configuração pode
  precisar ser reaplicada após reconectar o adaptador; ela não altera o firmware.
