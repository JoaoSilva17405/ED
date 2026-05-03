# Validação realizada

Esta versão foi corrigida especificamente após falha na funcionalidade de inserção de novo cliente.

## Testes executados

Foram executados testes automáticos com o programa compilado por GCC:

```bash
gcc src/*.c -I src -Wall -Wextra -std=c11 -g -o compra_aqui
./run_tests.sh
```

## Cenários validados

1. Pesquisa de cliente carregado do ficheiro:
   - entrada: `P21`
   - resultado esperado: cliente encontrado na caixa 2.

2. Inserção de novo cliente:
   - entrada: `T100`, 2 produtos
   - resultado esperado: cliente colocado numa caixa disponível.

3. Pesquisa do cliente recém-inserido:
   - entrada: `T100`
   - resultado esperado: cliente encontrado na caixa onde foi colocado.

4. Inserção duplicada:
   - entrada: `T100` novamente
   - resultado esperado: mensagem `Cliente ja existe em espera.`

5. Inserção inválida:
   - entrada: cliente com 0 produtos
   - resultado esperado: mensagem de dados inválidos.

6. Mudança de cliente de caixa:
   - entrada: mover `T100` para outra caixa aberta
   - resultado esperado: `Cliente movido com sucesso.`

7. Geração de CSV:
   - opção de gravação de estatísticas
   - resultado esperado: ficheiros CSV gerados em `output/`.

8. Política de oferta por espera excessiva:
   - configuração temporária com `MAX_ESPERA 1`
   - avanço de 50 instantes
   - resultado esperado: estatísticas mostram produtos e valor oferecido.

## Correções aplicadas

- `inserir_novo_cliente` passou a devolver códigos de erro específicos.
- O menu passou a distinguir cliente duplicado, dados inválidos, falta de caixa e falha de memória.
- A inserção passou a abrir uma caixa fechada se não existir caixa aberta disponível para receber novo cliente.
- `hash_inserir` passou a devolver sucesso/erro.
- A leitura de `Dados.txt` passou a tratar falha de inserção na fila/hash sem deixar estruturas incoerentes.
- O log de mudança de caixa passou a guardar a caixa de origem correta.
