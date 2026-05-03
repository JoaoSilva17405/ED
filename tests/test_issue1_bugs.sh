#!/usr/bin/env bash
# Tests for Issue 1: Bug Fixes (BUG-1 to BUG-5)
set -e
cd "$(dirname "$0")/.."
mkdir -p output

GCC="/c/Program Files/CodeBlocks/MinGW/bin/gcc.exe"
echo "=== Compilando... ==="
"$GCC" src/*.c -I src -Wall -Wextra -std=c11 -g -o compra_aqui_test
echo "Compilacao OK"
echo ""

PASS=0
FAIL=0

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
cp data/Configuracao.txt "$cfg_bak"
cp data/Dados.txt        "$dados_bak"

restore() { mv "$cfg_bak" data/Configuracao.txt; mv "$dados_bak" data/Dados.txt; rm -f compra_aqui_test; }
trap restore EXIT

# ── BUG-4: passos <= 0 deve ser rejeitado ─────────────────────────────────────
echo "--- BUG-4: passos <= 0 rejeitado ---"
cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 120
N_CAIXAS 6
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 3
CFG
printf '3\n0\n0\n' | ./compra_aqui_test > output/test_bug4.txt 2>&1 || true
check "BUG-4: passos=0 produz mensagem de erro" \
      output/test_bug4.txt "invalido|maior que zero|positivo|nao valido"

# ── BUG-1: exatamente 1 produto oferecido por cliente ─────────────────────────
echo ""
echo "--- BUG-1: exatamente 1 produto oferecido ---"
cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 1
N_CAIXAS 6
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 0
CFG
# Caixa1: LongA (100 produtos em servico) + QueuedB (3 produtos na fila)
# QueuedB espera >> MAX_ESPERA; deve receber exatamente 1 oferta
cat > data/Dados.txt <<'DAT'
1
Caixa1 : 1
2
LongA : 100
QueuedB : 3
DAT
printf '3\n10\n12\n0\n' | ./compra_aqui_test > output/test_bug1.txt 2>&1 || true
check "BUG-1: exatamente 1 produto oferecido (nao 3)" \
      output/test_bug1.txt "Produtos oferecidos: 1$"

# ── BUG-2: MIN_FILA nao fecha a ultima caixa verdadeiramente aberta ────────────
echo ""
echo "--- BUG-2: MIN_FILA nao fecha ultima caixa aberta ---"
cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 120
N_CAIXAS 2
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 10
CFG
# 2 caixas abertas, 0 clientes -> avg=0 < MIN_FILA=10 -> politica tenta fechar
# Fechar suavemente Caixa1 -> so Caixa2 e verdadeiramente ABERTA
# Com o bug: Caixa2 tambem fecha; com o fix: Caixa2 fica ABERTA
cat > data/Dados.txt <<'DAT'
2
Caixa1 : 1
0
Caixa2 : 1
0
DAT
# 7=fechar suave, 1=caixa1, 3=avancar, 1=1 passo, 2=mostrar estado, 0=sair
printf '7\n1\n3\n1\n2\n0\n' | ./compra_aqui_test > output/test_bug2.txt 2>&1 || true
check "BUG-2: Caixa2 permanece ABERTA apos politica MIN_FILA" \
      output/test_bug2.txt "estado=ABERTA"

# ── BUG-3: totalProdutosVendidos nao inclui produtos oferecidos ────────────────
echo ""
echo "--- BUG-3: totalProdutosVendidos exclui oferecidos ---"
cat > data/Configuracao.txt <<'CFG'
MAX_ESPERA 1
N_CAIXAS 6
TEMPO_ATENDIMENTO_PRODUTO 6
MAX_PRECO 40
MAX_FILA 7
MIN_FILA 0
CFG
# LongA (3 prods) em servico; QueuedB (2 prods) na fila -> 1 produto oferecido
# Apos ambos serem atendidos:
#   Fix:  vendidos = 3(LongA) + 1(QueuedB) = 4
#   Bug:  vendidos = 3(LongA) + 2(QueuedB) = 5
cat > data/Dados.txt <<'DAT'
1
Caixa1 : 1
2
LongA : 3
QueuedB : 2
DAT
printf '3\n100\n12\n0\n' | ./compra_aqui_test > output/test_bug3.txt 2>&1 || true
check "BUG-3: Total de produtos vendidos = 4 (exclui o oferecido)" \
      output/test_bug3.txt "Total de produtos vendidos: 4"

# ── Resultados ─────────────────────────────────────────────────────────────────
echo ""
echo "=== RESULTADOS: $PASS passou(aram), $FAIL falhou(aram) ==="
[ "$FAIL" -eq 0 ] && echo "Todos os testes passaram!" && exit 0
exit 1
