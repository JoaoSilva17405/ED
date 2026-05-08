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

#define TMP_STAT16 "output/test16_estatisticas.csv"

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
    cfg.maxEspera   = 120;
    cfg.nCaixas     = nCaixas;
    cfg.maxPreco    = 40.0f;
    cfg.maxFila     = 7;
    cfg.minFila     = 3;
    cfg.minProdutos = 1;
    cfg.maxProdutos = 20;
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

static int count_occurrences(const char *filename, const char *substring) {
    char line[512];
    int count = 0;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, substring)) count++;
    }
    fclose(f);
    return count;
}

static void serve_cliente(Supermercado *sm, HashClientes *hash,
                          const char *operador, float preco) {
    Produto prods[1];
    char id[MAX_ID];
    strncpy(sm->caixas[0].operador, operador, MAX_OPERADOR - 1);
    sm->caixas[0].operador[MAX_OPERADOR - 1] = '\0';
    prods[0] = make_prod_preco(1.0f, 1.0f, preco);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(sm, hash, id, "Cl", prods, 1);
    avancar_simulacao(sm, hash, 2);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 1 — secao por operador no CSV com cabecalho e dados corretos
   ══════════════════════════════════════════════════════════════════════════ */

static void test_csv_cabecalho_por_operador(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;

    serve_cliente(&sm, &hash, "Alice", 2.0f);

    guardar_estatisticas_csv(TMP_STAT16, &sm);

    CHECK(file_contains(TMP_STAT16, "operador,clientes_atendidos,produtos_vendidos,valor_vendido"),
          "csv operador: cabecalho da secao por operador presente");

    remove(TMP_STAT16);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_csv_dois_operadores_mesma_caixa(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;

    /* Alice: 1 cliente com 1 produto a 2.00 */
    serve_cliente(&sm, &hash, "Alice", 2.0f);
    /* Bob: 2 clientes com 1 produto a 2.00 cada */
    serve_cliente(&sm, &hash, "Bob", 2.0f);
    serve_cliente(&sm, &hash, "Bob", 2.0f);

    CHECK(sm.caixas[0].nHistorico == 3,
          "setup dois operadores: 3 clientes no historico");

    guardar_estatisticas_csv(TMP_STAT16, &sm);

    CHECK(file_contains(TMP_STAT16, "Alice,1,1,2.00"),
          "csv dois operadores: Alice com 1 cliente, 1 produto, 2.00");
    CHECK(file_contains(TMP_STAT16, "Bob,2,2,4.00"),
          "csv dois operadores: Bob com 2 clientes, 2 produtos, 4.00");
    /* cada nome deve aparecer exatamente 1 vez no ficheiro */
    CHECK(count_occurrences(TMP_STAT16, "Alice") == 1,
          "csv dois operadores: Alice aparece exatamente 1 vez");
    CHECK(count_occurrences(TMP_STAT16, "Bob") == 1,
          "csv dois operadores: Bob aparece exatamente 1 vez");

    remove(TMP_STAT16);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 2 — secao por caixa nao e afetada; secao por operador adicionada
   ══════════════════════════════════════════════════════════════════════════ */

static void test_csv_secao_caixa_mantida(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;

    serve_cliente(&sm, &hash, "Alice", 2.0f);

    guardar_estatisticas_csv(TMP_STAT16, &sm);

    /* secao por caixa continua presente */
    CHECK(file_contains(TMP_STAT16, "caixa,operador,clientes_atendidos,produtos_vendidos,valor_vendido"),
          "csv: secao por caixa ainda presente no CSV");
    /* secao por operador foi adicionada */
    CHECK(file_contains(TMP_STAT16, "operador,clientes_atendidos,produtos_vendidos,valor_vendido"),
          "csv: secao por operador adicionada sem remover a secao por caixa");

    remove(TMP_STAT16);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 3 — agregacao cross-caixa: mesmo operador em duas caixas distintas
   ══════════════════════════════════════════════════════════════════════════ */

static void test_csv_mesmo_operador_em_duas_caixas(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[1];
    char id[MAX_ID];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    sm.caixas[1].estado = CAIXA_ABERTA;

    /* Alice serve 1 cliente na caixa 0 */
    strncpy(sm.caixas[0].operador, "Alice", MAX_OPERADOR - 1);
    sm.caixas[0].operador[MAX_OPERADOR - 1] = '\0';
    prods[0] = make_prod_preco(1.0f, 1.0f, 3.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cl", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    /* Alice serve 1 cliente na caixa 1 (mesma operadora, caixa diferente) */
    strncpy(sm.caixas[1].operador, "Alice", MAX_OPERADOR - 1);
    sm.caixas[1].operador[MAX_OPERADOR - 1] = '\0';
    prods[0] = make_prod_preco(1.0f, 1.0f, 3.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cl", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    guardar_estatisticas_csv(TMP_STAT16, &sm);

    /* Alice deve aparecer 1 vez com totais agregados (2 clientes, 2 produtos, 6.00) */
    CHECK(count_occurrences(TMP_STAT16, "Alice") == 1,
          "cross-caixa: Alice aparece uma unica vez nos dados");
    CHECK(file_contains(TMP_STAT16, "Alice,2,2,6.00"),
          "cross-caixa: Alice com totais agregados de 2 caixas (2 clientes, 2 produtos, 6.00)");

    remove(TMP_STAT16);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 4 — minimo por operador real (nao por caixa)
             Verifica que a identificacao do minimo usa os totais corretos
             por operador real e nao o ultimo operador da caixa
   ══════════════════════════════════════════════════════════════════════════ */

static void test_csv_valores_para_minimo_corretos(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;

    /* Alice serve 1 cliente → minimo real */
    serve_cliente(&sm, &hash, "Alice", 1.0f);
    /* Bob serve 3 clientes → nao e o minimo */
    serve_cliente(&sm, &hash, "Bob", 1.0f);
    serve_cliente(&sm, &hash, "Bob", 1.0f);
    serve_cliente(&sm, &hash, "Bob", 1.0f);

    guardar_estatisticas_csv(TMP_STAT16, &sm);

    /* Alice: 1 cliente (o minimo real) */
    CHECK(file_contains(TMP_STAT16, "Alice,1,1,1.00"),
          "minimo real: Alice com 1 cliente (o minimo) presente no CSV");
    /* Bob: 3 clientes (nao e o minimo) */
    CHECK(file_contains(TMP_STAT16, "Bob,3,3,3.00"),
          "minimo real: Bob com 3 clientes presente no CSV");

    remove(TMP_STAT16);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ── runner ─────────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== Issue #16: Estatisticas por Operador On-Demand ===\n\n");

    printf("-- Cycle 1: secao por operador no CSV --\n");
    test_csv_cabecalho_por_operador();
    test_csv_dois_operadores_mesma_caixa();

    printf("\n-- Cycle 2: secao por caixa mantida --\n");
    test_csv_secao_caixa_mantida();

    printf("\n-- Cycle 3: agregacao cross-caixa --\n");
    test_csv_mesmo_operador_em_duas_caixas();

    printf("\n-- Cycle 4: dados para identificacao do minimo --\n");
    test_csv_valores_para_minimo_corretos();

    printf("\n=== Resultados: %d PASS, %d FAIL ===\n", pass_count, fail_count);
    return fail_count > 0 ? 1 : 0;
}
