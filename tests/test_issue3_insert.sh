#!/usr/bin/env bash
# Testa o novo comportamento da opcao 4: inserir ou mover se ja em fila
set -e
cd "$(dirname "$0")/.."
mkdir -p output

GCC="/c/Program Files/CodeBlocks/MinGW/bin/gcc.exe"
echo "=== Compilando... ==="
"$GCC" src/*.c -I src -Wall -Wextra -std=c11 -g -o compra_aqui_test
echo "Compilacao OK"
echo ""

PASS=0; FAIL=0
check() {
    local desc="$1" file="$2" pattern="$3" invert="${4:-}"
    if [ -n "$invert" ]; then
        grep -qE "$pattern" "$file" 2>/dev/null \
            && { echo "FAIL: $desc"; FAIL=$((FAIL+1)); } \
            || { echo "PASS: $desc"; PASS=$((PASS+1)); }
    else
        grep -qE "$pattern" "$file" 2>/dev/null \
            && { echo "PASS: $desc"; PASS=$((PASS+1)); } \
            || { echo "FAIL: $desc"; echo "--- output ---"; cat "$file"; echo "--------------"; FAIL=$((FAIL+1)); }
    fi
}

cfg_bak=$(mktemp); dados_bak=$(mktemp)
cp data/Configuracao.txt "$cfg_bak"; cp data/Dados.txt "$dados_bak"
restore() { mv "$cfg_bak" data/Configuracao.txt; mv "$dados_bak" data/Dados.txt; rm -f compra_aqui_test; }
trap restore EXIT

cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 120
N_CAIXAS 3
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 0
CFG

cat > data/Dados.txt <<'DAT'
3
Caixa1 : 1
0
Caixa2 : 1
0
Caixa3 : 0
0
DAT

# --- Teste 1: inserir cliente novo ---
echo "--- INSERT-1: inserir cliente novo ---"
# opcao 4, ID=T001, nProdutos=2, sair
printf '4\nT001\n2\n0\n' | ./compra_aqui_test > output/test_ins1.txt 2>&1 || true
check "INSERT-1: cliente T001 colocado numa caixa" \
      output/test_ins1.txt "colocado na caixa"

# --- Teste 2: inserir mesmo ID -> deve perguntar se quer mover ---
echo ""
echo "--- INSERT-2: ID ja em fila -> pergunta mover ---"
# inserir T001, depois inserir T001 de novo -> diz que ja esta e pergunta se quer mover; responde n
printf '4\nT001\n2\n4\nT001\nn\n0\n' | ./compra_aqui_test > output/test_ins2.txt 2>&1 || true
check "INSERT-2: avisa que cliente ja esta em fila" \
      output/test_ins2.txt "ja est(a|á) (na caixa|em fila)"
check "INSERT-2: pergunta se quer mover" \
      output/test_ins2.txt "[Pp]retende mover|[Mm]over"

# --- Teste 3: inserir mesmo ID e aceitar mover ---
echo ""
echo "--- INSERT-3: ID ja em fila -> mover para outra caixa ---"
# inserir T002 na caixa (auto), depois tentar inserir T002 de novo -> aceitar mover para caixa 2
printf '4\nT002\n3\n4\nT002\ns\n2\n0\n' | ./compra_aqui_test > output/test_ins3.txt 2>&1 || true
check "INSERT-3: cliente movido com sucesso" \
      output/test_ins3.txt "movido com sucesso"

# --- Teste 4: opcao 4 sem sugestao automatica do registo ---
echo ""
echo "--- INSERT-4: opcao 4 nao exibe sugestao automatica ---"
printf '4\nT003\n1\n0\n' | ./compra_aqui_test > output/test_ins4.txt 2>&1 || true
check "INSERT-4: sem 'Sugestao do registo' no output" \
      output/test_ins4.txt "Sugest" "invert"

echo ""
echo "=== RESULTADOS: $PASS passou(aram), $FAIL falhou(aram) ==="
[ "$FAIL" -eq 0 ] && echo "Todos os testes passaram!" && exit 0
exit 1
