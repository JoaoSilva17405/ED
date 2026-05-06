# PRD — Correcção de 7 Bugs (Issue #11)

## Problem Statement

O simulador contém sete defeitos que comprometem a correcção das estatísticas, a integridade do estado ao fechar caixas, e a clareza do ecrã para o utilizador:

1. Ao fechar uma caixa, o campo `operador` é limpo, destruindo a informação necessária para as estatísticas finais ("operador que atendeu menos pessoas") e para os ficheiros CSV/histórico — a mesma variável serve dois propósitos incompatíveis.
2. `fechar_caixa_imediata_manual()` usa `contar_caixas_abertas()` que conta caixas em estado `A_FECHAR` como destinos válidos para redistribuição, mas `redistribuir_fila()` só aceita caixas `ABERTA` — uma caixa pode ser fechada imediatamente quando não existe nenhum destino real para os seus clientes.
3. Ainda em `fechar_caixa_imediata_manual()`, os campos `estavaEmAtendimento`, `instanteInicioAtendimento` e `tempoRestanteAtendimento` do cliente em atendimento são alterados antes de saber se o destino existe — se a inserção falhar, a restauração usa `cliente_tempo_atendimento()` em vez do tempo restante original, corrompendo o estado.
4. O menu mostra "Caixa marcada para fecho suave." mesmo quando a caixa estava vazia e foi fechada imediatamente — a mensagem não corresponde ao estado real.
5. `catalog_obter_produtos_aleatorios()` faz clamp de `tempoPassagem` mas não de `preco`, podendo gerar produtos com preço acima de `MAX_PRECO` ou negativo, em violação do enunciado.
6. `mostrar_cliente()` exibe `caixa=0` para clientes ainda na loja (`caixaAtual == -1`), induzindo o utilizador a pensar que o cliente está na caixa 1.
7. O snapshot não guarda os totais globais `totalProdutosOferecidos` e `totalValorOferecido`, nem o `ultimoOperador` de cada caixa — após recarregar, as estatísticas finais e os CSV ficam errados.

## Solution

Corrigir cada defeito cirurgicamente, separando os dois papéis do campo `operador` com um novo campo `ultimoOperador` na `struct Caixa`, tornando `redistribuir_fila` fiável e atómica, corrigindo as mensagens do menu, aplicando clamp de preço no catálogo, e estendendo o formato do snapshot com três novas linhas opcionais.

## User Stories

1. Como simulador, quero que as estatísticas finais mostrem sempre o operador correcto para cada caixa, mesmo depois de essa caixa ter fechado durante a sessão.
2. Como utilizador do menu, quero que "Operador que atendeu menos pessoas" mostre um nome válido e não uma string vazia.
3. Como utilizador do menu, quero que o CSV de estatísticas mostre o operador de cada caixa, mesmo que a caixa esteja fechada no momento da exportação.
4. Como utilizador do menu, quero que o CSV de histórico mostre o operador de cada caixa para cada cliente atendido.
5. Como operador de snapshot, quero que ao recarregar um snapshot os nomes dos operadores das caixas fechadas sejam restaurados, para que as estatísticas da sessão continuem correctas.
6. Como gerente, quero que `fechar_caixa_imediata_manual()` recuse o fecho quando não existe nenhuma outra caixa em estado ABERTA para receber os clientes redistribuídos.
7. Como gerente, quero que `fechar_caixa_imediata_manual()` recuse o fecho se algum cliente da fila não conseguiu ser redistribuído, para que a caixa nunca feche com clientes presos.
8. Como gerente, quero que ao redistribuir o cliente em atendimento, os seus campos de estado sejam restaurados exactamente como estavam caso o destino não exista, para que a simulação não acumule estado inválido.
9. Como utilizador do menu, quero que ao fechar suavemente uma caixa já vazia a mensagem diga "Caixa estava vazia e foi fechada imediatamente.", para saber que o estado real é FECHADA.
10. Como utilizador do menu, quero que ao fechar suavemente uma caixa com clientes a mensagem diga "Caixa marcada para fecho suave.", para saber que a caixa aguarda esvaziar.
11. Como simulador, quero que os produtos gerados aleatoriamente tenham sempre preço no intervalo ]0, MAX_PRECO], em conformidade com o enunciado.
12. Como simulador, quero que os produtos gerados aleatoriamente nunca tenham preço negativo ou zero, para que as ofertas e estatísticas de valor sejam sempre válidas.
13. Como utilizador do menu, quero que ao pesquisar um cliente em loja a informação exibida mostre "(em compras)" em vez de "caixa=0", para não confundir com a caixa 1.
14. Como operador de snapshot, quero que ao recarregar um snapshot os totais globais `totalProdutosOferecidos` e `totalValorOferecido` sejam restaurados, para que as estatísticas finais reflictam toda a história da simulação.
15. Como operador de snapshot, quero que o formato do snapshot continue a ser compatível com ficheiros mais antigos que não contenham as novas linhas, para que snapshots anteriores continuem a carregar sem erros.

## Implementation Decisions

### Módulos a modificar

**`register.h` / `register.c` — struct Caixa**
- Adicionar campo `char ultimoOperador[MAX_OPERADOR]` à `struct Caixa`.
- `caixa_init()` inicializa `ultimoOperador[0] = '\0'`.
- `caixa_listar_historico()` passa a usar `ultimoOperador` no cabeçalho da linha de histórico.

**`store.c` — abertura e fecho de caixas**
- `caixa_atribuir_operador()`: ao atribuir `operador`, copia simultaneamente para `ultimoOperador`.
- `mostrar_estatisticas_finais()`, `guardar_estatisticas_csv()`, `guardar_historico_caixas_csv()`: substituir `sm->caixas[i].operador` por `sm->caixas[i].ultimoOperador` nos locais de saída de estatísticas e CSV.
- `fechar_caixa_imediata_manual()`:
  - Substituir guarda `contar_caixas_abertas(sm) <= 1` por `contar_outras_caixas_abertas(sm, idCaixa) <= 0` (nova função estática que conta apenas `CAIXA_ABERTA`, excluindo a caixa alvo).
  - Salvar `tempoRestanteAntigo`, `inicioAntigo`, `estavaAntigo` do cliente em atendimento antes de qualquer modificação; restaurar todos se `fila_inserir` falhar.
  - `redistribuir_fila()` passa a retornar `int`: devolve 0 se algum cliente não conseguiu ser redistribuído (cliente é reinserido na fila de origem e a função pára), devolve 1 se a redistribuição foi completa.
  - Se `redistribuir_fila()` retornar 0, cancelar o fecho (retornar 0).

**`main.c` — opção 7 (fecho suave)**
- Após chamar `fechar_caixa_suave_manual()`, verificar `sm.caixas[caixa].estado`:
  - Se `CAIXA_FECHADA`: imprimir `"Caixa estava vazia e foi fechada imediatamente.\n"`.
  - Senão: imprimir `"Caixa marcada para fecho suave.\n"`.

**`catalog.c` — `catalog_obter_produtos_aleatorios()`**
- Após o clamp existente de `tempoPassagem`, aplicar clamp de `preco`:
  - Máximo: `cfg->maxPreco`.
  - Mínimo: `0.01f`.

**`customer.c` — `mostrar_cliente()`**
- Quando `caixaAtual >= 0`: mostrar `caixa=N` (comportamento actual).
- Quando `caixaAtual < 0`: mostrar `(em compras)` em vez de `caixa=0`.

**`store.c` — `guardar_snapshot()` / `carregar_formato_novo()`**
- `guardar_snapshot()`:
  - Escrever novas linhas de cabeçalho global: `TOTAL_PRODUTOS_OFERECIDOS n` e `TOTAL_VALOR_OFERECIDO x.xx`.
  - Após cada linha `CAIXA`: escrever `ULTIMO_OPERADOR <idCaixa> <nome>` (sentinel `-` quando vazio).
- `carregar_formato_novo()`:
  - Parsear as novas linhas de cabeçalho e restaurar `sm->totalProdutosOferecidos` e `sm->totalValorOferecido`.
  - Parsear linhas `ULTIMO_OPERADOR` e restaurar `sm->caixas[idx].ultimoOperador`.
  - Linhas desconhecidas são ignoradas (compatibilidade retroactiva).

### Decisões de comportamento

- `ultimoOperador` nunca é limpo após atribuição; representa sempre "o último operador que serviu esta caixa".
- `operador` continua a representar apenas o operador activo; é limpo quando a caixa fecha.
- A guarda de `fechar_caixa_imediata_manual` verifica caixas `ABERTA` (não `A_FECHAR`), porque `redistribuir_fila` e `encontrar_caixa_menos_pessoas_aberta` só usam `ABERTA` como destinos.
- O formato do snapshot com as novas linhas é retrocompatível: ficheiros antigos sem `ULTIMO_OPERADOR` ou `TOTAL_PRODUTOS_OFERECIDOS` carregam sem erro (campos ficam em zero/vazio).

## Testing Decisions

### O que constitui um bom teste

Testar comportamento externo observável através das interfaces públicas:
- Estado dos campos da `struct Caixa` e `Supermercado` após chamadas.
- Valor de retorno das funções públicas.
- Conteúdo de ficheiros gerados (snapshot, CSV) — verificável por leitura directa.

Não testar: nomes de funções estáticas internas, ordem de campos em structs, detalhes de alocação.

### Módulos a testar

| Módulo / Comportamento | O que testar |
|---|---|
| `ultimoOperador` | Após abrir e fechar caixa, `ultimoOperador` mantém o nome; `operador` fica vazio |
| `fechar_caixa_imediata_manual` (bug #2) | Retorna 0 quando única outra caixa está `A_FECHAR`; retorna 1 quando existe caixa `ABERTA` |
| `fechar_caixa_imediata_manual` (bug #3) | Após falha de inserção, `tempoRestanteAtendimento`, `instanteInicioAtendimento` e `estavaEmAtendimento` estão inalterados |
| `redistribuir_fila` | Retorna 0 quando não existe destino; retorna 1 quando todos os clientes são redistribuídos |
| `guardar_snapshot` / `carregar_dados_iniciais` | Após round-trip, `ultimoOperador`, `totalProdutosOferecidos` e `totalValorOferecido` são iguais aos originais |
| Preço no catálogo | Nenhum produto com preço > maxPreco ou <= 0 após `catalog_obter_produtos_aleatorios` |
| `mostrar_cliente` | Cliente com `caixaAtual == -1` não imprime `caixa=0` |

### Arte anterior

Os testes existentes em `tests/` usam a macro `CHECK(cond, desc)` com setup manual de `Supermercado` + `HashClientes`. Seguir o mesmo padrão.

## Out of Scope

- Bug #4 (mensagem dupla ao abrir caixa `A_FECHAR`) — adiado para issue futura.
- Refactoring de `caixa_atribuir_operador` para suportar múltiplos operadores por caixa.
- Persistência do histórico completo de operadores por caixa no snapshot.
- Alteração do formato da linha `CAIXA` no snapshot.

## Further Notes

- A separação `operador` / `ultimoOperador` resolve simultaneamente o bug visual (caixa fechada sem operador visível no estado) e o bug estatístico (operador desaparece das estatísticas) sem duplicar lógica.
- O fix do bug #2 requer que `redistribuir_fila` seja uma função com retorno em vez de void — esta mudança de interface é interna (função estática), mas implica actualizar todos os locais de chamada dentro de `store.c`.
- O fix do bug #3 (save/restore) é independente do fix do bug #2 e pode ser aplicado separadamente.
- Os totais globais no snapshot (`TOTAL_PRODUTOS_OFERECIDOS`, `TOTAL_VALOR_OFERECIDO`) complementam o fix da issue #10 (que recomputava apenas os campos por-cliente) — juntos garantem que tanto os clientes presentes como os já atendidos contribuem correctamente para as estatísticas após reload.
