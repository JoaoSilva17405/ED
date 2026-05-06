# PRD — Correcção de 8 Bugs (Issue #10)

## Problem Statement

O simulador do supermercado contém oito defeitos que comprometem a correcteza da simulação e a integridade dos dados:

1. A função pública `inserir_novo_cliente()` aceita qualquer ID não vazio, ao contrário do menu que exige exactamente 6 dígitos — duas regras diferentes para o mesmo campo.
2. A opção 4 do menu permite inserir um cliente com nome vazio caso o utilizador pressione Enter sem escrever nada, criando entradas anónimas na simulação.
3. Um cliente que termina a fase de compras é sempre removido de `clientesEmLoja`, mesmo que não exista caixa disponível nem espaço na fila — o cliente desaparece silenciosamente da simulação.
4. O fecho suave manual de uma caixa já vazia (sem fila e sem cliente em atendimento) deixa a caixa presa no estado `CAIXA_A_FECHAR` indefinidamente, sem nunca transitar para `CAIXA_FECHADA`.
5. `abrir_caixa_manual()` atribui um novo operador e reabre qualquer caixa independentemente do estado actual — uma caixa já aberta troca de operador sem razão, e uma caixa a encerrar é reaberta sem aviso.
6. O critério de escolha para fecho automático de caixa usa "menor fila" (`fila.tamanho`) em vez de "menos pessoas" (fila + cliente em atendimento), podendo fechar uma caixa que está a servir um cliente enquanto outra apenas tem pessoas em espera.
7. A verificação de `MAX_ESPERA` para clientes em atendimento usa `instanteAtual - instanteEntradaFila`, um valor que cresce durante todo o atendimento, em vez do tempo real de espera na fila, que é fixo no momento em que o atendimento começa.
8. Ao carregar um snapshot, os campos `produtosOferecidos`, `valorOferecido` e `oferecimentoFeito` de cada cliente não são recalculados a partir dos produtos marcados como `oferecido=true`, ficando a zero mesmo que produtos já tenham sido oferecidos.

## Solution

Corrigir cada um dos oito defeitos de forma cirúrgica, sem alterar a arquitectura existente:

- Propagar `id_valido()` para o interior de `inserir_novo_cliente()`, tornando a validação de ID consistente em todo o código.
- Bloquear a inserção no menu quando o nome está vazio, com mensagem de erro clara.
- Tornar a remoção de `clientesEmLoja` condicional ao sucesso da inserção na fila — cliente só sai da loja quando entra numa fila.
- Completar `fechar_caixa_suave_manual()` com o caminho de fecho imediato para caixas já vazias.
- Restringir `abrir_caixa_manual()` a caixas em estado `CAIXA_FECHADA`, devolvendo erro com mensagem adequada para os outros estados.
- Substituir o critério de fecho automático por `encontrar_caixa_menos_pessoas_aberta()`.
- Fixar o cálculo de espera para clientes em atendimento usando `instanteInicioAtendimento - instanteEntradaFila`.
- Criar `recomputar_ofertas_cliente()` e chamá-la após reconstruir cada cliente a partir do snapshot.

## User Stories

1. Como gerente do supermercado, quero que a função de inserção de clientes valide o ID com as mesmas regras que o menu, para que não seja possível introduzir clientes com IDs inválidos por nenhum caminho.
2. Como utilizador do menu, quero que a opção "Inserir novo cliente" me informe claramente quando o ID tem formato errado, para que possa corrigi-lo imediatamente.
3. Como utilizador do menu, quero que a inserção de um cliente seja bloqueada se não fornecer um nome, para que não existam clientes anónimos na simulação.
4. Como utilizador do menu, quero receber uma mensagem de erro clara quando submeto um nome vazio, para que saiba que tenho de preencher o campo.
5. Como simulador, quero que um cliente que termina as compras só seja removido da loja quando entra efectivamente numa fila de caixa, para que nenhum cliente desapareça silenciosamente da simulação.
6. Como simulador, quero que um cliente que não consegue entrar numa fila permaneça na loja e tente novamente no próximo instante, para que o fluxo de clientes seja realista e sem perdas.
7. Como gerente, quero que fechar suavemente uma caixa vazia a feche de imediato, para que o estado do supermercado não contenha caixas presas em `A_FECHAR` sem razão.
8. Como utilizador do menu, quero que ao fechar suavemente uma caixa já vazia receba confirmação de que a caixa foi fechada imediatamente, para ter feedback correcto.
9. Como gerente, quero que tentar abrir uma caixa já aberta não produza efeito, para que o operador não seja substituído acidentalmente.
10. Como gerente, quero que tentar abrir uma caixa em estado "a encerrar" me informe que essa caixa está a encerrar e me instrua a escolher uma caixa fechada, para que a acção seja clara e não produza estados incoerentes.
11. Como gerente, quero que o sistema feche automaticamente a caixa com menos pessoas (fila + atendimento), para que a caixa escolhida seja sempre a que tem menos carga real.
12. Como simulador, quero que a oferta de produto por `MAX_ESPERA` em clientes em atendimento seja baseada no tempo real de espera na fila, para que o critério seja consistente com o enunciado.
13. Como simulador, quero que um cliente que esperou tempo suficiente na fila receba exactamente um produto oferecido, independentemente de quanto tempo demora o atendimento, para que o mecanismo de compensação funcione correctamente.
14. Como operador de snapshot, quero que ao carregar um ficheiro de estado os campos de ofertas de cada cliente sejam recalculados a partir dos produtos marcados, para que a simulação retome correctamente após uma paragem.
15. Como operador de snapshot, quero que os totais do supermercado (`totalProdutosOferecidos`, `totalValorOferecido`) continuem correctos após carregar um snapshot, para que as estatísticas finais sejam fiáveis.

## Implementation Decisions

### Módulos a modificar

**`store.c` — função `inserir_novo_cliente`**
- Substituir a guarda `id[0] == '\0'` por `!id_valido(id)`.
- Nenhuma alteração de interface — assinatura e códigos de retorno mantêm-se.

**`main.c` — case 4 (opção "Inserir novo cliente")**
- Após `ler_string` do nome, se `nome[0] == '\0'`: imprimir mensagem de erro e `break`.
- O bloco de registo (`registo_adicionar`) só é chamado quando o nome é não vazio.

**`store.c` — função `avancar_simulacao`**
- Remover as linhas `c->comprasTerminadas = true` e `c->instanteEntradaFila = sm->instanteAtual` da posição anterior à verificação de destino.
- Envolver a remoção de `clientesEmLoja` (swap-with-last) dentro do `if (destino >= 0 && fila_inserir(...))`.
- No ramo de sucesso: definir `comprasTerminadas`, `instanteEntradaFila`, `caixaAtual`, actualizar hash, registar log.
- No ramo de falha: incrementar `j` e não remover o cliente.

**`store.c` — função `fechar_caixa_suave_manual`**
- Após definir `CAIXA_A_FECHAR`, verificar se `fila.tamanho == 0 && emAtendimento == NULL`.
- Se sim: transitar directamente para `CAIXA_FECHADA`, limpar `operador[0] = '\0'`, registar log distinto (`FECHAR_CAIXA_IMEDIATA_MANUAL`).

**`store.c` — função `abrir_caixa_manual`**
- Adicionar guarda: se `estado == CAIXA_ABERTA`, retornar 0.
- Adicionar guarda: se `estado == CAIXA_A_FECHAR`, imprimir mensagem `"Caixa seleccionada encontra-se a encerrar, escolha uma caixa fechada para abrir.\n"` e retornar 0.
- Apenas prosseguir (atribuir operador, abrir, registar) se `estado == CAIXA_FECHADA`.

**`store.c` — função `verificar_politica_caixas`**
- Substituir `encontrar_caixa_menos_fila_aberta(sm)` por `encontrar_caixa_menos_pessoas_aberta(sm, -1)`.

**`store.c` — função `avancar_simulacao` (bloco emAtendimento / MAX_ESPERA)**
- Substituir `int espera = sm->instanteAtual - c->instanteEntradaFila` por:
  `int espera = (c->instanteInicioAtendimento >= 0) ? c->instanteInicioAtendimento - c->instanteEntradaFila : sm->instanteAtual - c->instanteEntradaFila;`

**`store.c` / `customer.c` — nova função `recomputar_ofertas_cliente`**
- Função estática em `store.c` (usada apenas aí): percorre `cliente->produtos`, conta os marcados como `oferecido=true`, acumula preços, define `oferecimentoFeito`.
- Chamada no final de `finalizar_cliente_loja()` e `finalizar_cliente_pendente()`.

### Decisões de comportamento

- Um cliente que não consegue entrar numa fila permanece em `clientesEmLoja` com `comprasTerminadas = false` e tentará novamente no próximo passo — não há limite de tentativas.
- O fecho suave manual de uma caixa vazia fecha-a imediatamente e limpa o operador, exactamente como o fecho automático.
- `abrir_caixa_manual` com caixa `CAIXA_A_FECHAR`: mensagem de erro é impressa dentro da função (não no menu), e retorna 0 para que o menu exiba a mensagem genérica de falha — **não**: a mensagem é impressa dentro da função antes de retornar 0, para ser mais específica que a mensagem genérica do menu.
- O cálculo de espera frozen para emAtendimento usa o `else` defensivo (`instanteAtual - instanteEntradaFila`) para o caso em que `instanteInicioAtendimento` ainda seja -1 por algum motivo de carregamento.

## Testing Decisions

### O que constitui um bom teste

Testar comportamento externo observável, não detalhes de implementação:
- Resultado de retorno de funções
- Estado do `Supermercado` após chamadas
- Mensagens impressas (para o menu)
- Campos do `Cliente` após operações

### Módulos a testar

| Módulo / Função | O que testar |
|---|---|
| `inserir_novo_cliente` | Rejeita ID com 5 dígitos, rejeita ID não numérico, aceita ID com 6 dígitos |
| `avancar_simulacao` (bug #4) | Cliente permanece em `clientesEmLoja` quando todas as caixas estão fechadas |
| `fechar_caixa_suave_manual` | Caixa vazia fecha imediatamente; caixa com fila fica em `A_FECHAR` |
| `abrir_caixa_manual` | Retorna 0 para `ABERTA`; retorna 0 para `A_FECHAR`; abre e atribui operador para `FECHADA` |
| `verificar_politica_caixas` | Fecha a caixa com cliente em atendimento quando outra só tem fila |
| Bloco MAX_ESPERA emAtendimento | Oferta não é duplicada se já foi feita na fila; oferta usa espera frozen |
| `recomputar_ofertas_cliente` | Campos recalculados correctamente após load de snapshot com `oferecido=true` |

### Arte anterior

Os testes existentes em `tests/` usam `assert()` directo com setup manual de `Supermercado` + `HashClientes`. Seguir o mesmo padrão.

## Out of Scope

- Alteração do formato do snapshot para guardar `instanteInicioAtendimento` (os campos já existem e são guardados).
- Suporte a múltiplos produtos oferecidos por cliente (o mecanismo de oferta único mantém-se).
- Refactoring de `verificar_politica_caixas` para suportar estratégias configuráveis.
- Introdução de testes de integração end-to-end com entrada simulada via stdin.

## Further Notes

- O bug #4 tem uma subtileza: quando `comprasTerminadas` era definido a `true` antes da tentativa de inserção, o cliente podia entrar no ciclo de atendimento com `instanteEntradaFila` errado. Com a correcção, esses campos só são definidos após inserção bem-sucedida.
- O bug #7 não afecta clientes ainda na fila (o bloco da fila já usa `instanteAtual - instanteEntradaFila` correctamente, pois enquanto na fila esse valor representa a espera real).
- A numeração dos bugs no docx salta do #8 para o #10 — não existe bug numerado como #9 no documento original.
