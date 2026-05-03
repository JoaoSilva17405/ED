# Compra Aqui Lda. - Projeto Prático em C

Projeto em C para gestão de caixas de atendimento de supermercado.

## Estrutura

- `src/` código fonte modular
- `data/Configuracao.txt` parâmetros do sistema
- `data/Dados.txt` estado inicial das caixas
- `output/` CSV gerados pelo programa
- `ProjetoED.cbp` projeto para Code::Blocks

## Compilar com GCC

```bash
gcc src/*.c -I src -Wall -Wextra -std=c11 -g -o compra_aqui
```

## Executar

```bash
./compra_aqui
```

## Nota importante

O programa lê os ficheiros com caminhos relativos:

- `data/Configuracao.txt`
- `data/Dados.txt`
- `output/historico_utilizador.csv`
- `output/estatisticas.csv`
- `output/historico_caixas.csv`

Por isso, deve ser executado a partir da raiz do projeto.

## Funcionalidades implementadas

- carregar configuração e dados iniciais de ficheiro
- filas por caixa com lista ligada
- pesquisa de pessoas com hashing
- avanço da simulação por instantes de tempo
- abertura automática e manual de caixas
- fecho suave e fecho imediato com redistribuição
- mudança de cliente entre filas
- cálculo de produtos oferecidos e respetivo custo
- estatísticas finais
- histórico das ações do utilizador em CSV
- cálculo aproximado de memória usada e desperdiçada

## Testes funcionais incluídos

O ficheiro `run_tests.sh` compila o projeto e valida automaticamente estes cenários principais:

- pesquisa de cliente carregado de `Dados.txt`;
- inserção de novo cliente;
- rejeição de cliente duplicado;
- rejeição de número de produtos inválido;
- mudança de cliente entre caixas;
- geração dos CSV;
- simulação com `MAX_ESPERA` reduzido para validar oferta de produtos.

Em Windows/Code::Blocks, os mesmos cenários podem ser testados manualmente pelo menu.
