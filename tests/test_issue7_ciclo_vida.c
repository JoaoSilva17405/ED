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

/* ── helpers ────────────────────────────────────────────────────────────── */

static Produto make_prod(int tempoCompra, int tempoPassagem) {
    Produto p;
    memset(&p, 0, sizeof(p));
    p.id = 1;
    strncpy(p.nome, "Teste", sizeof(p.nome) - 1);
    p.preco = 1.0f;
    p.tempoCompra   = tempoCompra;
    p.tempoPassagem = tempoPassagem;
    p.oferecido     = false;
    return p;
}

static Produto *make_produto_heap(int tempoCompra, int tempoPassagem) {
    Produto *p = (Produto *)malloc(sizeof(Produto));
    if (!p) return NULL;
    *p = make_prod(tempoCompra, tempoPassagem);
    return p;
}

static int cliente_seq = 0;

static Cliente *make_cliente_n(int nProd, int tempoCompra, int tempoPassagem) {
    Produto *prods;
    char id[MAX_ID];
    int i;
    prods = (Produto *)malloc(sizeof(Produto) * nProd);
    if (!prods) return NULL;
    for (i = 0; i < nProd; i++)
        prods[i] = make_prod(tempoCompra, tempoPassagem);
    snprintf(id, sizeof(id), "C%05d", ++cliente_seq);
    return criar_cliente(id, "Teste", nProd, 0, 0, prods);
}

static Supermercado make_sm(int nCaixas) {
    Supermercado sm;
    Configuracao cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.maxEspera              = 120;
    cfg.nCaixas                = nCaixas;
    cfg.maxPreco               = 40.0f;
    cfg.maxFila                = 7;
    cfg.minFila                = 3;
    cfg.minProdutos            = 1;
    cfg.maxProdutos            = 20;
    supermercado_init(&sm, &cfg, NULL, NULL, NULL);
    return sm;
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 1 — tempo_compra_total_produtos
   ══════════════════════════════════════════════════════════════════════════ */

static void test_tempo_compra_total_vazio(void) {
    CHECK(tempo_compra_total_produtos(NULL, 0) == 0,
          "tempo_compra_total: array vazio retorna 0");
}

static void test_tempo_compra_total_unitario(void) {
    Produto p = make_prod(7, 3);
    CHECK(tempo_compra_total_produtos(&p, 1) == 7,
          "tempo_compra_total: produto unico retorna o seu tempoCompra");
}

static void test_tempo_compra_total_normal(void) {
    Produto prods[3];
    prods[0] = make_prod(5, 2);
    prods[1] = make_prod(8, 3);
    prods[2] = make_prod(3, 1);
    CHECK(tempo_compra_total_produtos(prods, 3) == 16,
          "tempo_compra_total: soma correta de 3 produtos (5+8+3=16)");
}

static void test_tempo_passagem_nao_afeta_compra(void) {
    Produto prods[2];
    prods[0] = make_prod(4, 99);
    prods[1] = make_prod(6, 99);
    CHECK(tempo_compra_total_produtos(prods, 2) == 10,
          "tempo_compra_total: tempoPassagem nao e incluido na soma");
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 2 — Configuracao: minProdutos / maxProdutos
   ══════════════════════════════════════════════════════════════════════════ */

static void test_config_default_minmax_produtos(void) {
    Configuracao cfg;
    config_default(&cfg);
    CHECK(cfg.minProdutos >= 1,  "config_default: minProdutos >= 1");
    CHECK(cfg.maxProdutos >= cfg.minProdutos,
          "config_default: maxProdutos >= minProdutos");
}

static void test_config_parse_minmax_produtos(void) {
    const char *fname = "output/test_cfg_tmp.txt";
    Configuracao cfg;
    FILE *f = fopen(fname, "w");
    if (!f) { CHECK(0, "config parse: nao foi possivel criar ficheiro temporario"); return; }
    fprintf(f, "MIN_PRODUTOS 3\nMAX_PRODUTOS 15\n");
    fclose(f);
    config_default(&cfg);
    carregar_configuracao(fname, &cfg);
    CHECK(cfg.minProdutos == 3,  "config parse: MIN_PRODUTOS lido corretamente");
    CHECK(cfg.maxProdutos == 15, "config parse: MAX_PRODUTOS lido corretamente");
    remove(fname);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 3 — inserir_novo_cliente: cliente fica em loja, nao na fila
   ══════════════════════════════════════════════════════════════════════════ */

static void test_inserir_coloca_em_loja(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[2];
    int resultado, i;

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    sm.caixas[1].estado = CAIXA_ABERTA;

    prods[0] = make_prod(10, 3);
    prods[1] = make_prod(5,  2);

    resultado = inserir_novo_cliente(&sm, &hash, "CLI001", "Ana", prods, 2);

    CHECK(resultado == INSERIR_CLIENTE_EM_COMPRAS,
          "inserir: retorna INSERIR_CLIENTE_EM_COMPRAS");
    CHECK(sm.nClientesEmLoja == 1,
          "inserir: nClientesEmLoja == 1 apos insercao");
    for (i = 0; i < sm.cfg.nCaixas; i++)
        CHECK(sm.caixas[i].fila.tamanho == 0,
              "inserir: fila de todas as caixas continua vazia");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_inserir_cliente_detetado_via_hash(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[1];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    prods[0] = make_prod(5, 2);

    inserir_novo_cliente(&sm, &hash, "CLI002", "Bruno", prods, 1);
    CHECK(hash_pesquisar(&hash, "CLI002") != NULL,
          "inserir: cliente em loja encontrado via hash");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_inserir_duplicado_rejeitado(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[1];
    int resultado;

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    prods[0] = make_prod(5, 2);

    inserir_novo_cliente(&sm, &hash, "CLI003", "Carlos", prods, 1);
    resultado = inserir_novo_cliente(&sm, &hash, "CLI003", "Carlos", prods, 1);

    CHECK(resultado == INSERIR_CLIENTE_DUPLICADO,
          "inserir: segundo inserir do mesmo ID retorna INSERIR_CLIENTE_DUPLICADO");
    CHECK(sm.nClientesEmLoja == 1,
          "inserir: duplicado nao aumenta nClientesEmLoja");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_inserir_tempoCompraTotal_calculado(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[3];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    prods[0] = make_prod(4, 1);
    prods[1] = make_prod(6, 2);
    prods[2] = make_prod(2, 1);

    inserir_novo_cliente(&sm, &hash, "CLI004", "Diana", prods, 3);

    CHECK(sm.nClientesEmLoja == 1, "inserir: cliente presente em loja");
    CHECK(sm.clientesEmLoja[0]->tempoCompraTotal == 12,
          "inserir: tempoCompraTotal = soma dos tempoCompra (4+6+2=12)");
    CHECK(sm.clientesEmLoja[0]->instanteEntradaLoja == 0,
          "inserir: instanteEntradaLoja == instanteAtual da simulacao");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 4 — avancar_simulacao: transicao loja → fila
   ══════════════════════════════════════════════════════════════════════════ */

static void test_avancar_antes_do_tempo_nao_move(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[1];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    prods[0] = make_prod(10, 2);

    inserir_novo_cliente(&sm, &hash, "CLI010", "Eva", prods, 1);
    /* tempoCompraTotal = 10; avancar apenas 5 ticks */
    avancar_simulacao(&sm, &hash, 5);

    CHECK(sm.nClientesEmLoja == 1,
          "avancar: cliente ainda em loja antes de completar tempoCompraTotal");
    CHECK(sm.caixas[0].fila.tamanho == 0,
          "avancar: fila ainda vazia antes de completar tempoCompraTotal");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_avancar_apos_tempo_move_para_fila(void) {
    Supermercado sm = make_sm(2);
    HashClientes hash;
    Produto prods[1];
    int fila_total;

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    sm.caixas[1].estado = CAIXA_ABERTA;
    prods[0] = make_prod(10, 2);

    inserir_novo_cliente(&sm, &hash, "CLI011", "Filipe", prods, 1);
    /* avancar exactamente 10 ticks (== tempoCompraTotal) */
    avancar_simulacao(&sm, &hash, 10);

    fila_total = sm.caixas[0].fila.tamanho + sm.caixas[1].fila.tamanho
               + (sm.caixas[0].emAtendimento ? 1 : 0)
               + (sm.caixas[1].emAtendimento ? 1 : 0);

    CHECK(sm.nClientesEmLoja == 0,
          "avancar: cliente saiu de loja apos tempoCompraTotal ticks");
    CHECK(fila_total == 1,
          "avancar: cliente aparece numa caixa apos tempoCompraTotal ticks");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_avancar_instante_entrada_fila_correto(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[1];
    NoHash *no;

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    prods[0] = make_prod(7, 2);

    inserir_novo_cliente(&sm, &hash, "CLI012", "Gina", prods, 1);
    avancar_simulacao(&sm, &hash, 7);

    no = hash_pesquisar(&hash, "CLI012");
    CHECK(no != NULL, "avancar: cliente ainda no hash apos entrar na fila");
    if (no && no->cliente)
        CHECK(no->cliente->instanteEntradaFila == 7,
              "avancar: instanteEntradaFila definido no tick certo (7)");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

static void test_avancar_cliente_atendido_e_removido(void) {
    Supermercado sm = make_sm(1);
    HashClientes hash;
    Produto prods[1];

    hash_init(&hash);
    sm.caixas[0].estado = CAIXA_ABERTA;
    prods[0] = make_prod(3, 2); /* compra=3, caixa=2 */

    inserir_novo_cliente(&sm, &hash, "CLI013", "Hugo", prods, 1);
    /* 3 ticks compras + 2 ticks caixa = 5 ticks total */
    avancar_simulacao(&sm, &hash, 6);

    CHECK(sm.nClientesEmLoja == 0,
          "ciclo completo: cliente nao esta em loja");
    CHECK(sm.caixas[0].fila.tamanho == 0,
          "ciclo completo: fila vazia apos atendimento");
    CHECK(sm.caixas[0].emAtendimento == NULL,
          "ciclo completo: nenhum cliente em atendimento");
    CHECK(sm.totalClientesAtendidos == 1,
          "ciclo completo: totalClientesAtendidos incrementado");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 5 — Snapshot roundtrip com clientes em loja
   ══════════════════════════════════════════════════════════════════════════ */

#define TMP_SNAP7 "output/test_snap7_tmp.txt"

static void test_snapshot_preserva_clientes_em_loja(void) {
    Supermercado sm  = make_sm(2);
    Supermercado sm2 = make_sm(2);
    HashClientes h2;
    Produto prods[2];

    hash_init(&h2);
    sm.caixas[0].estado = CAIXA_ABERTA;
    sm.caixas[1].estado = CAIXA_ABERTA;
    sm.instanteAtual = 5;

    prods[0] = make_prod(8, 3);
    prods[1] = make_prod(4, 2);

    {
        HashClientes h1;
        hash_init(&h1);
        inserir_novo_cliente(&sm, &h1, "CLI020", "Ines", prods, 2);
        guardar_snapshot(TMP_SNAP7, &sm, NULL);
        hash_destruir(&h1);
    }

    sm2.caixas[0].estado = CAIXA_ABERTA;
    sm2.caixas[1].estado = CAIXA_ABERTA;
    carregar_dados_iniciais(TMP_SNAP7, &sm2, &h2);

    CHECK(sm2.nClientesEmLoja == 1,
          "snapshot: cliente em loja restaurado apos carregar");
    if (sm2.nClientesEmLoja == 1) {
        CHECK(sm2.clientesEmLoja[0]->tempoCompraTotal == 12,
              "snapshot: tempoCompraTotal restaurado (8+4=12)");
        CHECK(sm2.clientesEmLoja[0]->instanteEntradaLoja == 5,
              "snapshot: instanteEntradaLoja restaurado");
        CHECK(strcmp(sm2.clientesEmLoja[0]->id, "CLI020") == 0,
              "snapshot: ID do cliente em loja restaurado");
    }

    supermercado_destruir(&sm,  NULL);
    supermercado_destruir(&sm2, &h2);
}

/* ── runner ─────────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== Issue #7: Ciclo de Vida do Cliente ===\n\n");

    printf("-- Cycle 1: tempo_compra_total_produtos --\n");
    test_tempo_compra_total_vazio();
    test_tempo_compra_total_unitario();
    test_tempo_compra_total_normal();
    test_tempo_passagem_nao_afeta_compra();

    printf("\n-- Cycle 2: Configuracao minProdutos/maxProdutos --\n");
    test_config_default_minmax_produtos();
    test_config_parse_minmax_produtos();

    printf("\n-- Cycle 3: inserir_novo_cliente em loja --\n");
    test_inserir_coloca_em_loja();
    test_inserir_cliente_detetado_via_hash();
    test_inserir_duplicado_rejeitado();
    test_inserir_tempoCompraTotal_calculado();

    printf("\n-- Cycle 4: avancar_simulacao transicao --\n");
    test_avancar_antes_do_tempo_nao_move();
    test_avancar_apos_tempo_move_para_fila();
    test_avancar_instante_entrada_fila_correto();
    test_avancar_cliente_atendido_e_removido();

    printf("\n-- Cycle 5: Snapshot roundtrip --\n");
    test_snapshot_preserva_clientes_em_loja();

    printf("\n=== Resultados: %d PASS, %d FAIL ===\n", pass_count, fail_count);
    return fail_count > 0 ? 1 : 0;
}
