#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
mkdir -p output

gcc src/*.c -I src -Wall -Wextra -std=c11 -g -o compra_aqui

printf '9
P21
4
T100
2
9
T100
4
T100
2
4
BAD
0
5
T100
3
9
T100
13
0
' | ./compra_aqui > output/teste_funcional.txt

cp data/Configuracao.txt /tmp/projetoed_config_backup.txt
cat > data/Configuracao.txt <<CFG
MAX_ESPERA 1
N_CAIXAS 6
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 0
CFG
printf '3
50
12
13
0
' | ./compra_aqui > output/teste_ofertas.txt
mv /tmp/projetoed_config_backup.txt data/Configuracao.txt

grep -q 'Cliente P21 encontrado na caixa 2' output/teste_funcional.txt
grep -q 'Cliente colocado na caixa' output/teste_funcional.txt
grep -q 'Cliente T100 encontrado na caixa' output/teste_funcional.txt
grep -q 'Cliente ja existe em espera.' output/teste_funcional.txt
grep -q 'Dados invalidos' output/teste_funcional.txt
grep -q 'Cliente movido com sucesso.' output/teste_funcional.txt
grep -q 'Ficheiros CSV gerados em output/.' output/teste_funcional.txt
grep -q 'Produtos oferecidos:' output/teste_ofertas.txt
grep -q 'Valor oferecido:' output/teste_ofertas.txt

echo 'Todos os testes terminaram sem erro.'
