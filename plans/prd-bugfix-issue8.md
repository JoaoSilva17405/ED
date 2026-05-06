# PRD: Correção de Bugs e Display (Issue 8)

## Problem Statement

Após a implementação do ciclo de vida completo do cliente (Issue 7), foram identificados 5 bugs e 1 problema de display na simulação do supermercado:

1. **Bug 1 — MAX_ESPERA não verificado para clientes em atendimento**: O loop de verificação de `maxEspera` em `avancar_simulacao` itera apenas pelos clientes na fila. Um cliente que entra em atendimento já ultrapassando `MAX_ESPERA` (ou que ultrapassa durante o atendimento) nunca recebe o produto oferecido.

2. **Bug 2 — CAIXA_A_FECHAR fica presa quando fila já está vazia**: Quando a política automática marca uma caixa como `CAIXA_A_FECHAR`, não verifica se a fila já está vazia e não há cliente em atendimento. A caixa fica indefinidamente em estado `CAIXA_A_FECHAR` em vez de fechar de imediato.

3. **Bug 3 — Operador não restaurado ao carregar snapshot**: O snapshot guarda o nome do operador de cada caixa, mas `carregar_formato_novo` lê-o para uma variável local sem o copiar para `sm->caixas[caixaAtual].operador`. Ao recarregar, o operador é re-sorteado aleatoriamente de `funcionarios.txt`.

4. **Bug 4 — Cliente em atendimento carregado do snapshot não entra no hash**: Em `finalizar_cliente_pendente`, quando `pEmAtendimento=true`, o cliente é colocado em `sm->caixas[caixaAtual].emAtendimento` mas `hash_inserir` não é chamado. Pesquisas por esse cliente falham após o carregamento.

5. **Bug 5 — `memoria_utilizada` conta `HashClientes` duas vezes e ignora `clientesEmLoja`**:
   - `hash_memoria()` já inclui `sizeof(HashClientes)` internamente; `memoria_utilizada` adiciona `sizeof(HashClientes)` antes de chamar `hash_memoria`, duplicando esse valor.
   - A array `clientesEmLoja` (ponteiro, clientes e produtos) não é contada.

6. **Display — `mostrar_caixa` não mostra detalhes do cliente em atendimento**: Apenas imprime `id` e `tempo_restante`; deveria chamar `mostrar_cliente()` para exibir nome, produtos e demais detalhes, tal como é feito para os clientes na fila.

Adicionalmente, `run_tests.sh` tem padrões de grep desatualizados face às mensagens alteradas na opção 4 e no fluxo de inserção.

---

## Solution

Corrigir cada bug de forma cirúrgica nos ficheiros relevantes, acrescentar testes automáticos para cada correção e atualizar `run_tests.sh`.

---

## User Stories

1. Como operador, quero que um cliente que ultrapasse `MAX_ESPERA` enquanto está em atendimento receba um produto oferecido, de modo a que a política de compensação seja sempre respeitada.
2. Como operador, quero que uma caixa marcada para fecho suave feche imediatamente se já não tiver fila nem cliente em atendimento, de modo a evitar estados inconsistentes.
3. Como operador, quero que ao recarregar um snapshot o nome do operador de cada caixa seja restaurado corretamente, de modo a que a continuidade da simulação seja fiel ao estado gravado.
4. Como operador, quero que ao pesquisar um cliente que estava em atendimento quando o snapshot foi gravado o sistema o encontre corretamente, de modo a que a hash seja consistente após o carregamento.
5. Como desenvolvedor, quero que `memoria_utilizada` reporte um valor correto sem duplicações e incluindo todos os objetos alocados, de modo a que a memória reportada reflita a realidade.
6. Como operador, quero ver os detalhes completos (nome, produtos, preços) do cliente em atendimento ao mostrar o estado das caixas, de modo a ter visibilidade total sobre o que está a acontecer.

---

## Implementation Decisions

### Bug 1 — MAX_ESPERA check para `emAtendimento`

**Ficheiro:** `src/store.c`, função `avancar_simulacao`

Após o loop existente que itera pelos clientes em fila (linhas 776–795), adicionar um segundo loop que itera por `sm->caixas[i].emAtendimento`:

```c
for (i = 0; i < sm->cfg.nCaixas; ++i) {
    Cliente *c = sm->caixas[i].emAtendimento;
    if (c && !c->oferecimentoFeito) {
        int espera = sm->instanteAtual - c->instanteEntradaFila;
        if (espera > sm->cfg.maxEspera) {
            float valor = oferecer_um_produto(c);
            if (valor > 0.0f) { ... log ... }
            c->oferecimentoFeito = true;
        }
    }
}
```

Nota: `instanteEntradaFila` é o instante em que o cliente entrou na fila — a espera total inclui o tempo que passou na fila antes de entrar em atendimento.

---

### Bug 2 — Fechar imediatamente se já vazio

**Ficheiro:** `src/store.c`, função `verificar_politica_caixas`

Após `sm->caixas[idx].estado = CAIXA_A_FECHAR` (linha 831), adicionar verificação imediata:

```c
if (sm->caixas[idx].fila.tamanho == 0 && sm->caixas[idx].emAtendimento == NULL) {
    sm->caixas[idx].estado = CAIXA_FECHADA;
    log_acao(..., "FECHAR_CAIXA_FECHADA_AUTO", ...);
}
```

---

### Bug 3 — Restaurar operador do snapshot

**Ficheiro:** `src/store.c`, função `carregar_formato_novo`

No bloco `else if (strncmp(p, "CAIXA", 5) == 0)` (linhas 442–455), após definir o estado da caixa, adicionar:

```c
if (sscanf devolve >= 3 e caixaAtual válido)
    strncpy(sm->caixas[caixaAtual].operador, operador, MAX_OPERADOR - 1);
    sm->caixas[caixaAtual].operador[MAX_OPERADOR - 1] = '\0';
```

---

### Bug 4 — `hash_inserir` para cliente em atendimento

**Ficheiro:** `src/store.c`, função `finalizar_cliente_pendente`

No ramo `if (pEmAtendimento)` (linha 356), após atribuir `sm->caixas[caixaAtual].emAtendimento = cliente`, adicionar:

```c
if (hash) hash_inserir(hash, cliente, caixaAtual);
```

---

### Bug 5A — Remover double-count de `HashClientes`

**Ficheiro:** `src/store.c`, função `memoria_utilizada`

Remover a linha:
```c
total += sizeof(HashClientes);   // linha 941 — já incluído em hash_memoria()
```

---

### Bug 5B — Contar `clientesEmLoja`

**Ficheiro:** `src/store.c`, função `memoria_utilizada`

Adicionar após o loop das caixas:

```c
total += sizeof(Cliente *) * sm->capClientesEmLoja;
for (i = 0; i < sm->nClientesEmLoja; ++i) {
    total += sizeof(Cliente);
    total += sizeof(Produto) * sm->clientesEmLoja[i]->nProdutos;
}
```

---

### Display — `mostrar_caixa` com detalhes completos

**Ficheiro:** `src/store.c`, função `mostrar_caixa`

Substituir o `printf` simplificado por uma chamada a `mostrar_cliente`:

```c
if (caixa->emAtendimento) {
    printf("  Em atendimento (tempo_restante=%d):\n", caixa->tempoRestanteAtendimento);
    mostrar_cliente(caixa->emAtendimento);
}
```

---

### `run_tests.sh` — Atualizar padrões

**Ficheiro:** `run_tests.sh`

Atualizar os padrões de grep que verificavam mensagens da opção 4 (inserção manual com seleção de produtos) para corresponder ao novo fluxo automático ("a fazer compras com N produtos").

---

## Testing Decisions

**Princípio:** testar comportamento observável através de interfaces públicas — estado das structs após chamadas de função — sem depender de detalhes internos.

**Novo ficheiro de testes:** `tests/test_issue8_bugfixes.c`

### Ciclos de teste a implementar

| Ciclo | Comportamento a verificar |
|-------|--------------------------|
| 1 | Após avançar além de `maxEspera` com cliente em atendimento, `oferecimentoFeito=true` e `totalProdutosOferecidos` incrementado |
| 2 | `verificar_politica_caixas` numa caixa já vazia coloca estado `CAIXA_FECHADA`, não `CAIXA_A_FECHAR` |
| 3 | Roundtrip snapshot: operador da caixa 0 restaurado com o nome correto |
| 4 | Roundtrip snapshot com cliente em atendimento: `hash_pesquisar` encontra o cliente após carregar |
| 5A | `memoria_utilizada` sem clientes em loja == `sizeof(Supermercado) + caixas + hash_memoria` (sem duplicação) |
| 5B | `memoria_utilizada` com 1 cliente em loja reflete tamanho extra do cliente e seus produtos |
| 6 | `mostrar_caixa` com cliente em atendimento exibe nome do cliente na saída (via captura de stdout ou verificação de estado) |

**Prior art:** `tests/test_issue5_snapshot.c` (roundtrip via `guardar_snapshot` / `carregar_dados_iniciais`), `tests/test_issue7_ciclo_vida.c` (setup com `make_sm`/`hash_init`).

---

## Out of Scope

- Refactoring da função `avancar_simulacao`
- Alterações ao formato do snapshot além do campo `operador`
- Testes de performance ou stress
- Alterações ao menu ou à UI além do já descrito para `mostrar_caixa`

---

## Further Notes

- `hash_memoria` (linha 73 de `hash.c`) já inclui `sizeof(HashClientes)` — confirmar antes de remover a linha duplicada em `memoria_utilizada`.
- A verificação de `MAX_ESPERA` para `emAtendimento` usa `instanteEntradaFila`, que pode ser `-1` para clientes carregados de snapshot antigos; guardar com proteção `if (c->instanteEntradaFila >= 0)`.
- O campo `MAX_ESPERA` em `Configuracao.txt` mantém-se inalterado; a correção é puramente no código C.
