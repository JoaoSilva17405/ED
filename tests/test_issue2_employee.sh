#!/usr/bin/env bash
# Tests for Issue 2: Módulo employee - operadores reais de funcionarios.txt
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
            && { echo "FAIL: $desc"; cat "$file" | grep -E "$pattern" || true; FAIL=$((FAIL+1)); } \
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

# ── Teste 1: caixas NAO mostram "Operador_N" ─────────────────────────────────
echo "--- EMPLOYEE-1: estado das caixas nao usa Operador_N ---"
cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 120
N_CAIXAS 6
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 3
CFG
# Mostrar estado (opcao 2), sair (0)
printf '2\n0\n' | ./compra_aqui_test > output/test_emp1.txt 2>&1 || true
# Nao deve conter "operador=Operador_"
check "EMPLOYEE-1: sem nomes genericos Operador_N" \
      output/test_emp1.txt "operador=Operador_[0-9]" "invert"

# ── Teste 2: estatisticas mostram nome real do operador ───────────────────────
echo ""
echo "--- EMPLOYEE-2: estatisticas mostram nome real ---"
cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 120
N_CAIXAS 6
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 3
CFG
# Avancar 50 passos, mostrar estatisticas, sair
printf '3\n50\n12\n0\n' | ./compra_aqui_test > output/test_emp2.txt 2>&1 || true
# Deve conter "Operador que atendeu menos pessoas: <nome com espaco>"
# Nome com espaco implica que nao e "Operador_N" (que nao tem espaco antes do _)
check "EMPLOYEE-2: operador com menos atendimentos tem nome real (com espaco)" \
      output/test_emp2.txt "Operador que atendeu menos pessoas: [A-Za-z]+ [A-Za-z]+"

# ── Resultados ────────────────────────────────────────────────────────────────
echo ""
echo "=== RESULTADOS: $PASS passou(aram), $FAIL falhou(aram) ==="
[ "$FAIL" -eq 0 ] && echo "Todos os testes passaram!" && exit 0
exit 1
