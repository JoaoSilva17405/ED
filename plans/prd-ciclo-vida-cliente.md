# PRD: Ciclo de Vida Completo do Cliente no Supermercado

## Problem Statement

A simulação atual não implementa corretamente o ciclo de vida do cliente dentro do supermercado. Quando um cliente é inserido, é imediatamente colocado na fila de uma caixa, sem passar pela fase de compras. Além disso, o número de produtos (N) é selecionado manualmente pelo operador em vez de ser gerado aleatoriamente pelo sistema. O campo de tempo de compra dos produtos (`stock`) está mal nomeado e não é utilizado para nada, apesar de já existir no ficheiro `Produtos.txt`. Em suma, o fluxo correto — chegar → fazer compras → entrar na fila → pagar → sair — não está implementado.

## Solution

Implementar o ciclo de vida completo do cliente:

1. Quando um cliente chega ao supermercado, o sistema gera automaticamente N produtos aleatórios (retirados do catálogo). N é determinado aleatoriamente dentro de um intervalo configurável.
2. O cliente passa um tempo na loja igual à soma dos `tempoCompra` dos N produtos antes de entrar na fila.
3. Após a fase de compras, o cliente entra na fila da caixa com menos pessoas.
4. O cliente aguarda na fila (tempo definido pelos clientes à sua frente) e, quando é a sua vez, paga (tempo = soma dos `tempoPassagem` dos N produtos).
5. Após o pagamento, o cliente abandona o supermercado.

## User Stories

1. Como operador, quero inserir um cliente fornecendo apenas o ID, para que o sistema gere automaticamente os produtos que ele vai comprar.
2. Como operador, quero que o número de produtos (N) de cada cliente seja gerado aleatoriamente, para que a simulação seja realista sem intervenção manual.
3. Como operador, quero configurar o intervalo de N (mínimo e máximo de produtos por cliente) em `Configuracao.txt`, para que seja possível ajustar o comportamento da simulação.
4. Como operador, quero que os produtos sejam selecionados aleatoriamente do catálogo carregado, para que os tempos e preços reflitam dados reais.
5. Como operador, quero que o cliente passe um tempo na loja antes de entrar na fila (soma dos `tempoCompra` dos produtos), para que a simulação reflita o tempo real de compras.
6. Como operador, quero ver no estado do supermercado quais os clientes que estão atualmente na fase de compras (ainda não na fila), para acompanhar o estado completo da simulação.
7. Como operador, quero que o tempo de espera na fila seja calculado corretamente a partir do momento em que o cliente entra na fila (e não do momento em que chega à loja), para que as estatísticas sejam corretas.
8. Como operador, quero que o tempo de pagamento na caixa seja a soma dos `tempoPassagem` dos N produtos, para que o comportamento seja consistente com os requisitos.
9. Como operador, quero que os clientes em fase de compras sejam guardados no snapshot e restaurados corretamente, para que o estado da simulação seja preservado entre sessões.
10. Como operador, quero que ao pesquisar um cliente em fase de compras, o sistema indique que ele está "a fazer compras" em vez de indicar uma caixa, para que a informação mostrada seja correta.
11. Como operador, quero que não seja possível inserir um cliente duplicado mesmo que o cliente original ainda esteja na fase de compras, para evitar inconsistências.
12. Como operador, quero que o campo `tempoCompra` dos produtos seja lido corretamente do ficheiro `Produtos.txt` e usado na fase de compras, para que os tempos reflitam os dados do catálogo.
13. Como operador, quero que a política automática de abertura/fecho de caixas seja avaliada após cada cliente terminar as compras e entrar na fila, para que o sistema reaja ao aumento de carga.
14. Como operador, quero que ao terminar as compras o cliente seja dirigido para a caixa com menos pessoas, para que a carga seja distribuída equitativamente.
15. Como operador, quero que o log de eventos registe a entrada na loja e a transição para a fila separadamente, para que seja possível auditar o comportamento da simulação.

## Implementation Decisions

### Módulos a modificar

**Produto (product.h / product.c)**
- Renomear o campo `float stock` para `int tempoCompra` — o campo já existe no ficheiro `Produtos.txt` (4ª coluna) mas estava mal nomeado e não era usado.
- Adicionar a função `tempo_compra_total_produtos` que soma os `tempoCompra` de um array de produtos.
- `gerar_produtos_aleatorios` (fallback sem catálogo) deve gerar `tempoCompra` aleatoriamente.

**Catálogo (catalog.c)**
- `catalog_carregar`: ler a 4ª coluna como `tempoCompra` (era lida como `stock`), convertendo float para int com ceiling.
- `catalog_obter_produtos_aleatorios`: não aplicar cap a `tempoCompra` (vem do ficheiro).

**Configuração (config.h / config.c)**
- Adicionar `minProdutos` (default 1) e `maxProdutos` (default 20) à struct `Configuracao`.
- Parsear as chaves `MIN_PRODUTOS` e `MAX_PRODUTOS` em `Configuracao.txt`.

**Cliente (customer.h / customer.c)**
- Adicionar três campos à struct `Cliente`:
  - `instanteEntradaLoja` — tick em que o cliente chegou ao supermercado
  - `tempoCompraTotal` — soma dos `tempoCompra` dos seus produtos
  - `comprasTerminadas` — true quando a fase de compras terminou
- `criar_cliente` inicializa: `instanteEntradaLoja = instanteAtual`, `tempoCompraTotal = 0`, `comprasTerminadas = false`.
- Para clientes carregados do snapshot (já na fila), `comprasTerminadas` fica `true`.

**Supermercado (store.h / store.c)**
- Adicionar à struct `Supermercado`:
  - `clientesEmLoja` — array dinâmico de ponteiros para clientes em fase de compras
  - `nClientesEmLoja` — número atual
  - `capClientesEmLoja` — capacidade alocada
- Adicionar constante `INSERIR_CLIENTE_EM_COMPRAS` (valor negativo) para indicar sucesso na entrada em loja.
- `supermercado_init`: inicializar os novos campos a NULL/0.
- `supermercado_destruir`: libertar clientes em loja.
- `inserir_novo_cliente`: em vez de colocar o cliente diretamente na fila, colocá-lo em `clientesEmLoja`. Retorna `INSERIR_CLIENTE_EM_COMPRAS` em caso de sucesso. Adiciona ao hash com `idCaixa = -1` (permite detetar duplicados).
- `avancar_simulacao`: no início de cada tick, iterar `clientesEmLoja` e mover para a fila os clientes cujo `instanteAtual >= instanteEntradaLoja + tempoCompraTotal`. Ao mover: definir `instanteEntradaFila = instanteAtual`, escolher caixa com `encontrar_caixa_para_novo_cliente`, atualizar hash e avaliar política de caixas.
- `guardar_snapshot`: guardar secção `EM_LOJA N` com linhas `LOJA_CLIENTE id nome entrada_loja=X tempo_compra=Y` seguidas de `PRODUTO`.
- `carregar_formato_novo`: parsear secção `EM_LOJA` / `LOJA_CLIENTE` e reconstituir clientes em `clientesEmLoja`.
- `mostrar_estado_supermercado`: mostrar lista de clientes em fase de compras.
- `pesquisar_cliente`: se `idCaixa = -1`, indicar que o cliente está "a fazer compras".
- `mover_cliente_caixa`: rejeitar graciosamente se `idCaixa < 0` (cliente ainda em loja).
- `parse_produto_linha`: atualizar para ler `tempoCompra` como int (3ª posição a contar do fim).
- `guardar_produtos_snapshot`: escrever `tempoCompra` como `%d`.

**Menu principal (main.c)**
- Opção 4: substituir o loop de seleção manual de produtos por geração automática. Fluxo: pedir apenas o ID → resolver nome → gerar N aleatório com `minProdutos + rand() % (maxProdutos - minProdutos + 1)` → chamar `catalog_obter_produtos_aleatorios` → chamar `inserir_novo_cliente`.
- Atualizar mensagem de resultado para o caso `INSERIR_CLIENTE_EM_COMPRAS`.
- Atualizar mensagem de duplicado para indicar se o cliente está em loja ou na fila.

**Ficheiro de configuração (Configuracao.txt)**
- Adicionar linhas `MIN_PRODUTOS 1` e `MAX_PRODUTOS 20`.

### Decisões arquiteturais

- Os clientes em fase de compras são guardados num array dinâmico separado (`clientesEmLoja`) em vez de numa nova fila, pois não existe ordenação relevante entre eles.
- O hash regista clientes em loja com `idCaixa = -1`. Isto garante deteção de duplicados e permite pesquisa, mas todos os consumidores do hash têm de verificar `idCaixa < 0` antes de acederem a `sm->caixas[idCaixa]`.
- `inserir_novo_cliente` não determina a caixa de destino na inserção — apenas quando o cliente termina as compras. Isto evita reservar uma caixa para um cliente que ainda não está disponível.
- A função `tempo_compra_total_produtos` é um módulo profundo: interface simples, testável em isolamento.

## Testing Decisions

### O que constitui um bom teste
- Testar apenas comportamento observável externamente (output de funções, estado após chamada), nunca detalhes de implementação internos.
- Cada teste deve ter uma única razão para falhar.

### Módulos a testar

**`tempo_compra_total_produtos`**
- Input: array de produtos com `tempoCompra` variados; array vazio; array com um produto.
- Output: soma correta dos `tempoCompra`.

**`gerar_produtos_aleatorios` (com `tempoCompra`)**
- Verificar que todos os produtos gerados têm `tempoCompra > 0`.

**`config_default` + `carregar_configuracao`**
- Defaults de `minProdutos` e `maxProdutos` corretos.
- Parse correto de `MIN_PRODUTOS` e `MAX_PRODUTOS` de um ficheiro de teste.

**`inserir_novo_cliente` (fase de compras)**
- Após inserção, cliente está em `clientesEmLoja` e não em nenhuma fila.
- `nClientesEmLoja` incrementa.
- Retorna `INSERIR_CLIENTE_EM_COMPRAS`.
- Inserção de duplicado retorna `INSERIR_CLIENTE_DUPLICADO`.

**`avancar_simulacao` (transição loja → fila)**
- Avançar `tempoCompraTotal` ticks: cliente é removido de `clientesEmLoja` e aparece na fila de uma caixa.
- `instanteEntradaFila` é definido no tick correto.
- Avançar menos ticks que `tempoCompraTotal`: cliente permanece em `clientesEmLoja`.

## Out of Scope

- Geração automática de clientes durante `avancar_simulacao` sem intervenção do operador (chegadas automáticas por distribuição de Poisson ou intervalo fixo).
- Modelação do comportamento do cliente na loja (percurso, categorias de produtos).
- Interface gráfica ou visualização em tempo real.
- Alteração ao mecanismo de oferta de produtos para clientes na fila.

## Further Notes

- O ficheiro `Dados.txt` existente usa `%.1f` para o campo que era `stock`. O parser de snapshot deve suportar tanto o formato float legado como o novo formato int, usando `atof` seguido de truncagem.
- O campo `tempoCompra` nos produtos carregados pelo catálogo vem diretamente do ficheiro e não é limitado por `tempoAtendimentoProduto` (ao contrário de `tempoPassagem`).
- A função `cliente_tempo_atendimento` (tempo na caixa) continua a usar `tempoPassagem` — não muda.
