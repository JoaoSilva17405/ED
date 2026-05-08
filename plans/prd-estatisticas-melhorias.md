# PRD: Melhorias nas Estatísticas e Histórico

## Problem Statement

O sistema de estatísticas e histórico do simulador tem quatro problemas:

1. **Operador errado no historial por cliente** — `historico_caixas.csv` usa `ultimoOperador` da caixa para todos os clientes históricos, mesmo os atendidos por um operador anterior. Quando uma caixa muda de operador (fecha e reabre), o CSV atribui o operador errado a parte dos clientes.

2. **Requisito do PDF não cumprido: "operador que atendeu menos pessoas"** — a implementação atual compara por caixa usando `ultimoOperador`, não por operador real. Se dois operadores trabalharam na mesma caixa em momentos diferentes, os totais de ambos ficam misturados num único contador da caixa, tornando a estatística incorreta.

3. **Histórico de utilizador sem persistência real** — `historico_utilizador.csv` é truncado (`fopen "w"`) a cada sessão. O histórico real acumula-se entre sessões mas não fica registado quem abriu e fechou cada sessão nem o seu resumo.

4. **Total global de valor vendido em falta** — `estatisticas.csv` tem valor vendido por caixa mas não tem o total global. O campo existe por caixa mas nunca é agregado nem exportado.

## Solution

1. Guardar o operador correto em cada `Cliente` no momento exato do atendimento e usá-lo em todos os outputs por cliente.

2. Calcular estatísticas por operador on-demand no final da simulação, percorrendo `historicoAtendidos` de todas as caixas e agrupando por `operadorAtendimento`, sem estrutura de dados adicional em memória durante a simulação.

3. Mudar o log de utilizador para modo append com deteção automática de ficheiro novo, e adicionar entradas de início e fim de sessão.

4. Adicionar campo `totalValorVendido` ao `Supermercado`, acumulado em cada atendimento, exportado no CSV global e na consola.

## User Stories

1. Como utilizador, quero que `historico_caixas.csv` mostre o operador que efetivamente atendeu cada cliente, para que o registo seja fidedigno mesmo quando a caixa mudou de operador durante a simulação.

2. Como utilizador, quero ver "Operador com menos pessoas: X com N clientes" nas estatísticas finais referindo o operador real (pessoa), não a caixa, para que o requisito do enunciado seja cumprido corretamente.

3. Como utilizador, quero ver nas estatísticas finais uma lista de todos os operadores com o seu número de clientes atendidos, produtos vendidos e valor vendido, para poder comparar o desempenho real de cada operador.

4. Como utilizador, quero que `estatisticas.csv` tenha uma secção por operador com `operador,clientes_atendidos,produtos_vendidos,valor_vendido`, para poder analisar o ficheiro externamente sem precisar de cruzar dados do `historico_caixas.csv` manualmente.

5. Como utilizador, quero que `historico_utilizador.csv` acumule entradas de sessões anteriores sem ser apagado, para ter um registo contínuo e real de toda a atividade do programa.

6. Como utilizador, quero que o início de cada sessão fique registado com o ficheiro de snapshot carregado e o instante atual, e o fim com o número de clientes atendidos e instante final, para poder reconstituir o historial de sessões.

7. Como utilizador, quero que o cabeçalho de `historico_utilizador.csv` apareça apenas uma vez (na criação do ficheiro), para o CSV se manter válido em ferramentas externas ao longo de múltiplas sessões.

8. Como utilizador, quero ver o total global de valor vendido nas estatísticas finais em consola e no `estatisticas.csv`, para avaliar o desempenho financeiro global do supermercado sem ter de somar as colunas por caixa.

9. Como utilizador, quero que a secção por caixa em `estatisticas.csv` continue a mostrar `ultimoOperador` da caixa, pois identifica quem operou a caixa — informação distinta das estatísticas individuais por operador.

10. Como utilizador, quero que `caixa_listar_historico` continue a mostrar `ultimoOperador` no cabeçalho do histórico da caixa, para identificar rapidamente quem foi o último responsável pela caixa.

## Implementation Decisions

### Módulo 1 — Operador correto por cliente (`customer`, `store`, `logging`)

- Adicionar campo `char operadorAtendimento[MAX_OPERADOR]` ao struct `Cliente`.
- Inicializar `operadorAtendimento[0] = '\0'` em `criar_cliente`.
- Em `concluir_atendimento`, copiar `caixa->operador` para `cliente->operadorAtendimento` **antes** de chamar `caixa_adicionar_historico` — garante o operador ativo naquele instante.
- Em `guardar_historico_caixas_csv`, usar `c->operadorAtendimento` por cliente em vez de `sm->caixas[i].ultimoOperador`.
- `ultimoOperador` na `Caixa` mantém-se intacto — continua a ser usado no snapshot (`ULTIMO_OPERADOR`), em `caixa_listar_historico` e na secção por caixa de `estatisticas.csv`.

### Módulo 2 — Estatísticas por operador on-demand (`store`)

- Sem nova estrutura de dados persistente. O cálculo é feito no final a partir de `historicoAtendidos`.
- Criar função auxiliar estática `calcular_estatisticas_operadores` que:
  1. Percorre `historicoAtendidos` de todas as caixas
  2. Agrupa por `c->operadorAtendimento` num array local temporário de structs `{ nome, clientes, produtos, valor }`
  3. Determina o operador com menos clientes
  4. Retorna (ou preenche) o array para uso em consola e CSV
- Campos acumulados por operador: `totalClientes`, `totalProdutos`, `totalValor`
- `mostrar_estatisticas_finais` chama esta função e imprime:
  - Lista de todos os operadores com os seus totais
  - "Operador com menos pessoas: X com N clientes"
- `guardar_estatisticas_csv` chama esta função e adiciona secção `operador,clientes_atendidos,produtos_vendidos,valor_vendido` após a secção por caixa.

### Módulo 3 — Histórico de utilizador com persistência (`logging`, `main`)

- Alterar `abrir_log_acoes` para detetar se o ficheiro já existe:
  - Tenta `fopen(filename, "r")` — se bem sucedido, o ficheiro existe → fechar e reabrir com `"a"` (sem cabeçalho)
  - Se falha → criar com `"w"` e escrever cabeçalho `timestamp,acao,detalhes`
- Assinatura de `abrir_log_acoes` não muda.
- Em `main.c`, após carregar o snapshot, chamar `log_acao` com:
  - Ação: `INICIO_SESSAO`
  - Detalhes: `snapshot=<ficheiro> instante=<instanteAtual>`
- Em `main.c`, imediatamente antes de fechar o log, chamar `log_acao` com:
  - Ação: `FIM_SESSAO`
  - Detalhes: `clientes_atendidos=<N> instante_final=<instanteAtual>`

### Módulo 4 — Total global de valor vendido (`store`)

- Adicionar campo `float totalValorVendido` ao struct `Supermercado`.
- Inicializar a `0.0f` em `supermercado_init`.
- Em `concluir_atendimento`, acumular: `sm->totalValorVendido += cliente_valor_total(cliente) - cliente->valorOferecido` (paralelamente ao que já acontece na caixa).
- Em `guardar_estatisticas_csv`, adicionar `total_valor_vendido` à linha global após `total_valor_oferecido`.
- Em `mostrar_estatisticas_finais`, adicionar linha `Total de valor vendido: %.2f` após `Total de produtos vendidos`.
- Não persistido no snapshot — acumulado de raiz a cada sessão, consistente com os restantes contadores globais.

### Interfaces alteradas

| Componente | Alteração |
|---|---|
| `Cliente` struct | + `char operadorAtendimento[MAX_OPERADOR]` |
| `Supermercado` struct | + `float totalValorVendido` |
| `criar_cliente` | Inicializa `operadorAtendimento[0] = '\0'` |
| `concluir_atendimento` | Preenche `operadorAtendimento`; acumula `sm->totalValorVendido` |
| `abrir_log_acoes` | Lógica interna append vs truncate; assinatura inalterada |
| `guardar_historico_caixas_csv` | Usa `c->operadorAtendimento` por cliente |
| `guardar_estatisticas_csv` | + coluna `total_valor_vendido` na linha global; + secção por operador |
| `mostrar_estatisticas_finais` | + total valor vendido; + lista e mínimo por operador |
| `main.c` | Chama `log_acao` para `INICIO_SESSAO` e `FIM_SESSAO` |

## Testing Decisions

**O que torna um bom teste:** testa comportamento externo observável — conteúdo dos ficheiros CSV, valores nos structs após execução — não detalhes internos. Seguir o padrão de `test_issue5_snapshot.c` (estado controlado → executar → verificar output).

### Módulo 1 — Operador por cliente

- Criar dois clientes, atendê-los na mesma caixa com operadores diferentes (simular abertura com Operador A, fechar, reabrir com Operador B).
- Verificar que `cliente->operadorAtendimento` é preenchido corretamente para cada cliente.
- Chamar `guardar_historico_caixas_csv` e verificar que o CSV tem o operador correto por linha.

### Módulo 2 — Estatísticas por operador

- Criar clientes servidos por dois operadores distintos na mesma caixa (em momentos diferentes).
- Chamar `mostrar_estatisticas_finais` / `guardar_estatisticas_csv` com output capturado.
- Verificar que os totais por operador somam corretamente e que o operador com menos clientes é identificado corretamente.
- Verificar que a secção por operador aparece no CSV com os valores corretos.

### Módulo 3 — Append do log

- Criar log numa primeira "sessão" com algumas entradas, fechar.
- Simular segunda sessão via `abrir_log_acoes` no mesmo ficheiro.
- Verificar que o ficheiro resultante tem o cabeçalho **apenas uma vez** e contém entradas de ambas as sessões.
- Verificar presença e formato correto de `INICIO_SESSAO` e `FIM_SESSAO`.

### Módulo 4 — Total valor vendido

- Simular atendimento de clientes com produtos de valor conhecido (incluindo alguns oferecidos).
- Verificar que `sm->totalValorVendido` acumula corretamente (valor líquido, descontando ofertas).
- Chamar `guardar_estatisticas_csv` e verificar que `total_valor_vendido` aparece na linha global com o valor correto.

**Ficheiros de teste de referência:** `test_issue5_snapshot.c`, `test_issue7_ciclo_vida.c`.

## Out of Scope

- Tempo médio de espera por operador (não pedido).
- Persistência da lista de operadores no snapshot (estatísticas são por sessão, consistente com os restantes contadores).
- Total global de `somaTemposEspera` exportado no CSV.
- Caixa com maior valor vendido na consola (não pedido explicitamente).
- Reescrita completa do sistema de logging.
- Estrutura de dados em tempo real (Lista Ligada) para acumulação por operador — decidido usar cálculo on-demand.

## Further Notes

- O campo `ultimoOperador` na `Caixa` mantém-se com os seus usos atuais (snapshot, cabeçalho de histórico, secção por caixa do CSV). Não é removido — serve um propósito diferente de `operadorAtendimento` no `Cliente`.
- A função auxiliar `calcular_estatisticas_operadores` opera sobre `historicoAtendidos` que só contém clientes da sessão atual. Estatísticas de sessões anteriores não são acumuladas — comportamento consistente com `totalClientesAtendidos` e demais contadores do `Supermercado`.
- O formato das entradas de sessão no log segue o padrão existente: `YYYY-MM-DD HH:MM:SS,INICIO_SESSAO,snapshot=data/Dados.txt instante=7034` e `YYYY-MM-DD HH:MM:SS,FIM_SESSAO,clientes_atendidos=12 instante_final=7200`.
- Para o agrupamento por operador em `calcular_estatisticas_operadores`, usar um array local de tamanho máximo `MAX_CAIXAS * 2` (cobre o cenário em que cada caixa teve dois operadores distintos durante a sessão) — sem alocação dinâmica.
