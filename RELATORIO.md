# Relatório - Compra Aqui Lda.

## 1. Objetivo

Desenvolver um programa em linguagem C que permita gerir caixas de atendimento de um supermercado, acompanhar filas, abrir e fechar caixas e recolher estatísticas da simulação.

## 2. Estruturas de dados utilizadas

### 2.1 Filas de espera
Cada caixa possui uma fila de clientes implementada com lista ligada simples, com cabeça e cauda.

### 2.2 Hashing
Os clientes em espera são indexados numa tabela hash para permitir pesquisa rápida por identificador.

### 2.3 Vetores dinâmicos
Cada caixa guarda o histórico de clientes atendidos num vetor dinâmico de apontadores.

### 2.4 Ficheiros
São usados ficheiros de texto para entrada (`Configuracao.txt`, `Dados.txt`) e ficheiros CSV para saída e auditoria.

## 3. Dependências entre as entidades

```text
Supermercado
 ├── Configuracao
 ├── Caixa[]
 │    ├── FilaClientes
 │    │    └── NoFila -> Cliente -> Produto[]
 │    ├── emAtendimento -> Cliente
 │    └── historicoAtendidos -> Cliente*
 └── HashClientes
      └── NoHash -> Cliente
```

## 4. Funcionalidades implementadas

1. Carregar os dados a partir de ficheiros.
2. Gravar estatísticas e históricos em CSV.
3. Manter histórico das ações do utilizador.
4. Mudar um cliente de caixa.
5. Abrir uma nova caixa manualmente ou automaticamente.
6. Fechar uma caixa suavemente quando a média desce abaixo de `MIN_FILA`.
7. Fechar uma caixa imediatamente e redistribuir as pessoas.
8. Pesquisar pessoa em espera.
9. Determinar a memória utilizada.
10. Determinar a memória desperdiçada.
11. Calcular medidas de desempenho finais.

## 5. Política da simulação

A simulação avança por instantes de tempo. Em cada instante:
- uma caixa aberta inicia atendimento ao primeiro cliente da fila, se estiver livre;
- o tempo restante do atendimento é decrementado;
- clientes que excedem `MAX_ESPERA` recebem a oferta de um produto;
- é verificada a política de abertura/fecho de caixas.

## 6. Memória utilizada e desperdiçada

### Memória utilizada
É estimada pela soma de:
- estrutura `Supermercado`
- vetor de `Caixa`
- nós das filas
- `Cliente`
- vetores de `Produto`
- estrutura hash e seus nós
- capacidade reservada nos históricos das caixas

### Memória desperdiçada
É estimada por:
- posições livres nos vetores dinâmicos de histórico
- buckets vazios da tabela hash
- filas alocadas mas sem uso em caixas fechadas

## 7. Horas aproximadas de execução

Estimativa razoável para um desenvolvimento deste trabalho:
- análise e modelação: 6 h
- implementação das estruturas: 10 h
- simulação e regras de negócio: 10 h
- menus, testes e correções: 8 h
- relatório e documentação: 4 h

**Total aproximado: 38 horas**

## 8. Observações finais

A implementação privilegia clareza, modularidade e uso de estruturas de dados alinhadas com os conteúdos da unidade curricular.
