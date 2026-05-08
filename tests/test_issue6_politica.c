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

/* ── helpers ─────────────────────────────────────────────────────────── */

static Produto *make_produto(void) {
    Produto *p = (Produto *)malloc(sizeof(Produto));
    if (!p) return NULL;
    p->id = 1;
    strncpy(p->nome, "Teste", sizeof(p->nome) - 1);
    p->nome[sizeof(p->nome) - 1] = '\0';
    p->preco = 1.0f;
    p->tempoCompra = 5;
    p->tempoPassagem = 1;
    p->oferecido = false;
    return p;
}

static int cliente_seq = 0;

static Cliente *make_cliente(void) {
    char id[MAX_ID];
    snprintf(id, sizeof(id), "T%05d", ++cliente_seq);
    return criar_cliente(id, "Teste", 1, 0, 0, make_produto());
}

static Supermercado make_sm(int nCaixas, int maxFila, int minFila) {
    Supermercado sm;
    Configuracao cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.maxEspera = 120;
    cfg.nCaixas = nCaixas;
    cfg.maxPreco = 40.0f;
    cfg.maxFila = maxFila;
    cfg.minFila = minFila;
    supermercado_init(&sm, &cfg, NULL, NULL, NULL);
    return sm;
}

static void enqueue_n(Supermercado *sm, int idx, int n) {
    int i;
    for (i = 0; i < n; i++) {
        Cliente *c = make_cliente();
        if (c) fila_inserir(&sm->caixas[idx].fila, c);
    }
}

/* ── Cycle 1: media > MAX_FILA abre caixa de menor ID fechada ─────────── */
static void test_abre_caixa_media_alta(void) {
    Supermercado sm = make_sm(3, 3, 1);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 4);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 4);
    /* caixa[2] FECHADA por defeito; avg=8/2=4 > MAX_FILA=3 */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[2].estado == CAIXA_ABERTA, "media>MAX_FILA: abre caixa[2] (menor ID fechada)");
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA, "media>MAX_FILA: caixa[0] permanece aberta");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 2: CAIXA_A_FECHAR excluida do denominador da media ─────────── */
static void test_a_fechar_excluida_media(void) {
    Supermercado sm = make_sm(3, 3, 1);
    sm.caixas[0].estado = CAIXA_ABERTA;   enqueue_n(&sm, 0, 4);
    sm.caixas[1].estado = CAIXA_A_FECHAR; enqueue_n(&sm, 1, 10);
    /* caixa[2] FECHADA; media de CAIXA_ABERTA = 4/1=4 > MAX_FILA=3 */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[2].estado == CAIXA_ABERTA, "A_FECHAR excluida: abre com media das so ABERTAS");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 3: media < MIN_FILA faz soft-close da caixa com menor fila ── */
static void test_fecha_suave_media_baixa(void) {
    Supermercado sm = make_sm(3, 7, 3);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 1);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 3);
    sm.caixas[2].estado = CAIXA_ABERTA; enqueue_n(&sm, 2, 2);
    /* avg=(1+3+2)/3=2.0 < MIN_FILA=3; caixa[0] tem menor fila=1 */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[0].estado == CAIXA_A_FECHAR, "media<MIN_FILA: caixa mais vazia vai A_FECHAR");
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA,   "media<MIN_FILA: caixa[1] permanece aberta");
    CHECK(sm.caixas[2].estado == CAIXA_ABERTA,   "media<MIN_FILA: caixa[2] permanece aberta");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 3b: fecho usa fila.tamanho, nao caixa_total_pessoas ────────── */
static void test_fecha_usa_fila_nao_total(void) {
    Supermercado sm = make_sm(2, 7, 2);
    Cliente *servido;
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 2);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 1);
    /* caixa[1] tem fila=1 mas tambem 1 em atendimento (total=2 igual a caixa[0]) */
    servido = make_cliente();
    servido->estavaEmAtendimento = true;
    sm.caixas[1].emAtendimento = servido;
    sm.caixas[1].tempoRestanteAtendimento = 99;
    /* avg=(2+1)/2=1.5 < MIN_FILA=2                           */
    /* codigo errado (total pessoas): caixa[0]=2, caixa[1]=2  */
    /*   -> empate -> fecha caixa[0] (idx=0)                  */
    /* codigo correto (fila.tamanho): caixa[0]=2, caixa[1]=1  */
    /*   -> fecha caixa[1] (menor fila)                       */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[1].estado == CAIXA_A_FECHAR, "fecha usa fila.tamanho: caixa[1] (fila=1) fecha");
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA,   "fecha usa fila.tamanho: caixa[0] (fila=2) permanece");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 4: media == MAX_FILA => zona estavel, sem abertura ─────────── */
static void test_zona_estavel_max(void) {
    Supermercado sm = make_sm(3, 3, 1);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 3);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 3);
    /* avg=3.0 == MAX_FILA=3 => sem abertura */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[2].estado == CAIXA_FECHADA, "media==MAX_FILA: zona estavel, caixa[2] nao abre");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 5: media == MIN_FILA => zona estavel, sem fecho ───────────── */
static void test_zona_estavel_min(void) {
    Supermercado sm = make_sm(3, 7, 2);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 2);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 2);
    sm.caixas[2].estado = CAIXA_ABERTA; enqueue_n(&sm, 2, 2);
    /* avg=2.0 == MIN_FILA=2 => sem fecho */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA, "media==MIN_FILA: zona estavel, caixa[0] nao fecha");
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA, "media==MIN_FILA: zona estavel, caixa[1] nao fecha");
    CHECK(sm.caixas[2].estado == CAIXA_ABERTA, "media==MIN_FILA: zona estavel, caixa[2] nao fecha");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 6: anti-oscilacao — fecho no instante T bloqueia abertura ──── */
static void test_anti_oscilacao_mesmo_instante(void) {
    Supermercado sm = make_sm(2, 5, 1);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 10);
    /* caixa[1] FECHADA; avg=10 > MAX_FILA=5 */
    sm.instanteAtual = 3;
    sm.instanteUltimoFecho = 3; /* fecho ocorreu neste mesmo instante */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[1].estado == CAIXA_FECHADA, "anti-oscilacao: abertura bloqueada no mesmo instante do fecho");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 7: instante T+1 desbloqueia abertura ──────────────────────── */
static void test_anti_oscilacao_instante_seguinte(void) {
    Supermercado sm = make_sm(2, 5, 1);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 10);
    sm.instanteAtual = 4;
    sm.instanteUltimoFecho = 3; /* fecho foi no instante anterior */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA, "anti-oscilacao: abertura permitida no instante seguinte");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 8: so 1 caixa aberta => nunca fecha ───────────────────────── */
static void test_nunca_fecha_ultima_caixa(void) {
    Supermercado sm = make_sm(3, 7, 1);
    sm.caixas[0].estado = CAIXA_ABERTA;
    /* caixas[1] e [2] FECHADAS; avg=0 < MIN_FILA=1, mas so 1 aberta */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA, "so 1 aberta: nunca fecha a ultima caixa");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 9: todas abertas, avg > MAX_FILA => ignora silenciosamente ── */
static void test_todas_abertas_ignora(void) {
    Supermercado sm = make_sm(3, 5, 1);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 10);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 10);
    sm.caixas[2].estado = CAIXA_ABERTA; enqueue_n(&sm, 2, 10);
    verificar_politica_caixas(&sm, NULL); /* nao deve crashar */
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA, "todas abertas: sem crash, caixa[0] aberta");
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA, "todas abertas: sem crash, caixa[1] aberta");
    CHECK(sm.caixas[2].estado == CAIXA_ABERTA, "todas abertas: sem crash, caixa[2] aberta");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 10: abrir_caixa_manual atualiza instanteUltimaAberturaManual  */
static void test_abertura_manual_atualiza_instante(void) {
    Supermercado sm = make_sm(2, 7, 1);
    sm.instanteAtual = 10;
    /* caixa[1] FECHADA */
    abrir_caixa_manual(&sm, 1);
    CHECK(sm.instanteUltimaAberturaManual == 10,
          "abrir_manual: atualiza instanteUltimaAberturaManual para instante atual");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 11: protecao manual bloqueia fecho dentro dos 3 instantes ──── */
static void test_protecao_manual_bloqueia_fecho(void) {
    Supermercado sm = make_sm(3, 7, 3);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 1);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 1);
    sm.caixas[2].estado = CAIXA_ABERTA; enqueue_n(&sm, 2, 1);
    /* avg=1 < MIN_FILA=3 => normalmente fecharia, mas dentro da protecao */
    sm.instanteUltimaAberturaManual = 10;
    sm.instanteAtual = 12; /* 12 <= 10+3=13: ainda protegido */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA, "protecao manual: fecho bloqueado (instante 12 <= 13)");
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA, "protecao manual: caixa[1] protegida");
    CHECK(sm.caixas[2].estado == CAIXA_ABERTA, "protecao manual: caixa[2] protegida");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 12: protecao expira apos 3 instantes (instante > manual+3) ── */
static void test_protecao_manual_expira(void) {
    Supermercado sm = make_sm(3, 7, 3);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 1);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 1);
    sm.caixas[2].estado = CAIXA_ABERTA; enqueue_n(&sm, 2, 1);
    /* avg=1 < MIN_FILA=3 => deve fechar apos protecao expirar */
    sm.instanteUltimaAberturaManual = 10;
    sm.instanteAtual = 14; /* 14 > 10+3=13: protecao expirada */
    verificar_politica_caixas(&sm, NULL);
    {
        int alguma_fechando = (sm.caixas[0].estado == CAIXA_A_FECHAR ||
                               sm.caixas[1].estado == CAIXA_A_FECHAR ||
                               sm.caixas[2].estado == CAIXA_A_FECHAR);
        CHECK(alguma_fechando, "protecao expirada: fecho automatico retomado (instante 14 > 13)");
    }
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 13: empate em fila.tamanho => fecha caixa de menor ID ──────── */
static void test_empate_fecha_menor_id(void) {
    Supermercado sm = make_sm(3, 7, 2);
    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 1);
    sm.caixas[1].estado = CAIXA_ABERTA; enqueue_n(&sm, 1, 1);
    sm.caixas[2].estado = CAIXA_ABERTA; enqueue_n(&sm, 2, 3);
    /* avg=(1+1+3)/3~=1.67 < MIN_FILA=2; empate entre caixa[0] e [1]; fecha caixa[0] */
    verificar_politica_caixas(&sm, NULL);
    CHECK(sm.caixas[0].estado == CAIXA_A_FECHAR, "empate: caixa de menor ID (0) e escolhida para fechar");
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA,   "empate: caixa[1] permanece aberta");
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 14: politica reage apos A_FECHAR transitar para FECHADA ────── */
static void test_politica_apos_fecho_definitivo(void) {
    Supermercado sm = make_sm(3, 3, 1);
    HashClientes hash;
    Cliente *c;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA; enqueue_n(&sm, 0, 5);
    /* caixa[1]: A_FECHAR com 1 cliente em atendimento, fila vazia */
    sm.caixas[1].estado = CAIXA_A_FECHAR;
    c = make_cliente();
    c->estavaEmAtendimento = true;
    c->instanteInicioAtendimento = 0;
    c->instanteEntradaFila = 0;
    sm.caixas[1].emAtendimento = c;
    sm.caixas[1].tempoRestanteAtendimento = 1;
    /* caixa[2] FECHADA */

    /* 1 passo: caixa[1] serve e fecha; politica ve avg>MAX_FILA e abre caixa[1] */
    avancar_simulacao(&sm, &hash, 1);
    CHECK(sm.caixas[1].estado == CAIXA_FECHADA || sm.caixas[1].estado == CAIXA_ABERTA,
          "A_FECHAR->FECHADA: caixa[1] concluiu fecho ou foi reaberta pela politica");
    /* A politica ve 1 ABERTA com 4 clientes (avg=4 > MAX_FILA=3) e abre 1a FECHADA (caixa[1]) */
    CHECK(sm.caixas[1].estado == CAIXA_ABERTA,
          "politica pos-fecho: caixa[1] reaberta (avg da unica ABERTA > MAX_FILA)");

    hash_destruir(&hash);
    supermercado_destruir(&sm, NULL);
}

/* ── Cycle 15: snapshot roundtrip preserva os dois novos campos ───────── */
#define TMP_SNAP6 "output/test_issue6_snap_tmp.txt"
static void test_snapshot_roundtrip_politica(void) {
    Supermercado sm  = make_sm(2, 7, 1);
    Supermercado sm2 = make_sm(2, 7, 1);
    HashClientes h2;
    hash_init(&h2);

    sm.instanteUltimoFecho          = 17;
    sm.instanteUltimaAberturaManual = 42;
    sm.caixas[0].estado = CAIXA_ABERTA;

    guardar_snapshot(TMP_SNAP6, &sm, NULL);
    carregar_dados_iniciais(TMP_SNAP6, &sm2, &h2);

    CHECK(sm2.instanteUltimoFecho == 17,
          "snapshot roundtrip: instanteUltimoFecho restaurado");
    CHECK(sm2.instanteUltimaAberturaManual == 42,
          "snapshot roundtrip: instanteUltimaAberturaManual restaurado");

    supermercado_destruir(&sm,  NULL);
    supermercado_destruir(&sm2, &h2);
}

/* ── runner ──────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== Issue #6: Politica Automatica de Caixas ===\n\n");

    test_abre_caixa_media_alta();
    test_a_fechar_excluida_media();
    test_fecha_suave_media_baixa();
    test_fecha_usa_fila_nao_total();
    test_zona_estavel_max();
    test_zona_estavel_min();
    test_anti_oscilacao_mesmo_instante();
    test_anti_oscilacao_instante_seguinte();
    test_nunca_fecha_ultima_caixa();
    test_todas_abertas_ignora();
    test_abertura_manual_atualiza_instante();
    test_protecao_manual_bloqueia_fecho();
    test_protecao_manual_expira();
    test_empate_fecha_menor_id();
    test_politica_apos_fecho_definitivo();
    test_snapshot_roundtrip_politica();

    printf("\n=== Resultados: %d PASS, %d FAIL ===\n", pass_count, fail_count);
    return fail_count > 0 ? 1 : 0;
}
