# Relatório técnico de desenvolvimento

## Máquina de corte de vira

**Data de atualização:** 03/11/2026  
**Versão:** 1.1 — Documento de concepção  
**Situação:** Em desenvolvimento

---

## 1. Resumo do projeto

O projeto prevê o desenvolvimento de uma máquina para alimentar, medir e cortar fita conhecida como vira. O avanço será realizado por rolos acionados por motor de passo, e o corte será executado por uma navalha acionada pneumaticamente.

A proposta busca oferecer controle do comprimento, repetibilidade entre peças e operação acompanhada por uma interface homem-máquina (IHM). As especificações de desempenho e os detalhes construtivos ainda serão definidos.

## 2. Identificação e aplicação

- **Nome:** Máquina de Corte de Vira
- **Categoria:** Automação industrial
- **Aplicação prevista:** Alimentação, medição e corte automatizado de vira

## 3. Objetivo

Desenvolver uma máquina que avance a vira até o comprimento configurado e execute o corte de forma controlada. O projeto deverá definir os requisitos mecânicos, elétricos, pneumáticos e de controle necessários para atender ao material e à produção pretendida.

## 4. Conceito de funcionamento

O ciclo de operação previsto é:

1. O operador configura o comprimento e, quando aplicável, a quantidade de peças na IHM.
2. O sistema de rolos traciona e conduz a vira pelas guias.
3. O motor de passo avança o material conforme o comprimento configurado.
4. Ao final do avanço, o sistema interrompe a alimentação e comanda o corte.
5. O cilindro pneumático movimenta a navalha para cortar o material e retorna à posição inicial.
6. O controle atualiza a contagem e, se houver peças restantes, prepara o próximo ciclo.

A sequência final, as condições para iniciar cada etapa e o comportamento em caso de falha deverão ser definidos durante o detalhamento do controle.

## 5. Conjunto previsto

### Alimentação e guiamento

- Motor de passo e respectivo acionamento.
- Rolos de tração e pressão.
- Guias para condução e alinhamento da vira.

### Corte e estrutura

- Cilindro pneumático e componentes de preparação e controle do ar.
- Navalha de corte.
- Estrutura mecânica de sustentação.
- Proteção física amarela com painéis transparentes, conforme detalhamento do projeto.

### Operação e controle

- IHM para configuração e acompanhamento do processo.
- Comandos de início e parada.
- Dispositivo de parada de emergência.
- Sistema de controle eletrônico e pneumático.

A seleção e o dimensionamento dos componentes dependem das características do material, da precisão e da capacidade produtiva desejadas.

## 6. Operação prevista

A IHM deverá permitir, no mínimo:

- Configurar o comprimento de corte.
- Configurar a quantidade de peças, se aplicável ao processo.
- Iniciar e parar a produção.
- Consultar a quantidade produzida e o estado operacional.

Alarmes, permissões de operação, ajustes de processo e recuperação após interrupções serão definidos na especificação funcional.

## 7. Segurança

O projeto deverá incluir apreciação de riscos e considerar os requisitos aplicáveis à máquina, incluindo a NR-12. A análise deverá orientar a proteção contra acesso às partes móveis e à região de corte, os dispositivos de comando e parada, os intertravamentos necessários e as medidas relativas aos sistemas elétrico e pneumático.

As funções de segurança e sua validação deverão ser definidas por profissionais tecnicamente habilitados, com base na apreciação de riscos e na configuração final do equipamento. A descrição deste documento é conceitual e não substitui o projeto de segurança.

## 8. Requisitos a detalhar

Antes do dimensionamento e da construção, é necessário definir:

- Material, largura, espessura e demais características da vira.
- Faixa de comprimentos de corte e tolerância dimensional requerida.
- Capacidade produtiva pretendida e regime de operação.
- Método de medição e calibração do avanço do material.
- Requisitos de alimentação, troca de rolos e ajuste das guias.
- Pressão e disponibilidade de ar comprimido.
- Arquitetura de controle, componentes elétricos e lógica de operação.
- Requisitos de instalação, limpeza e manutenção.
- Estimativa de custos e cronograma de desenvolvimento.

## 9. Próximas etapas

1. Levantar as características do material e os requisitos de produção.
2. Definir a faixa de operação e os critérios de aceitação, incluindo a tolerância de corte.
3. Elaborar o conceito mecânico e selecionar preliminarmente os componentes.
4. Desenvolver a especificação funcional do controle e da IHM.
5. Realizar a apreciação de riscos e detalhar as medidas de segurança.
6. Preparar o orçamento e o plano de validação do protótipo.

---

*Documento de concepção sujeito a revisão conforme os requisitos forem definidos.*
