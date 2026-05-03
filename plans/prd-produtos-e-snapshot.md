# PRD — Seleção Real de Produtos e Snapshot Completo do Estado

**Data:** 2026-05-03
**Prazo de entrega:** 2026-06-01 (Época Normal)
**Linguagem:** C (C11)
**Estado:** Draft

---

## Problem Statement

O sistema apresenta duas lacunas funcionais que comprometem a utilidade da simulação:

**1. Inserção de cliente sem produtos reais:** Ao inserir um novo cliente (opção 4), o gerente apenas indica o número de produtos — o sistema atribui N produtos aleatórios do catálogo sem interação. Não é possível saber quais produtos estão no carrinho nem o valor real antes do atendimento. A simulação não reflete compras reais.

**2. Estado inicial fictício e snapshot incompleto:** O ficheiro `Dados.txt` contém IDs sintéticos (`P1`, `P21`) sem correspondência em `Clientes.txt`. Ao mostrar o estado das caixas (opção 2), os clientes aparecem sem nome. Para além disso, quando o programa termina, o estado atual das caixas perde-se — não existe persistência real. O snapshot deveria guardar o estado completo (instante, todas as caixas, todos os clientes com os seus produtos exatos e timestamps) e ser restaurado ao reiniciar.

---

## Solution

**Problema 1:** Substituir o pedido "Numero de produtos:" por um loop interativo de pesquisa no catálogo. O gerente pesquisa por nome parcial ou ID exato, seleciona de uma lista numerada, e repete até confirmar o carrinho. O cliente só é inserido após confirmação.

**Problema 2:** Implementar persistência bidirecional completa. Todo o cliente inserido no sistema é imediatamente registado em `Clientes.txt` (garantindo que nunca existem IDs desconhecidos). O snapshot (`Dados.txt`) guarda o estado integral — instante atual, estado de todas as caixas, cliente em atendimento com produtos e timestamps exatos, todos os clientes em fila com produtos e timestamps. O snapshot é gravado automaticamente ao sair e disponível como opção de menu. Ao arrancar, o estado é restaurado fielmente.

---

## User Stories

### Problema 1 — Seleção de produtos

1. Como gerente, quero pesquisar produtos do catálogo por nome parcial ao construir o carrinho de um cliente, para que a simulação reflita produtos reais.
2. Como gerente, quero pesquisar produtos por ID exato do catálogo, para que possa localizar rapidamente um artigo específico.
3. Como gerente, quero ver uma lista numerada com até 10 resultados por pesquisa, para que possa selecionar sem ser sobrecarregado.
4. Como gerente, quero adicionar múltiplos produtos ao carrinho um a um, para que o carrinho reflita a compra completa do cliente.
5. Como gerente, quero ver um resumo do carrinho (nome, preço e tempo de cada produto) antes de confirmar a inserção, para que possa detetar erros.
6. Como gerente, quero cancelar a inserção durante a construção do carrinho (Enter com carrinho vazio), para que não crie entradas inválidas na fila.
7. Como gerente, quero que o número de produtos seja determinado pelo carrinho construído, sem ter de o introduzir manualmente.
8. Como gerente, quero que uma pesquisa sem resultados mostre uma mensagem clara e permita pesquisar novamente, sem interromper o fluxo.

### Problema 2 — Persistência e snapshot

9. Como gerente, quero que todo o cliente inserido via opção 4 seja imediatamente guardado em `Clientes.txt`, para que o registo esteja sempre completo e consistente.
10. Como gerente, quero que ao inserir um cliente com um ID não registado, o sistema peça o nome completo e o registe em `Clientes.txt` antes de continuar, para que nunca existam IDs anónimos no sistema.
11. Como gerente, quero que o ID de um novo cliente siga o formato de 6 dígitos numéricos de `Clientes.txt`, para que o ficheiro mantenha consistência estrutural.
12. Como gerente, quero que o estado das caixas seja gravado automaticamente quando saio do programa (opção 0), para que o estado seja preservado sem ação extra.
13. Como gerente, quero ter uma opção no menu para gravar o snapshot manualmente a qualquer momento, para que possa preservar o estado intermédio da simulação.
14. Como gerente, quero que ao reiniciar o programa o estado seja restaurado exatamente: instante atual, estado de cada caixa, cliente em atendimento com tempo restante, e todos os clientes em fila com os seus produtos específicos.
15. Como gerente, quero que os timestamps de entrada e início de atendimento de cada cliente sejam restaurados do snapshot, para que os tempos de espera finais sejam calculados corretamente.
16. Como gerente, quero ver o nome completo do cliente ao mostrar o estado das caixas (opção 2), para que possa identificar quem está em cada fila.
17. Como gerente, quero que o `Dados.txt` inicial contenha clientes reais de `Clientes.txt`, para que a simulação arranque com um estado realista e identificável.

---

## Implementation Decisions

### Módulos a Modificar

#### `catalog.h / catalog.c`
Nova função de pesquisa:
- `catalog_pesquisar(catalogo, termo, resultados, max)` — preenche `resultados` com ponteiros para `Produto` cujo nome contém `termo` (case-insensitive via `str_casecmp_local` de `utils.c`) ou cujo ID corresponde exatamente; devolve o número de resultados (limitado a `max`, sugerido 10); pesquisa por ID é exata, pesquisa por nome é substring.

#### `client_registry.h / client_registry.c`
Nova função de registo persistente:
- `registo_adicionar(registo, id, nome, filename)` — valida que `id` tem exatamente 6 dígitos numéricos (`strlen == 6 && isdigit` para cada char) e que `nome` não é vazio; adiciona à lista em memória (com `realloc` se necessário); faz append de `"ID\tNome\n"` ao ficheiro; devolve sucesso/erro.

#### `store.h / store.c`

**`inserir_novo_cliente`** — assinatura passa a receber `Produto*` e `nProdutos` do caller:
```
inserir_novo_cliente(sm, hash, id, nome, produtos, nProdutos)
```
O `store.c` deixa de chamar `catalog_obter_produtos_aleatorios` — toda a seleção de produtos é responsabilidade do `main.c`. Se `produtos == NULL` ou `nProdutos <= 0`, devolve `INSERIR_CLIENTE_INVALIDO`.

**`guardar_snapshot(filename, sm)`** — nova função que escreve o estado completo em `Dados.txt`. Formato proposto:

```
INSTANTE 42
CAIXAS 6
CAIXA 1 ABERTA Ana Silva
  EM_ATENDIMENTO 609140 Ana Rita Pires tempo_restante=4 entrada=0 inicio=38
    PRODUTO 10001 Azeitona Verde Retalhada 3.00 12.0 4 false
    PRODUTO 10002 Croutons Pao Tostado 1.69 6.2 3 false
  FILA 2
    CLIENTE 814991 Ricardo Andrade Matos entrada=25
      PRODUTO 10003 Azeitona Mista Britada 2.00 14.6 2 false
    CLIENTE 843818 Eduardo Carneiro Castro entrada=15
      PRODUTO 10005 Mini Picos Azeite 1.45 6.7 2 false
CAIXA 2 FECHADA Joana Gomes
  FILA 0
CAIXA 3 A_FECHAR Pedro Costa
  FILA 1
    CLIENTE 647507 Cristiana Eanes Baptista entrada=10
      PRODUTO 10001 Azeitona Verde Retalhada 3.00 12.0 4 false
```

Todas as caixas são incluídas independentemente do estado. O cliente em atendimento é guardado com `tempo_restante`, `entrada` e `inicio`. Cada produto é guardado com todos os campos da `struct Produto` incluindo `oferecido`.

**`carregar_dados_iniciais(filename, sm, hash, registo)`** — assinatura recebe `RegistoClientes*`; lê o novo formato; para cada cliente, resolve o nome via `registo_pesquisar_id`; restaura `instanteAtual`, estado de cada caixa, cliente em atendimento com `tempoRestanteAtendimento` e timestamps, e fila completa com produtos.

#### `main.c`

**Opção 4 (inserir cliente)** — novo fluxo:
1. Pede ID → verifica hash → se já existe, oferece mover (comportamento atual).
2. Se ID novo: verifica registo → se não existe, pede nome e chama `registo_adicionar` antes de continuar (ou cancela se nome vazio).
3. Loop de construção do carrinho:
   - `"Pesquisar produto (nome/ID, Enter para terminar): "` → `catalog_pesquisar` → lista numerada → manager escolhe → produto adicionado a array local → repete.
   - Sem resultados: `"Nenhum produto encontrado."` → volta ao prompt.
   - Enter com texto vazio: termina o loop.
4. Se carrinho vazio: `"Carrinho vazio, insercao cancelada."`.
5. Mostra resumo do carrinho; chama `inserir_novo_cliente` com `Produto*` e `nProdutos`.

**Opção 0 (sair)** — chama `guardar_snapshot` antes de destruir estruturas.

**Nova opção de menu** (opção 14) — `"Gravar snapshot do estado atual"` — chama `guardar_snapshot` imediatamente.

**`carregar_dados_iniciais`** — chamada no arranque passa também `registo`.

---

### Decisões Arquiteturais

- **Registo obrigatório ao inserir:** todo o cliente que entra no sistema via opção 4 fica registado em `Clientes.txt` — garante que o snapshot nunca tem IDs desconhecidos e que o carregamento é sempre determinístico.
- **`inserir_novo_cliente` não gera produtos:** recebe `Produto*` pronto — separação de responsabilidades; `store.c` não conhece o catálogo para efeitos de seleção.
- **Snapshot inclui todos os campos temporais:** `instanteEntradaFila`, `instanteInicioAtendimento` e `tempoRestanteAtendimento` são gravados e restaurados; os tempos de espera finais ficam corretos após restauro.
- **Snapshot inclui todas as caixas:** independentemente do estado (ABERTA, FECHADA, A_FECHAR) — o estado do sistema é integral.
- **Formato do snapshot é legível por humanos:** facilita depuração e edição manual do estado inicial; linhas com prefixos (`CAIXA`, `CLIENTE`, `PRODUTO`, etc.) simplificam o parsing.
- **`Dados.txt` atual substituído:** como parte da implementação, o ficheiro existente com IDs fictícios é substituído por um snapshot válido gerado a partir de clientes reais de `Clientes.txt`.
- **Pesquisa case-insensitive:** reutiliza `str_casecmp_local` já existente em `utils.c` ou normalização para minúsculas numa cópia local.

---

## Testing Decisions

**O que faz um bom teste:** verifica comportamento observável através da interface pública — o que a função devolve e o estado das estruturas após a operação — sem depender de detalhes internos.

### Módulos a testar

| Módulo | Comportamentos a testar |
|--------|------------------------|
| `catalog_pesquisar` | Nome parcial devolve resultados corretos; ID exato devolve só esse produto; sem resultados devolve 0; limite `max` respeitado; pesquisa case-insensitive |
| `registo_adicionar` | ID de 6 dígitos válido é adicionado em memória e no ficheiro; ID com menos/mais de 6 chars rejeitado; ID não numérico rejeitado; nome vazio rejeitado; append preserva entradas existentes |
| `guardar_snapshot` + `carregar_dados_iniciais` | Snapshot gravado e relido restaura mesmo `instanteAtual`; mesmo número de clientes por caixa; produtos exatos preservados; `tempoRestanteAtendimento` restaurado; estado de caixas preservado |
| `inserir_novo_cliente` (nova assinatura) | Recebe `Produto*` e `nProdutos`; cliente inserido tem exatamente esses produtos; duplicado e sem caixa mantêm comportamento atual; `produtos == NULL` devolve `INSERIR_CLIENTE_INVALIDO` |

**Prior art:** `tests/test_issue2.c` (testes unitários em C com macro `CHECK`), `tests/test_issue3_insert.sh` (testes de comportamento com input pipe).

---

## Out of Scope

- Edição ou remoção de clientes já registados em `Clientes.txt`.
- Persistência das estatísticas acumuladas (`totalClientesAtendidos`, `somaTemposEspera`, etc.) no snapshot — apenas o estado das filas é persistido.
- Gestão de stock — o campo `stock` de `Produtos.txt` é lido mas não decrementado.
- Pesquisa de produtos com filtros avançados (preço, categoria, stock).
- Ordenação dos resultados de pesquisa por relevância.
- Interface gráfica ou web.

---

## Further Notes

- **`Dados.txt` atual deve ser substituído** por um snapshot válido com IDs reais de `Clientes.txt` como parte da implementação — substituir `P1`, `P21`, etc. por IDs selecionados aleatoriamente do registo.
- **Carrinho vazio cancela a inserção** — o gerente pode ter pesquisado sem adicionar nada; a mensagem deve ser clara e não criar clientes com 0 produtos.
- **`registo_adicionar` é chamado antes da seleção de produtos** — se o registo falhar (ID inválido, ficheiro não acessível), a inserção é cancelada.
- **O campo `oferecido` de cada produto é gravado no snapshot** — ao restaurar, um cliente que já recebeu oferta não a recebe de novo.
