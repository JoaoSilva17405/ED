#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"
#include "hash.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define FLOAT_EQ(a, b) ((a) > (b) - 0.01f && (a) < (b) + 0.01f)
#define TMP_STAT18 "output/test18_estatisticas.csv"

static Produto make_prod_preco(float tempoCompra, float tempoPassagem, float preco) {
    Produto p;
    memset(&p, 0, sizeof(p));
    p.id = 1;
    strncpy(p.nome, "Teste", sizeof(p.nome) - 1);
    p.preco = preco;
    p.tempoCompra   = tempoCompra;
    p.tempoPassagem = tempoPassagem;
    p.oferecido     = false;
    return p;
}

static int cli_seq = 0;

static Supermercado make_sm(int nCaixas) {
    Supermercado sm;
    Configuracao cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.maxEspera     = 120;
    cfg.nCaixas       = nCaixas;
    cfg.maxPreco      = 40.0f;
    cfg.maxFila       = 7;
    cfg.minFila       = 3;
    cfg.minProdutos   = 1;
    cfg.maxProdutos   = 20;
    supermercado_init(&sm, &cfg, NULL, NULL, NULL);
    return sm;
}

static int file_contains(const char *filename, const char *substring) {
    char line[512];
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, substring)) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 1 — totalValorVendido acumulado em concluir_atendimento
   ══════════════════════════════════════════════════════════════════════════ */

static void test_total_valor_vendido_zero_inicial(void) {
    Supermercado sm = make_sm(1);
    CHECK(FLOAT_EQ(sm.totalValorVendido, 0.0f),
          "init: totalValorVendido inicializado a 0.0");
    supermercado_destruir(&sm, NULL);
}

static void test_total_valor_vendido_acumulado(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[2];
    char id[MAX_ID];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op1", MAX_OPERADOR - 1);

    /* cliente com 2 produtos a 3.00€ cada = 6.00€ total */
    prods[0] = make_prod_preco(1.0f, 1.0f, 3.0f);
    prods[1] = make_prod_preco(1.0f, 1.0f, 3.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cliente1", prods, 2);
    avancar_simulacao(&sm, &hash, 3);

    CHECK(sm.caixas[0].nHistorico == 1,
          "acumulado: cliente atendido no historico");
    CHECK(FLOAT_EQ(sm.totalValorVendido, 6.0f),
          "acumulado: totalValorVendido == 6.00 apos 2 produtos a 3.00");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_total_valor_vendido_dois_clientes(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[1];
    char id[MAX_ID];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op1", MAX_OPERADOR - 1);

    /* cliente 1: 1 produto a 4.50€ */
    prods[0] = make_prod_preco(1.0f, 1.0f, 4.5f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cliente1", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    /* cliente 2: 1 produto a 2.00€ */
    prods[0] = make_prod_preco(1.0f, 1.0f, 2.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cliente2", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    CHECK(FLOAT_EQ(sm.totalValorVendido, 6.5f),
          "dois clientes: totalValorVendido == 6.50 (4.50 + 2.00)");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 2 — total_valor_vendido exportado no CSV de estatisticas
   ══════════════════════════════════════════════════════════════════════════ */

static void test_csv_tem_coluna_total_valor_vendido(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[1];
    char id[MAX_ID];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op1", MAX_OPERADOR - 1);

    prods[0] = make_prod_preco(1.0f, 1.0f, 5.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cliente1", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    guardar_estatisticas_csv(TMP_STAT18, &sm);

    CHECK(file_contains(TMP_STAT18, "total_valor_vendido"),
          "csv: cabecalho contem total_valor_vendido");
    CHECK(file_contains(TMP_STAT18, "5.00"),
          "csv: valor 5.00 aparece na linha de dados");

    remove(TMP_STAT18);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ── runner ─────────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== Issue #18: Total Global de Valor Vendido ===\n\n");

    printf("-- Cycle 1: totalValorVendido acumulado --\n");
    test_total_valor_vendido_zero_inicial();
    test_total_valor_vendido_acumulado();
    test_total_valor_vendido_dois_clientes();

    printf("\n-- Cycle 2: exportado no CSV de estatisticas --\n");
    test_csv_tem_coluna_total_valor_vendido();

    printf("\n=== Resultados: %d PASS, %d FAIL ===\n", pass_count, fail_count);
    return fail_count > 0 ? 1 : 0;
}
