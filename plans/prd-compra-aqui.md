# PRD – Compra Aqui Lda.: Gestão de Caixas de Supermercado

**Data:** 2026-05-03  
**Prazo de entrega:** 2026-06-01 (Época Normal)  
**Linguagem:** C (C11)  
**Estado:** Draft

---

## Problem Statement

O sistema atual gera clientes com IDs fictícios (`P1`, `P21`, …), produtos com nomes e preços aleatórios de uma lista hardcoded de 15 nomes, e atribui operadores com nomes genéricos (`Operador_1`, `Operador_2`). Foram fornecidos três ficheiros de dados reais que devem substituir toda esta geração sintética:

- **Clientes.txt** — registo real de clientes: `ID (6 dígitos) TAB Nome completo`
- **Produtos.txt** — catálogo real de produtos: `ID TAB Nome TAB Preço TAB Stock TAB tempoPassagem`
- **funcionarios.txt** — lista real de operadores de caixa: `ID (4 dígitos) TAB Nome completo`

Para além disso, foram identificados dois bugs críticos na simulação — a oferta repetida de produtos por cada instante acima de `MAX_ESPERA` em vez de uma única vez, e a política `MIN_FILA` que pode fechar a última caixa verdadeiramente aberta quando existem caixas em modo `CAIXA_A_FECHAR` — e várias inconsistências menores que comprometem a correção das estatísticas finais.

---

## Solution

Integrar os três ficheiros de dados reais como fonte primária de clientes, produtos e operadores, substituindo toda a geração aleatória sintética. Criar três novos módulos de carregamento (`catalog`, `employee`, `client_registry`) com interfaces limpas e testáveis em isolamento. Corrigir os cinco bugs identificados. Ajustar as estruturas de dados existentes para acomodar os novos campos (nome completo de cliente, ID de produto do catálogo, flag de oferta feita). Manter a modularidade, a clareza do código e a conformidade total com os requisitos do enunciado.

---

## User Stories

1. Como gerente, quero que os clientes na simulação tenham IDs e nomes reais carregados de `Clientes.txt`, para que os relatórios façam referência a pessoas identificáveis.
2. Como gerente, quero que os produtos nos carrinhos sejam retirados do catálogo real em `Produtos.txt`, com preços e tempos de passagem reais, para que as estatísticas de valor e tempo sejam realistas.
3. Como gerente, quero que cada caixa tenha um operador com nome e ID reais carregados de `funcionarios.txt`, para que a estatística "operador que atendeu menos pessoas" apresente nomes reais.
4. Como gerente, quero que um cliente que espere mais de `MAX_ESPERA` receba exatamente UM produto oferecido (não um por cada instante acima do limiar), para que o controlo de custo das ofertas seja correto.
5. Como gerente, quero que a política `MIN_FILA` nunca feche a última caixa verdadeiramente aberta mesmo que existam caixas em modo `CAIXA_A_FECHAR`, para que o supermercado nunca fique sem atendimento.
6. Como gerente, quero que a contagem de "produtos vendidos" não inclua os produtos oferecidos gratuitamente, para que as estatísticas de vendas reflitam receita real.
7. Como gerente, quero pesquisar um cliente pelo seu ID real de 6 dígitos e ver o seu nome completo, caixa atual e posição na fila.
8. Como gerente, quero que o cálculo de memória utilizada inclua todos os buffers de catálogo, registo de clientes e a estrutura do hash table, para que o valor reportado seja completo.
9. Como utilizador do menu, quero que a opção de avançar simulação rejeite valores negativos ou zero com uma mensagem de erro clara, em vez de não fazer nada silenciosamente.
10. Como avaliador, quero que o código tenha comentários Doxygen nas funções públicas de todos os módulos, eliminando a necessidade de relatório separado.
11. Como gerente, quero que as estatísticas finais mostrem o nome completo do operador com menos atendimentos (e não apenas `Operador_N`), para que a gestão de recursos humanos seja informada.
12. Como gerente, quero poder listar todos os clientes atendidos por uma caixa específica escolhida no menu, em vez de ver todas as caixas de uma vez.
13. Como gerente, quero que os ficheiros CSV de estatísticas separem as colunas `produtos_vendidos` e `produtos_oferecidos`, para que a análise de dados seja direta.
14. Como gerente, quero que a memória desperdiçada pelos buckets vazios do hash table seja corretamente incluída no cálculo total.
15. Como avaliador, quero que os IDs de clientes com zeros à esquerda (ex: `043001`) sejam tratados como strings e não como inteiros, para que não percam os zeros iniciais.

---

## Implementation Decisions

### Novos Módulos

#### `catalog.h / catalog.c`
Carrega e gere o catálogo de produtos de `Produtos.txt`. Substitui `gerar_produtos_aleatorios` em `product.c`.

Interface pública:
- `catalog_carregar(filename)` → `CatalogoProdutos*`
- `catalog_obter_produtos_aleatorios(catalogo, n, cfg)` → `Produto*`
- `catalog_destruir(catalogo)`

Decisões internas:
- Array dinâmico que cresce em blocos de 64 entradas para suportar o ficheiro grande (>635 KB).
- `tempoPassagem` é lido como `float` do ficheiro e convertido para `int` com `max(2, (int)ceil(valor))` para cumprir o intervalo `[2, TEMPO_ATENDIMENTO_PRODUTO]` do enunciado.
- O campo `stock` (coluna 4 do ficheiro) é lido mas não afeta a simulação nesta versão.

#### `employee.h / employee.c`
Carrega a lista de funcionários de `funcionarios.txt` e expõe atribuição aleatória a caixas.

Interface pública:
- `funcionarios_carregar(filename)` → `ListaFuncionarios*`
- `funcionarios_obter_aleatorio(lista)` → `Funcionario*`
- `funcionarios_destruir(lista)`

#### `client_registry.h / client_registry.c`
Carrega o registo de clientes de `Clientes.txt`. Usado para sugerir clientes ao inserir via menu.

Interface pública:
- `registo_carregar(filename)` → `RegistoClientes*`
- `registo_obter_aleatorio(registo)` → `EntradaCliente*`
- `registo_destruir(registo)`

Decisões internas:
- IDs tratados como strings (não inteiros) para preservar zeros iniciais (`043001`).
- Array dinâmico com seleção aleatória O(1) após carregamento.

---

### Módulos Modificados

#### `product.h / product.c`
- Adicionar campo `id` (int) à struct `Produto` para o ID do catálogo.
- Adicionar campo `stock` (float) à struct `Produto`.
- `tempoPassagem` mantém-se `int`; convertido na carga com `max(2, (int)ceil(valor_ficheiro))`.
- Remover `gerar_produtos_aleatorios` — substituído por `catalog_obter_produtos_aleatorios`.
- Manter `tempo_total_produtos` e `valor_total_produtos` inalterados.

#### `customer.h / customer.c`
- Adicionar campo `nome` (`char[MAX_NOME]`) à struct `Cliente` para o nome completo de `Clientes.txt`.
- Adicionar campo `oferecimentoFeito` (`bool`) — flag que garante oferta única por cliente.
- `criar_cliente()` recebe agora também o nome completo do cliente.
- `mostrar_cliente()` exibe o nome completo além do ID.

#### `register.h / register.c`
- Adicionar campo `idFuncionario` (int) à struct `Caixa`.
- `caixa_init()` recebe um `Funcionario*` em vez de uma string de nome gerada.

#### `store.c`
- **BUG-1 fix:** Em `avancar_simulacao`, verificar `!cliente->oferecimentoFeito` antes de chamar `oferecer_um_produto`; marcar o flag após a oferta.
- **BUG-2 fix:** Criar `contar_caixas_realmente_abertas` que conta apenas `CAIXA_ABERTA`; usar esta função no guarda `abertas > 1` da política `MIN_FILA`.
- **BUG-3 fix:** Em `concluir_atendimento`, usar `cliente->nProdutos - cliente->produtosOferecidos` para `totalProdutosVendidos`.
- Substituir `gerar_nome_operador` por atribuição de `Funcionario*` real em `supermercado_init`.
- `supermercado_init()` recebe agora uma `ListaFuncionarios*`.
- Adicionar `sizeof(HashClientes)` ao cálculo de `memoria_utilizada`.

#### `config.h / config.c`
- Adicionar constantes de caminho: `CLIENTES_FILE`, `PRODUTOS_FILE`, `FUNCIONARIOS_FILE`.

#### `main.c`
- Carregar `ListaFuncionarios`, `RegistoClientes` e `CatalogoProdutos` no arranque, antes de `supermercado_init`.
- **BUG-4 fix:** Validar `passos > 0` na opção 3 do menu.
- Adicionar opção de menu para listar clientes atendidos por uma caixa específica.
- Libertar os novos recursos na saída do programa.
- Separar colunas `produtos_vendidos` e `produtos_oferecidos` no CSV de estatísticas.

#### `logging.c / logging.h`
- Incluir o nome completo do cliente nas entradas de log `NOVO_CLIENTE`, `CLIENTE_ATENDIDO` e `PRODUTO_OFERECIDO`.

---

### Correções de Bugs

| ID | Severidade | Descrição | Fix |
|----|-----------|-----------|-----|
| BUG-1 | **CRÍTICO** | `oferecer_um_produto` chamado em cada instante após `MAX_ESPERA` → cliente recebe N produtos | Adicionar `oferecimentoFeito` (bool) à struct `Cliente`; só oferecer quando `false` |
| BUG-2 | **CRÍTICO** | `contar_caixas_abertas` inclui `CAIXA_A_FECHAR` → política `MIN_FILA` pode fechar a última caixa aberta | Criar `contar_caixas_realmente_abertas` que conta só `CAIXA_ABERTA` |
| BUG-3 | **MÉDIO** | `totalProdutosVendidos` inclui produtos oferecidos (inconsistente com `totalValorVendido`) | Subtrair `produtosOferecidos` de `nProdutos` no momento do atendimento |
| BUG-4 | **MÉDIO** | Avançar simulação com `passos <= 0` não produz erro | Validar e rejeitar com mensagem antes de chamar `avancar_simulacao` |
| BUG-5 | **BAIXO** | `sizeof(HashClientes)` ausente de `memoria_utilizada` | Adicionar ao total calculado |

---

### Decisões Arquiteturais

- **IDs de cliente como strings:** Os IDs de `Clientes.txt` têm 6 dígitos com possíveis zeros iniciais (`043001`). São tratados como `char[MAX_ID]`, não como inteiros, em toda a cadeia (hash, fila, pesquisa).
- **Hash table mantém-se com HASH_SIZE=101 e encadeamento.** Com milhares de clientes possíveis mas tipicamente dezenas simultâneos, o tamanho atual é adequado.
- **Stock não afeta a simulação:** O campo stock de `Produtos.txt` é lido para completude da struct mas não é decrementado — a gestão de inventário está fora de âmbito.
- **Carregamento único dos ficheiros grandes:** `Clientes.txt` (>290 KB) e `Produtos.txt` (>635 KB) são lidos uma vez no arranque com arrays dinâmicos; após isso toda a seleção é O(1) por índice aleatório.

---

## Testing Decisions

**O que faz um bom teste:** Testa comportamento externo observável — o que a função retorna e o estado das estruturas após a operação — sem depender de detalhes de implementação internos (como o algoritmo de hash ou a estratégia de crescimento do array).

### Módulos a testar

| Módulo | Testes |
|--------|--------|
| `catalog` | Número de produtos carregados correto; preços sem truncamento float; `tempoPassagem` → `max(2, ceil(valor))`; seleção aleatória não retorna NULL para N ≤ tamanho do catálogo |
| `employee` | Nomes e IDs corretos após carga; operadores diferentes atribuídos a caixas diferentes |
| `client_registry` | IDs com zeros iniciais preservados; nome completo carregado sem truncamento |
| `store` (simulação) | Cliente com espera `MAX_ESPERA+10` recebe exatamente 1 produto oferecido (BUG-1); sistema com 1 `CAIXA_ABERTA` + 1 `CAIXA_A_FECHAR` não fecha a última aberta (BUG-2); `totalProdutosVendidos == nProdutos - produtosOferecidos` após atendimento (BUG-3); avançar 0 passos é rejeitado sem alterar estado (BUG-4); cliente duplicado retorna `INSERIR_CLIENTE_DUPLICADO`; fechar caixa imediata redistribui todos os clientes |

**Prior art:** `run_tests.sh` na raiz do projeto; ficheiros de referência em `output/` (`teste_funcional.txt`, `teste_ofertas.txt`).

---

## Out of Scope

- Gestão de stock real — o campo `stock` de `Produtos.txt` é lido mas não decrementado na simulação.
- Interface gráfica ou web — sistema exclusivamente menu-driven em terminal.
- Persistência do estado da simulação entre execuções (save/restore do estado).
- Multi-threading ou paralelismo.
- Autenticação de utilizadores ou controlo de acesso.
- Rede ou comunicação entre processos.
- Base de dados — apenas ficheiros de texto e CSV.

---

## Further Notes

- **Prazo:** 2026-06-01 (Época Normal). Entregar em Moodle um ZIP com: código fonte, ficheiros de dados (`Configuracao.txt`, `Dados.txt`, `Clientes.txt`, `Produtos.txt`, `funcionarios.txt`), Doxygen ou relatório, e ficheiro de identificação dos alunos. Sem executáveis, código objeto ou ficheiros de IDE.
- **Doxygen como substituto do relatório:** O enunciado aceita explicitamente Doxygen em vez de relatório. Recomendar adicionar comentários Doxygen a todos os headers públicos dos módulos novos e modificados.
- **Eficiência no carregamento:** `Produtos.txt` (>635 KB) e `Clientes.txt` (>290 KB) são ficheiros grandes. O enunciado menciona explicitamente que "existe grande quantidade de informação em alguns ficheiros". Usar arrays dinâmicos com crescimento por blocos (ex: 64 entradas) para evitar realloc frequente.
- **Avaliação em 2 etapas:** 40% avaliação em aula + 60% defesa. O à-vontade na apresentação e o domínio do código são explicitamente valorizados — manter o código simples e bem comentado facilita a explicação oral.
- **Registo do grupo:** A constituição do grupo deve ser submetida no Moodle até **2026-05-07**.
