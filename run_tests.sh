#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
mkdir -p output

gcc src/*.c -I src -Wall -Wextra -std=c11 -g -o compra_aqui

# Minimal clean snapshot: 1 caixa aberta, sem clientes
CLEAN_DADOS=$(cat <<'EOF'
INSTANTE 0
ULTIMO_FECHO -1
ULTIMA_ABERTURA_MANUAL -1
CAIXAS 6
CAIXA 1 ABERTA Operador_1
CAIXA 2 FECHADA Operador_2
CAIXA 3 FECHADA Operador_3
CAIXA 4 FECHADA Operador_4
CAIXA 5 FECHADA Operador_5
CAIXA 6 FECHADA Operador_6
EOF
)

# --- Functional test ---
# Uses ID 609140 (valid 6-digit, present in Clientes.txt → name auto-filled)
# Flow: insert → show state (1 client in loja) → try move (in loja → fails) →
#        search → duplicate insert → insert with non-numeric ID → CSV → exit
cp data/Dados.txt /tmp/projetoed_dados_backup.txt
echo "$CLEAN_DADOS" > data/Dados.txt

printf '4
609140
2
5
609140
2
9
609140
4
609140
4
ABCDEF
Nome Teste
13
0
' | ./compra_aqui > output/teste_funcional.txt

mv /tmp/projetoed_dados_backup.txt data/Dados.txt

# --- Offers test (MAX_ESPERA=1, advance 500 ticks) ---
cp data/Configuracao.txt /tmp/projetoed_config_backup.txt
cp data/Dados.txt /tmp/projetoed_dados_backup.txt
echo "$CLEAN_DADOS" > data/Dados.txt
cat > data/Configuracao.txt <<CFG
MAX_ESPERA 1
N_CAIXAS 2
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 0
MIN_PRODUTOS 1
MAX_PRODUTOS 3
CFG

printf '4
609140
3
500
12
13
0
' | ./compra_aqui > output/teste_ofertas.txt

mv /tmp/projetoed_config_backup.txt data/Configuracao.txt
mv /tmp/projetoed_dados_backup.txt data/Dados.txt

# --- Assertions ---
grep -q 'a fazer compras com' output/teste_funcional.txt
grep -q 'Clientes a fazer compras (1)' output/teste_funcional.txt
grep -q 'Nao foi possivel mover o cliente.' output/teste_funcional.txt
grep -q 'esta a fazer compras' output/teste_funcional.txt
grep -q 'ja esta no supermercado' output/teste_funcional.txt
grep -q 'ID invalido' output/teste_funcional.txt
grep -q 'Ficheiros CSV gerados em output/.' output/teste_funcional.txt
grep -q 'Produtos oferecidos:' output/teste_ofertas.txt
grep -q 'Valor oferecido:' output/teste_ofertas.txt

echo 'Todos os testes terminaram sem erro.'
