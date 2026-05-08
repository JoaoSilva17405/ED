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

#define TMP_HIST15 "output/test15_historico.csv"

static Produto make_prod(float tempoCompra, float tempoPassagem) {
    Produto p;
    memset(&p, 0, sizeof(p));
    p.id = 1;
    strncpy(p.nome, "Teste", sizeof(p.nome) - 1);
    p.preco = 2.0f;
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

static Cliente *make_cliente(int nProd, float tc, float tp) {
    Produto *prods;
    char id[MAX_ID];
    int i;
    prods = (Produto *)malloc(sizeof(Produto) * nProd);
    if (!prods) return NULL;
    for (i = 0; i < nProd; i++)
        prods[i] = make_prod(tc, tp);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    return criar_cliente(id, "Teste", nProd, 0, 0, prods);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 1 — operadorAtendimento e preenchido apos concluir_atendimento
   ══════════════════════════════════════════════════════════════════════════ */

static void test_operador_preenchido_apos_atendimento(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[1];
    char id[MAX_ID];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Alice", MAX_OPERADOR - 1);
    sm.caixas[0].operador[MAX_OPERADOR - 1] = '\0';

    prods[0] = make_prod(1.0f, 1.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id, "Cliente", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    CHECK(sm.caixas[0].nHistorico == 1,
          "operador: cliente aparece no historico apos atendimento");
    if (sm.caixas[0].nHistorico == 1) {
        CHECK(strcmp(sm.caixas[0].historicoAtendidos[0]->operadorAtendimento, "Alice") == 0,
              "operador: operadorAtendimento preenchido com operador ativo no momento");
    }

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_operador_campo_inicializado_vazio(void) {
    Produto prods[1];
    char id[MAX_ID];
    Cliente *c;
    prods[0] = make_prod(1.0f, 1.0f);
    snprintf(id, sizeof(id), "C%05d", ++cli_seq);
    c = criar_cliente(id, "Teste", 1, 0, 0, prods);
    CHECK(c != NULL, "criar_cliente: retorna nao-NULL");
    if (c) {
        CHECK(c->operadorAtendimento[0] == '\0',
              "criar_cliente: operadorAtendimento inicializado como string vazia");
        destruir_cliente(c);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 2 — dois operadores na mesma caixa -> CSV correto por cliente
   ══════════════════════════════════════════════════════════════════════════ */

static int file_contains(const char *filename, const char *substring) {
    char line[256];
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, substring)) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

static int count_occurrences(const char *filename, const char *substring) {
    char line[256];
    int count = 0;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, substring)) count++;
    }
    fclose(f);
    return count;
}

static void test_csv_usa_operador_por_cliente(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[1];
    char id1[MAX_ID], id2[MAX_ID];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;

    /* Primeiro operador: Alice serve cliente 1 */
    strncpy(sm.caixas[0].operador, "Alice", MAX_OPERADOR - 1);
    sm.caixas[0].operador[MAX_OPERADOR - 1] = '\0';
    prods[0] = make_prod(1.0f, 1.0f);
    snprintf(id1, sizeof(id1), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id1, "PrimeiroCliente", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    /* Segundo operador: Bob serve cliente 2 */
    strncpy(sm.caixas[0].operador, "Bob", MAX_OPERADOR - 1);
    sm.caixas[0].operador[MAX_OPERADOR - 1] = '\0';
    prods[0] = make_prod(1.0f, 1.0f);
    snprintf(id2, sizeof(id2), "C%05d", ++cli_seq);
    inserir_novo_cliente(&sm, &hash, id2, "SegundoCliente", prods, 1);
    avancar_simulacao(&sm, &hash, 2);

    CHECK(sm.caixas[0].nHistorico == 2,
          "csv dois operadores: dois clientes no historico");

    if (sm.caixas[0].nHistorico == 2) {
        CHECK(strcmp(sm.caixas[0].historicoAtendidos[0]->operadorAtendimento, "Alice") == 0,
              "csv dois operadores: primeiro cliente tem operadorAtendimento=Alice");
        CHECK(strcmp(sm.caixas[0].historicoAtendidos[1]->operadorAtendimento, "Bob") == 0,
              "csv dois operadores: segundo cliente tem operadorAtendimento=Bob");
    }

    guardar_historico_caixas_csv(TMP_HIST15, &sm);

    CHECK(file_contains(TMP_HIST15, "Alice"),
          "csv dois operadores: Alice aparece no CSV");
    CHECK(file_contains(TMP_HIST15, "Bob"),
          "csv dois operadores: Bob aparece no CSV");
    /* cada operador deve aparecer exatamente 1 vez nos dados (nao no cabecalho) */
    CHECK(count_occurrences(TMP_HIST15, "Alice") == 1,
          "csv dois operadores: Alice aparece exatamente 1 vez");
    CHECK(count_occurrences(TMP_HIST15, "Bob") == 1,
          "csv dois operadores: Bob aparece exatamente 1 vez");

    remove(TMP_HIST15);
    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ── runner ─────────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== Issue #15: operadorAtendimento no Cliente ===\n\n");

    printf("-- Cycle 1: operadorAtendimento preenchido --\n");
    test_operador_campo_inicializado_vazio();
    test_operador_preenchido_apos_atendimento();

    printf("\n-- Cycle 2: CSV usa operadorAtendimento por cliente --\n");
    test_csv_usa_operador_por_cliente();

    printf("\n=== Resultados: %d PASS, %d FAIL ===\n", pass_count, fail_count);
    return fail_count > 0 ? 1 : 0;
}
