# PRD — Política Automática de Abertura e Fecho de Caixas

**Data:** 2026-05-05
**Prazo de entrega:** 2026-06-01 (Época Normal)
**Linguagem:** C (C11)
**Estado:** Draft

---

## Problem Statement

O sistema de simulação do supermercado não implementa gestão dinâmica da capacidade de atendimento. Atualmente, as caixas só abrem ou fecham por ação manual explícita do gerente, sem qualquer reação automática à carga real das filas. Isto significa que:

- Quando as filas crescem acima de um limite razoável (`MAX_FILA`), o sistema não abre automaticamente uma nova caixa — os clientes acumulam-se sem resposta;
- Quando as filas ficam quase vazias e existem caixas abertas em excesso, o sistema não fecha automaticamente a caixa menos ocupada — desperdiçam-se recursos;
- O gerente é obrigado a monitorizar manualmente o estado das filas e a tomar todas as decisões de capacidade, tornando a simulação pouco realista.

---

## Solution

Implementar uma **política automática de abertura e fecho de caixas** baseada no número médio de clientes por fila, com as seguintes regras:

**Abertura automática:** quando o número médio de clientes nas filas das caixas abertas ultrapassa `MAX_FILA`, abrir a caixa fechada de menor ID disponível (uma por verificação).

**Fecho automático:** quando o número médio de clientes nas filas das caixas abertas fica abaixo de `MIN_FILA`, iniciar o fecho suave (`CAIXA_A_FECHAR`) da caixa aberta com menor fila (a mais vazia), permitindo-lhe atender os clientes em espera antes de fechar definitivamente.

A política coexiste com o controlo manual do gerente. A abertura manual pelo gerente ativa um período de proteção de 3 instantes durante o qual o fecho automático é suspenso.

---

## User Stories

1. Como sistema de simulação, quero abrir automaticamente uma nova caixa quando a média de clientes por fila ultrapassa `MAX_FILA`, para que os clientes não se acumulem indefinidamente.
2. Como sistema de simulação, quero fechar suavemente a caixa menos ocupada quando a média de clientes por fila fica abaixo de `MIN_FILA`, para não desperdiçar recursos com caixas desnecessárias.
3. Como sistema de simulação, quero que a caixa a fechar sirva todos os clientes em espera antes de fechar definitivamente, para que nenhum cliente seja prejudicado pelo fecho automático.
4. Como sistema de simulação, quero garantir que pelo menos uma caixa permanece sempre aberta, para que o supermercado nunca fique sem capacidade de atendimento.
5. Como sistema de simulação, quero verificar a política após cada passo de simulação, após inserir um cliente, após mover um cliente, e após concluir o atendimento de um cliente, para que a política reaja sempre ao estado mais recente.
6. Como sistema de simulação, quero que apenas uma caixa seja aberta por verificação da política, para evitar abrir demasiadas caixas de uma vez antes de reavaliar a carga.
7. Como sistema de simulação, quero que o fecho automático selecione a caixa aberta com menor `fila.tamanho` (em caso de empate, a de menor ID), para minimizar o impacto sobre os clientes.
8. Como sistema de simulação, quero que a abertura automática selecione a caixa fechada de menor ID, para que a escolha seja determinista e previsível.
9. Como gerente do supermercado, quero poder abrir manualmente uma caixa a qualquer momento, e que essa decisão seja respeitada pelo sistema durante pelo menos 3 instantes, para que a minha intenção não seja imediatamente anulada pela política automática.
10. Como gerente do supermercado, quero ver no log todas as ações automáticas da política (abertura e fecho), com a média atual e os thresholds, para poder auditar o comportamento do sistema.
11. Como sistema de simulação, quero evitar ciclos de abertura/fecho no mesmo instante, bloqueando a abertura automática no mesmo instante em que ocorreu um fecho automático, para estabilizar o sistema.
12. Como sistema de simulação, quero que a média seja calculada apenas sobre caixas em estado `CAIXA_ABERTA` (excluindo `CAIXA_A_FECHAR`), para que caixas já em processo de fecho não distorçam a avaliação da carga.
13. Como sistema de simulação, quero que a média seja calculada usando apenas `fila.tamanho` (excluindo o cliente em atendimento), para refletir apenas os clientes que ainda aguardam.
14. Como sistema de simulação, quero que as condições de abertura e fecho sejam estritamente `> MAX_FILA` e `< MIN_FILA` respetivamente, para que o intervalo `[MIN_FILA, MAX_FILA]` seja uma zona estável onde nada acontece.
15. Como sistema de simulação, quando não existem caixas fechadas disponíveis para abrir e a média ultrapassa `MAX_FILA`, quero ignorar silenciosamente, para não gerar erros em situações de capacidade máxima.
16. Como sistema de simulação, quero verificar a política após a conclusão do fecho definitivo de uma caixa em `CAIXA_A_FECHAR`, para que o sistema possa reagir à redução de capacidade resultante.

---

## Implementation Decisions

### Novos campos em `Supermercado`
- `instanteUltimoFecho` (int): regista o instante em que a política automática efetuou o último fecho. Bloqueia abertura automática se `instanteAtual == instanteUltimoFecho`.
- `instanteUltimaAberturaManual` (int): regista o instante da última abertura manual pelo gerente. Bloqueia fecho automático se `instanteAtual <= instanteUltimaAberturaManual + 3`. Apenas abertura manual atualiza este campo — abertura automática não o altera.

### Cálculo da média
- Numerador: soma de `fila.tamanho` de todas as caixas em estado `CAIXA_ABERTA`.
- Denominador: contagem de caixas em estado `CAIXA_ABERTA`.
- Caixas em `CAIXA_A_FECHAR` e `CAIXA_FECHADA` excluídas de ambos.
- Cliente `emAtendimento` não conta para a média.

### Lógica de abertura automática
- Condição: `media > MAX_FILA` (estritamente) e `instanteAtual != instanteUltimoFecho`.
- Ação: mudar o estado da primeira caixa `CAIXA_FECHADA` (menor ID) para `CAIXA_ABERTA`.
- Limite: uma única abertura por chamada a `verificar_politica_caixas()`.
- Sem caixas disponíveis: ignorar silenciosamente.

### Lógica de fecho automático
- Condição: `media < MIN_FILA` (estritamente), número de caixas `CAIXA_ABERTA` > 1, e `instanteAtual > instanteUltimaAberturaManual + 3`.
- Seleção: caixa `CAIXA_ABERTA` com menor `fila.tamanho`; em empate, menor ID.
- Tipo de fecho: sempre soft-close (`CAIXA_A_FECHAR`) — a caixa serve os clientes em espera antes de fechar; atualiza `instanteUltimoFecho = instanteAtual`.
- Exceção: nunca fechar se só restar 1 caixa `CAIXA_ABERTA`.

### Pontos de verificação da política
- Após `inserir_novo_cliente()`
- Após `mover_cliente_caixa()`
- A cada passo de `avancar_simulacao()`
- **Novo:** após `concluir_atendimento()` (incluindo quando uma caixa em `CAIXA_A_FECHAR` transita para `CAIXA_FECHADA`)

### Abertura manual pelo gerente (opção 6)
- Ao abrir manualmente, atualizar `instanteUltimaAberturaManual = instanteAtual`.
- Verificar a política após a abertura (pode abrir mais se necessário, mas o fecho fica bloqueado por 3 instantes).

### Logging
- Registar no `logFile` com formato: `[AUTO] Caixa N aberta (media=X.X > MAX_FILA=Y)` e `[AUTO] Caixa N a fechar (media=X.X < MIN_FILA=Y)`.

---

## Testing Decisions

**O que constitui um bom teste:**
- Testar comportamento externo (estado das caixas, valores retornados), não detalhes de implementação.
- Criar um `Supermercado` com configuração controlada (valores pequenos de MAX_FILA e MIN_FILA) e verificar que os estados das caixas mudam corretamente.

**Módulos a testar:**
- `verificar_politica_caixas()`: cenários de abertura, fecho, zona estável, bloqueio anti-oscilação, proteção pós-abertura manual.
- `concluir_atendimento()`: verificar que a política é chamada após fecho definitivo de `CAIXA_A_FECHAR`.

**Casos de teste prioritários:**
- Media acima de MAX_FILA → 1 caixa abre.
- Media abaixo de MIN_FILA → caixa menos ocupada inicia soft-close.
- Fecho no instante T → abertura bloqueada no mesmo instante T.
- Abertura manual → fecho bloqueado por 3 instantes.
- Só 1 caixa aberta → política de fecho não atua.
- Todas as caixas abertas → política de abertura ignora silenciosamente.

---

## Out of Scope

- Implementação de histerese multi-instante para abertura automática (a proteção de 1 instante via `instanteUltimoFecho` é suficiente).
- Redistribuição imediata de clientes ao fechar automaticamente (o soft-close serve os clientes in situ).
- Configuração dinâmica de MAX_FILA e MIN_FILA em runtime (valores lidos de `Configuracao.txt` no arranque).
- Fecho imediato automático (apenas soft-close é usado pela política automática).
- Proteção por caixa individual após abertura manual (a proteção é global).

---

## Further Notes

- Os valores por defeito `MAX_FILA=7` e `MIN_FILA=3` já existem em `Configuracao.txt` e na struct `Configuracao`.
- A struct `Configuracao` já contém os campos `maxFila` e `minFila` — não é necessário alterar o parser de configuração.
- A função `verificar_politica_caixas()` já existe em `store.c` com lógica parcial; a implementação desta PRD consiste em corrigir e completar essa função e adicionar os dois novos campos ao `Supermercado`.
- O campo `instanteUltimoFecho` deve ser inicializado a `-1` para não bloquear a abertura no instante 0.
- O campo `instanteUltimaAberturaManual` deve ser inicializado a `-4` (ou qualquer valor tal que `0 > -4 + 3` seja falso... na verdade `-4` faz `0 > -4+3 = -1` → verdadeiro, então deve ser `-1` mas o check é `instanteAtual <= instanteUltimaAberturaManual + 3`, então inicializar a `-4` garante que no instante 0, `0 <= -4+3 = -1` é falso — correto).
