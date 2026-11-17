# IHM de teste — SC01 Plus

Firmware para Panlee ZX3D50CE08S-V16-USRC (SC01 Plus), ESP32-S3,
flash de 16 MB e PSRAM QSPI de 2 MB, com controlador operacional simulado
na própria tela. Não é necessário conectar o ESP32 de controle.

## Funções

- Comprimento em mm com até três casas decimais.
- Quantidade total do lote na principal.
- Engrenagem no canto superior direito abre acesso com senha `1234`.
- Tela de configuração com comandos manuais do motor e da válvula, mais
  tempos de avanço/retorno da válvula em milissegundos.
- Botões manuais de avanço/retorno do motor também na tela principal,
  no lugar dos antigos campos de tempo.
- Teclado ao tocar nos parâmetros; SALVAR valida e aplica.
- INICIAR, PARAR após a peça e contagem do lote.
- FALHA injeta abertura de porta simulada durante o ciclo.
- REARMAR, CONFIRMAR e INICIAR são ações separadas para recuperação.
- NOVO LOTE zera a contagem e inicia outro lote com os parâmetros atuais.

Valores editados durante uma peça ficam pendentes para a próxima. A simulação
continua enquanto o teclado está aberto. A velocidade simulada é fixa em
100 mm/s. Não há armazenamento de parâmetros/contagem nesta etapa.

Os comandos manuais são simulados, exclusivos e momentâneos: segurar o botão
ativa a indicação; soltar ou arrastar para fora encerra. Só ficam disponíveis
em PRONTO ou LOTE CONCLUÍDO. Nenhum desses comandos altera a contagem.
Sair da configuração bloqueia novamente o acesso; a senha é mascarada e
o retorno do editor de tempos mantém a tela de configuração aberta.

## Compilar e gravar

Na raiz do repositório, com PlatformIO instalado:

```sh
pio run -d firmware/ihm-esp32s3
```

O binário de aplicação é gerado em `.pio/build/ihm-simulada/firmware.bin`.
A gravação substitui o programa atual da IHM: preservar uma cópia antes.
Usar o alvo upload para gravar o conjunto completo nos endereços corretos:

```sh
pio run -d firmware/ihm-esp32s3 -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 --baud 115200
```

As portas USB podem mudar após reinicialização.

## Roteiro de teste na tela

1. Confirmar tela em paisagem 480×320 com indicação SIMULACAO e engrenagem.
2. Editar comprimento e quantidade na principal; os tempos ficam na configuração.
3. Iniciar lote de três peças e observar alimentação, corte, retorno e contagem.
4. Durante alimentação, alterar comprimento; o valor vale na peça seguinte.
5. Tocar PARAR e conferir parada após concluir a peça; INICIAR retoma.
6. Durante um ciclo, tocar FALHA; conferir bloqueio e manutenção da contagem.
7. Tocar REARMAR e CONFIRMAR; conferir que só INICIAR retoma o lote.
8. Conferir que quantidade/tempos zero são rejeitados; comprimento aceita 0.001 mm.
9. Tocar na engrenagem: senha incorreta deve manter o bloqueio; `1234` abre a configuração.
10. Editar os dois tempos e conferir retorno à configuração; VOLTAR leva à principal.
11. Segurar cada comando manual com ciclo parado; soltar e arrastar para fora devem
    encerrar o comando. Durante produção ou falha nenhum manual pode iniciar.
12. Sair e tocar na engrenagem novamente: deve solicitar a senha outra vez.

Nenhuma saída de motor, válvula, porta ou RS-485 é acionada. A tela usa GPIOs
somente para display/touch/backlight, conforme o driver SC01 Plus do fabricante.
Tempo de retorno não é confirmação física de posição da navalha.

## Referências

- [Configuração do fabricante](https://github.com/smartpanle/PanelLan_esp32_arduino/tree/2bc0d086c9a55f5288d85da6c16968c22744fe5f/src/board/sc01_plus)
- PanelLan no commit `2bc0d086c9a55f5288d85da6c16968c22744fe5f`.
- LovyanGFX 1.2.7; plataforma Espressif32 6.10.0 / Arduino.

O perfil do fabricante foi conferido com o datasheet da família. Tela,
orientação, touch e PSRAM na revisão física V16 exigem teste na placa.

## Validação realizada

- Compilação para ESP32-S3 concluída pelo PlatformIO.
- Testes no computador do núcleo e da conversão/validação dos valores aprovados.
- Gravação em `/dev/ttyACM0`, MAC `48:27:e2:1f:29:b0`, com hashes verificados.
- Firmware respondeu ao comando serial `?` com `IHM_SIM_READY`, estado PRONTO,
  tela configurada 480×320, PSRAM aproximadamente 2 MB e flash 16 MB.
- Imagem física, orientação e resposta do toque aguardam conferência pelo operador.
- Atualização UI=2 gravada: engrenagem, senha, configuração e comandos manuais.
  Inicialização confirmada por serial em PRONTO, página principal e manual parado.
- Três suítes no computador aprovadas, incluindo senha incorreta, bloqueio de
  tempos, retorno do editor, novo acesso e exclusão dos comandos manuais.

Antes da gravação foi lida uma cópia integral de 16 MB do programa anterior:
`backups/ihm-original-4827e21f29b0.bin`, na raiz do repositório. O arquivo
SHA-256 correspondente está ao lado. A pasta de backups é ignorada pelo Git.
Não publicar essa cópia: ela pode conter configurações do programa anterior.
