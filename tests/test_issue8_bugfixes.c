#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"
#include "hash.h"
#include "customer.h"
#include "product.h"
#include "config.h"
#include "register.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define TMP_SNAP "output/test_snap_issue8.txt"

static Supermercado make_sm(void) {
    Supermercado sm;
    Configuracao cfg;
    config_default(&cfg);
    cfg.nCaixas = 2;
    cfg.minFila = 1;
    cfg.maxFila = 10;
    cfg.maxEspera = 5;
    supermercado_init(&sm, &cfg, NULL, NULL, NULL);
    sm.caixas[0].estado = CAIXA_ABERTA;
    sm.caixas[1].estado = CAIXA_ABERTA;
    return sm;
}

static Produto make_produto(int id, const char *nome, float preco, int tempoPassagem) {
    Produto p;
    p.id = id;
    strncpy(p.nome, nome, MAX_NOME - 1);
    p.nome[MAX_NOME - 1] = '\0';
    p.preco = preco;
    p.tempoCompra = 3;
    p.tempoPassagem = tempoPassagem;
    p.oferecido = false;
    return p;
}

/* ------------------------------------------------------------------ */
/* Ciclo 1: Bug 1 — MAX_ESPERA verificado para emAtendimento           */
/* ------------------------------------------------------------------ */
static void test_bug1_oferta_em_atendimento(void) {
    Supermercado sm = make_sm();
    HashClientes hash;
    Produto p;
    Produto *copia;
    Cliente *c;

    hash_init(&hash);

    p = make_produto(1, "Leite", 0.80f, 2);
    copia = (Produto *)malloc(sizeof(Produto));
    *copia = p;

    c = criar_cliente("111111", "Ana Lima", 1, 0, 0, copia);
    c->instanteEntradaFila = 0;
    c->comprasTerminadas = true;
    sm.caixas[0].emAtendimento = c;
    sm.caixas[0].tempoRestanteAtendimento = 100;

    sm.instanteAtual = 6; /* espera = 6 > maxEspera=5 */
    avancar_simulacao(&sm, &hash, 1);

    CHECK(c->oferecimentoFeito == true,
          "bug1: cliente em atendimento com espera > maxEspera recebe oferta");
    CHECK(sm.totalProdutosOferecidos >= 1,
          "bug1: totalProdutosOferecidos incrementado");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------ */
/* Ciclo 2: Bug 2 — CAIXA_A_FECHAR fecha imediatamente se ja vazia     */
/* ------------------------------------------------------------------ */
static void test_bug2_fechar_imediato_se_vazio(void) {
    Supermercado sm = make_sm();
    HashClientes hash;
    hash_init(&hash);

    /* 2 caixas abertas, filas vazias, nenhum em atendimento → media=0 < minFila=1 */
    sm.cfg.minFila = 1;
    sm.instanteAtual = 10;
    sm.instanteUltimaAberturaManual = 0;

    verificar_politica_caixas(&sm, &hash);

    {
        int i, found_a_fechar = 0, found_fechada = 0;
        for (i = 0; i < sm.cfg.nCaixas; ++i) {
            if (sm.caixas[i].estado == CAIXA_A_FECHAR) found_a_fechar = 1;
            if (sm.caixas[i].estado == CAIXA_FECHADA)  found_fechada  = 1;
        }
        CHECK(found_a_fechar == 0,
              "bug2: nenhuma caixa fica em CAIXA_A_FECHAR quando ja estava vazia");
        CHECK(found_fechada == 1,
              "bug2: caixa passa diretamente para CAIXA_FECHADA quando ja estava vazia");
    }

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------ */
/* Ciclo 3: Bug 3 — operador restaurado do snapshot                    */
/* ------------------------------------------------------------------ */
static void test_bug3_operador_roundtrip(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h2;
    hash_init(&h2);

    strncpy(sm.caixas[0].operador, "TestOp", MAX_OPERADOR - 1);
    sm.caixas[0].operador[MAX_OPERADOR - 1] = '\0';

    guardar_snapshot(TMP_SNAP, &sm, NULL);

    sm2.caixas[0].operador[0] = '\0';
    carregar_dados_iniciais(TMP_SNAP, &sm2, &h2);

    CHECK(strcmp(sm2.caixas[0].operador, "TestOp") == 0,
          "bug3: operador restaurado do snapshot");

    supermercado_destruir(&sm, NULL);
    supermercado_destruir(&sm2, &h2);
}

/* ------------------------------------------------------------------ */
/* Ciclo 4: Bug 4 — cliente em atendimento inserido no hash ao carregar */
/* ------------------------------------------------------------------ */
static void test_bug4_hash_em_atendimento(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h, h2;
    Produto p;
    Produto *copia;
    Cliente *c;

    hash_init(&h);
    hash_init(&h2);

    p = make_produto(2, "Queijo", 2.50f, 3);
    copia = (Produto *)malloc(sizeof(Produto));
    *copia = p;
    c = criar_cliente("222222", "Bruno Costa", 1, 0, 0, copia);
    c->instanteInicioAtendimento = 5;
    c->comprasTerminadas = true;
    sm.caixas[0].emAtendimento = c;
    sm.caixas[0].tempoRestanteAtendimento = 4;

    guardar_snapshot(TMP_SNAP, &sm, NULL);
    carregar_dados_iniciais(TMP_SNAP, &sm2, &h2);

    CHECK(hash_pesquisar(&h2, "222222") != NULL,
          "bug4: cliente em atendimento encontrado no hash apos carregar snapshot");

    supermercado_destruir(&sm, &h);
    supermercado_destruir(&sm2, &h2);
}

/* ------------------------------------------------------------------ */
/* Ciclo 5A: Bug 5A — memoria_utilizada nao duplica sizeof(HashClientes) */
/* ------------------------------------------------------------------ */
static void test_bug5a_memoria_sem_duplicacao(void) {
    Supermercado sm = make_sm();
    HashClientes hash;
    size_t m, expected;
    hash_init(&hash);

    m = memoria_utilizada(&sm, &hash);
    /* sem clientes em fila, em atendimento ou em loja */
    expected = sizeof(Supermercado)
             + (size_t)sm.cfg.nCaixas * sizeof(Caixa)
             + hash_memoria(&hash);
    CHECK(m == expected,
          "bug5a: memoria_utilizada nao duplica sizeof(HashClientes)");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------ */
/* Ciclo 5B: Bug 5B — memoria_utilizada conta clientesEmLoja           */
/* ------------------------------------------------------------------ */
static void test_bug5b_memoria_clientes_em_loja(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h, h2;
    Produto prods[2];
    size_t m_sem, m_com;
    hash_init(&h); hash_init(&h2);

    m_sem = memoria_utilizada(&sm, &h);

    prods[0] = make_produto(10, "A", 1.0f, 1);
    prods[1] = make_produto(11, "B", 1.5f, 2);
    inserir_novo_cliente(&sm2, &h2, "333333", "Clara Dias", prods, 2);

    m_com = memoria_utilizada(&sm2, &h2);

    CHECK(m_com > m_sem,
          "bug5b: memoria_utilizada maior com 1 cliente em loja");
    CHECK(m_com - m_sem >= sizeof(Cliente) + 2 * sizeof(Produto),
          "bug5b: diferenca >= sizeof(Cliente)+2*sizeof(Produto)");

    supermercado_destruir(&sm, &h);
    supermercado_destruir(&sm2, &h2);
}

/* ------------------------------------------------------------------ */
/* Ciclo 6: Display — mostrar_caixa nao crasha com emAtendimento       */
/* ------------------------------------------------------------------ */
static void test_ciclo6_mostrar_caixa_com_atendimento(void) {
    Supermercado sm = make_sm();
    Produto p;
    Produto *copia;
    Cliente *c;

    p = make_produto(3, "Iogurte", 0.60f, 1);
    copia = (Produto *)malloc(sizeof(Produto));
    *copia = p;
    c = criar_cliente("444444", "Dora Reis", 1, 0, 0, copia);
    sm.caixas[0].emAtendimento = c;
    sm.caixas[0].tempoRestanteAtendimento = 3;

    /* smoke: must not crash and must print nome do cliente */
    mostrar_caixa(&sm.caixas[0]);

    CHECK(1, "display: mostrar_caixa com emAtendimento nao crasha");

    supermercado_destruir(&sm, NULL);
}

int main(void) {
    printf("=== Issue 8: Bugfixes ===\n\n");
    test_bug1_oferta_em_atendimento();
    test_bug2_fechar_imediato_se_vazio();
    test_bug3_operador_roundtrip();
    test_bug4_hash_em_atendimento();
    test_bug5a_memoria_sem_duplicacao();
    test_bug5b_memoria_clientes_em_loja();
    test_ciclo6_mostrar_caixa_com_atendimento();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
