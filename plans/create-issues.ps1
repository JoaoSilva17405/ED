$repo = "JoaoSilva17405/ED"

# ── Issue 0: PRD (parent) ─────────────────────────────────────────────────────
$prd = gh issue create --repo $repo `
  --title "PRD: Gestao de Caixas de Supermercado - Compra Aqui Lda." `
  --label "documentation" `
  --body @'
## Problem Statement

O sistema atual gera clientes com IDs fictícios (`P1`, `P21`, ...), produtos com nomes e precos aleatórios de uma lista hardcoded de 15 nomes, e atribui operadores com nomes genéricos (`Operador_1`). Foram fornecidos três ficheiros de dados reais que devem substituir toda esta geração sintética:

- **Clientes.txt** — registo real de clientes: `ID (6 dígitos) TAB Nome completo`
- **Produtos.txt** — catálogo real de produtos: `ID TAB Nome TAB Preco TAB Stock TAB tempoPassagem`
- **funcionarios.txt** — lista real de operadores de caixa: `ID (4 dígitos) TAB Nome completo`

Para além disso, foram identificados dois bugs críticos na simulação e várias inconsistências menores que comprometem a correção das estatísticas finais.

## Solution

Integrar os três ficheiros de dados reais como fonte primária de clientes, produtos e operadores. Criar três novos módulos (`catalog`, `employee`, `client_registry`). Corrigir os cinco bugs identificados. Manter modularidade e conformidade total com os requisitos do enunciado.

## User Stories

1. Como gerente, quero que os clientes tenham IDs e nomes reais de `Clientes.txt`
2. Como gerente, quero que os produtos sejam do catálogo real de `Produtos.txt`
3. Como gerente, quero que cada caixa tenha operador real de `funcionarios.txt`
4. Como gerente, quero que o cliente receba exatamente UM produto oferecido ao ultrapassar MAX_ESPERA
5. Como gerente, quero que MIN_FILA nunca feche a última caixa verdadeiramente aberta
6. Como gerente, quero que "produtos vendidos" não inclua produtos oferecidos gratuitamente
7. Como gerente, quero pesquisar cliente pelo ID real de 6 dígitos e ver nome completo
8. Como gerente, quero que o cálculo de memória inclua todos os buffers carregados
9. Como utilizador, quero que avançar simulação rejeite valores negativos ou zero
10. Como avaliador, quero comentários Doxygen em todos os módulos públicos
11. Como gerente, quero ver nome completo do operador com menos atendimentos
12. Como gerente, quero listar clientes atendidos por uma caixa específica
13. Como gerente, quero CSV com colunas separadas para vendidos vs oferecidos
14. Como gerente, quero que memória desperdiçada inclua buckets vazios do hash
15. Como avaliador, quero IDs com zeros iniciais preservados como strings

## Bugs Identificados

| ID | Severidade | Descrição |
|----|-----------|-----------|
| BUG-1 | CRÍTICO | `oferecer_um_produto` chamado em cada instante após MAX_ESPERA |
| BUG-2 | CRÍTICO | Política MIN_FILA pode fechar a última caixa aberta |
| BUG-3 | MÉDIO | `totalProdutosVendidos` inclui produtos oferecidos |
| BUG-4 | MÉDIO | Avançar simulação com passos <= 0 não produz erro |
| BUG-5 | BAIXO | `sizeof(HashClientes)` ausente de `memoria_utilizada` |

## Prazo

**2026-06-01** (Época Normal). Registo do grupo no Moodle até **2026-05-07**.
'@

$prdNum = ($prd -split '/')[-1]
Write-Host "PRD criado: #$prdNum"

# ── Issue 1: Bug Fixes ────────────────────────────────────────────────────────
$i1 = gh issue create --repo $repo `
  --title "Fix: Corrigir bugs críticos e inconsistências de simulação" `
  --label "bug" `
  --body @"
## Parent PRD

#$prdNum

## What to build

Corrigir os cinco bugs identificados no motor de simulação e nas estatísticas. Cada fix é uma alteração cirúrgica no módulo correspondente sem quebrar os restantes.

**BUG-1 (CRÍTICO):** `oferecer_um_produto` é chamado a cada instante após `MAX_ESPERA`, oferecendo N produtos em vez de 1. Adicionar campo `oferecimentoFeito` (bool) à struct `Cliente` e verificar o flag antes de oferecer.

**BUG-2 (CRÍTICO):** `contar_caixas_abertas` inclui `CAIXA_A_FECHAR`, permitindo que a política `MIN_FILA` feche a última caixa verdadeiramente aberta. Criar `contar_caixas_realmente_abertas` que conta apenas `CAIXA_ABERTA`.

**BUG-3 (MÉDIO):** `totalProdutosVendidos` conta produtos oferecidos gratuitamente (inconsistente com `totalValorVendido`). Subtrair `produtosOferecidos` de `nProdutos` em `concluir_atendimento`.

**BUG-4 (MÉDIO):** Avançar simulação com `passos <= 0` não produz erro nem aviso. Validar e rejeitar com mensagem antes de chamar `avancar_simulacao`.

**BUG-5 (BAIXO):** `sizeof(HashClientes)` ausente do cálculo de `memoria_utilizada`. Adicionar ao total.

## Acceptance criteria

- [ ] Cliente que espera MAX_ESPERA+10 instantes recebe exatamente 1 produto oferecido
- [ ] Sistema com 1 CAIXA_ABERTA + 1 CAIXA_A_FECHAR não fecha a última aberta por política MIN_FILA
- [ ] totalProdutosVendidos == nProdutos - produtosOferecidos após atendimento
- [ ] Avançar 0 ou -1 passos imprime mensagem de erro e não altera o estado
- [ ] memoria_utilizada inclui sizeof(HashClientes)
- [ ] Todos os testes existentes em run_tests.sh continuam a passar

## Blocked by

None - can start immediately

## User stories addressed

- User story 4
- User story 5
- User story 6
- User story 8
- User story 9
- User story 14
"@

$i1Num = ($i1 -split '/')[-1]
Write-Host "Issue 1 criada: #$i1Num"

# ── Issue 2: Módulo employee ──────────────────────────────────────────────────
$i2 = gh issue create --repo $repo `
  --title "Feature: Módulo employee - operadores reais de funcionarios.txt" `
  --label "enhancement" `
  --body @"
## Parent PRD

#$prdNum

## What to build

Criar o módulo `employee` (employee.h / employee.c) que carrega a lista de funcionários de `funcionarios.txt` e os atribui como operadores reais às caixas, substituindo os nomes genéricos `Operador_N`.

Formato do ficheiro: `ID (4 dígitos) TAB Nome completo`
Exemplo: `1670	Ana Francisca Lemos Martins`

O módulo expõe:
- Carregamento da lista em array dinâmico
- Seleção aleatória para atribuição a cada caixa em `supermercado_init`
- Libertação de memória

A struct `Caixa` recebe campo `idFuncionario` (int). A função `caixa_init` passa a receber um `Funcionario*` em vez de string gerada. As estatísticas finais passam a mostrar o nome real do operador com menos atendimentos.

## Acceptance criteria

- [ ] `funcionarios.txt` é carregado no arranque sem erros
- [ ] Cada caixa mostra o nome real do operador (não `Operador_N`)
- [ ] A estatística "operador com menos atendimentos" apresenta nome completo real
- [ ] Memória do módulo é corretamente libertada na saída
- [ ] Compila sem warnings com `-Wall -Wextra -std=c11`

## Blocked by

None - can start immediately

## User stories addressed

- User story 3
- User story 11
"@

$i2Num = ($i2 -split '/')[-1]
Write-Host "Issue 2 criada: #$i2Num"

# ── Issue 3: Módulo catalog ───────────────────────────────────────────────────
$i3 = gh issue create --repo $repo `
  --title "Feature: Módulo catalog - produtos reais de Produtos.txt" `
  --label "enhancement" `
  --body @"
## Parent PRD

#$prdNum

## What to build

Criar o módulo `catalog` (catalog.h / catalog.c) que carrega o catálogo real de produtos de `Produtos.txt` e o expõe para atribuição aleatória aos clientes, substituindo `gerar_produtos_aleatorios`.

Formato do ficheiro: `ID TAB Nome TAB Preco TAB Stock TAB tempoPassagem`
Exemplo: `10001	Azeitona Verde Retalhada [marcaB] 300 gr	3	12	3.2`

O módulo expõe:
- Carregamento em array dinâmico com crescimento por blocos de 64 (ficheiro >635 KB)
- `catalog_obter_produtos_aleatorios(catalogo, n, cfg)` que seleciona N produtos aleatórios do catálogo
- Libertação de memória

Conversão de `tempoPassagem`: valor float do ficheiro → `max(2, (int)ceil(valor))` para cumprir o intervalo `[2, TEMPO_ATENDIMENTO_PRODUTO]` do enunciado. O campo `stock` é lido mas não afeta a simulação.

A struct `Produto` recebe campos `id` (int) e `stock` (float). A função `gerar_produtos_aleatorios` em `product.c` é removida.

## Acceptance criteria

- [ ] Todos os produtos de `Produtos.txt` são carregados corretamente
- [ ] Precos preservados sem truncamento float
- [ ] `tempoPassagem` de cada produto é >= 2 após conversão
- [ ] Clientes criados na simulação têm produtos reais (nome, preco, tempo do catálogo)
- [ ] Memória do módulo é corretamente libertada na saída
- [ ] Compila sem warnings com `-Wall -Wextra -std=c11`

## Blocked by

None - can start immediately

## User stories addressed

- User story 2
"@

$i3Num = ($i3 -split '/')[-1]
Write-Host "Issue 3 criada: #$i3Num"

# ── Issue 4: Módulo client_registry ──────────────────────────────────────────
$i4 = gh issue create --repo $repo `
  --title "Feature: Módulo client_registry - clientes reais de Clientes.txt" `
  --label "enhancement" `
  --body @"
## Parent PRD

#$prdNum

## What to build

Criar o módulo `client_registry` (client_registry.h / client_registry.c) que carrega o registo real de clientes de `Clientes.txt` para usar na simulação e no menu de inserção.

Formato do ficheiro: `ID (6 dígitos) TAB Nome completo`
Exemplo: `609140	Carla Aguiar Pires` (IDs tratados como strings para preservar zeros iniciais como `043001`)

O módulo expõe:
- Carregamento em array dinâmico (ficheiro >290 KB)
- `registo_obter_aleatorio(registo)` para sugerir clientes ao inserir via menu
- Libertação de memória

A struct `Cliente` recebe campo `nome` (`char[MAX_NOME]`) com o nome completo. A função `criar_cliente` passa a receber o nome. `mostrar_cliente` e as entradas de log exibem o nome completo. O menu de pesquisa mostra nome completo ao encontrar o cliente.

## Acceptance criteria

- [ ] `Clientes.txt` é carregado no arranque sem erros
- [ ] IDs com zeros iniciais (ex: `043001`) preservados como string
- [ ] Inserção de novo cliente via menu sugere ID e nome do registo real
- [ ] Pesquisa de cliente mostra nome completo além do ID
- [ ] Entradas de log incluem nome completo do cliente
- [ ] Memória do módulo é corretamente libertada na saída
- [ ] Compila sem warnings com `-Wall -Wextra -std=c11`

## Blocked by

None - can start immediately

## User stories addressed

- User story 1
- User story 7
- User story 15
"@

$i4Num = ($i4 -split '/')[-1]
Write-Host "Issue 4 criada: #$i4Num"

# ── Issue 5: Menu e CSV ───────────────────────────────────────────────────────
$i5 = gh issue create --repo $repo `
  --title "Feature: Melhorias de menu e CSV - listagem por caixa e colunas separadas" `
  --label "enhancement" `
  --body @"
## Parent PRD

#$prdNum

## What to build

Duas melhorias de interface e relatório que dependem dos módulos de dados reais estarem integrados:

**1. Listagem de clientes por caixa específica:** Adicionar opção ao menu principal que pede ao utilizador o número de uma caixa e lista todos os clientes que foram atendidos por essa caixa, com nome completo, ID, produtos e tempo de espera.

**2. CSV com colunas separadas:** Separar `produtos_vendidos` e `produtos_oferecidos` em colunas distintas no ficheiro `estatisticas.csv`. Actualmente `totalProdutosVendidos` mistura os dois valores.

## Acceptance criteria

- [ ] Menu tem nova opção "Listar clientes atendidos por uma caixa"
- [ ] Utilizador escolhe o número da caixa e vê historial com nomes reais
- [ ] `estatisticas.csv` tem colunas separadas `produtos_vendidos` e `produtos_oferecidos`
- [ ] Valores nas novas colunas são consistentes com os totais globais
- [ ] Compila sem warnings com `-Wall -Wextra -std=c11`

## Blocked by

- Blocked by #$i2Num (nomes de operadores reais para exibir no historial)
- Blocked by #$i4Num (nomes de clientes reais para exibir na listagem)

## User stories addressed

- User story 12
- User story 13
"@

$i5Num = ($i5 -split '/')[-1]
Write-Host "Issue 5 criada: #$i5Num"

# ── Issue 6: Doxygen ──────────────────────────────────────────────────────────
$i6 = gh issue create --repo $repo `
  --title "Docs: Adicionar comentários Doxygen a todos os módulos públicos" `
  --label "documentation" `
  --body @"
## Parent PRD

#$prdNum

## What to build

Adicionar comentários Doxygen a todas as funções públicas de todos os headers do projeto (existentes e novos), substituindo o relatório escrito conforme permitido pelo enunciado.

Módulos a documentar: `config`, `store`, `register`, `queue`, `customer`, `product`, `hash`, `logging`, `utils`, `catalog`, `employee`, `client_registry`.

Para cada função pública documentar: `@brief`, `@param` para cada parâmetro, `@return` quando aplicável.

## Acceptance criteria

- [ ] Todos os ficheiros `.h` têm comentários Doxygen em todas as funções públicas
- [ ] `doxygen` corre sem warnings sobre símbolos não documentados
- [ ] Geração de HTML/PDF cobre todos os módulos
- [ ] Compila sem warnings com `-Wall -Wextra -std=c11`

## Blocked by

- Blocked by #$i2Num (módulo employee finalizado)
- Blocked by #$i3Num (módulo catalog finalizado)
- Blocked by #$i4Num (módulo client_registry finalizado)

## User stories addressed

- User story 10
"@

$i6Num = ($i6 -split '/')[-1]
Write-Host "Issue 6 criada: #$i6Num"

Write-Host ""
Write-Host "Todas as issues criadas:"
Write-Host "  PRD:      #$prdNum"
Write-Host "  Bug Fixes: #$i1Num"
Write-Host "  Employee:  #$i2Num"
Write-Host "  Catalog:   #$i3Num"
Write-Host "  Registry:  #$i4Num"
Write-Host "  Menu/CSV:  #$i5Num"
Write-Host "  Doxygen:   #$i6Num"
