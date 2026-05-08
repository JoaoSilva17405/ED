#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"
#include "hash.h"
#include "config.h"
#include "register.h"
#include "customer.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

static Supermercado make_sm(int nCaixas, int maxEspera, int minFila, int maxFila) {
    Supermercado sm;
    Configuracao cfg;
    config_default(&cfg);
    cfg.nCaixas   = nCaixas;
    cfg.maxEspera = maxEspera;
    cfg.minFila   = minFila;
    cfg.maxFila   = maxFila;
    supermercado_init(&sm, &cfg, NULL, NULL, NULL);
    return sm;
}

/* Insere cliente e força tempoCompraTotal = 1 para ser previsível */
static void inserir_com_compra_imediata(Supermercado *sm, HashClientes *hash,
                                        const char *id) {
    Produto p;
    memset(&p, 0, sizeof(p));
    strncpy(p.nome, "Teste", sizeof(p.nome) - 1);
    p.preco = 1.0f;
    p.tempoCompra = 5;
    p.tempoPassagem = 2;
    inserir_novo_cliente(sm, hash, id, "Teste", &p, 1);
    /* força fim de compras no próximo tick */
    if (sm->nClientesEmLoja > 0)
        sm->clientesEmLoja[sm->nClientesEmLoja - 1]->tempoCompraTotal = 1;
}

/* Ciclo 1: tick sem eventos -> retorna EVT_NENHUM */
static void test_tick_vazio_retorna_zero(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    int eventos = avancar_simulacao(&sm, &hash, 1);
    CHECK(eventos == EVT_NENHUM, "tick sem clientes retorna EVT_NENHUM (0)");

    supermercado_destruir(&sm, &hash);
}

/* Ciclo 2: cliente termina compras -> EVT_CLIENTE_ENTROU_FILA */
static void test_evt_cliente_entrou_fila(void) {
    Supermercado sm = make_sm(1, 120, 0, 7);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    inserir_com_compra_imediata(&sm, &hash, "000001");

    int eventos = avancar_simulacao(&sm, &hash, 1);
    CHECK(eventos & EVT_CLIENTE_ENTROU_FILA,
          "EVT_CLIENTE_ENTROU_FILA definido quando cliente passa para a fila");

    supermercado_destruir(&sm, &hash);
}

/* Ciclo 3: cliente inicia atendimento no mesmo tick que entra na fila */
static void test_evt_inicio_atendimento(void) {
    Supermercado sm = make_sm(1, 120, 0, 7);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    inserir_com_compra_imediata(&sm, &hash, "000002");

    int eventos = avancar_simulacao(&sm, &hash, 1);
    CHECK(eventos & EVT_CLIENTE_INICIO_ATENDIMENTO,
          "EVT_CLIENTE_INICIO_ATENDIMENTO definido quando caixa inicia atendimento");

    supermercado_destruir(&sm, &hash);
}

/* Ciclo 4: cliente conclui atendimento -> EVT_CLIENTE_FIM_ATENDIMENTO */
static void test_evt_fim_atendimento(void) {
    Supermercado sm = make_sm(1, 120, 0, 7);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    inserir_com_compra_imediata(&sm, &hash, "000003");

    /* avança até cliente estar em atendimento */
    avancar_simulacao(&sm, &hash, 1);
    CHECK(sm.caixas[0].emAtendimento != NULL, "setup: cliente em atendimento antes do teste");

    /* força conclusão no próximo tick */
    sm.caixas[0].tempoRestanteAtendimento = 1;
    int eventos = avancar_simulacao(&sm, &hash, 1);
    CHECK(eventos & EVT_CLIENTE_FIM_ATENDIMENTO,
          "EVT_CLIENTE_FIM_ATENDIMENTO definido quando atendimento conclui");

    supermercado_destruir(&sm, &hash);
}

/* Ciclo 5: política abre caixa -> EVT_CAIXA_ABRIU */
static void test_evt_caixa_abriu(void) {
    /* maxFila=0: 1 caixa aberta, 1 fechada; 2 clientes em loja terminam compras
       no tick 1 -> entram na fila -> media=1 > 0 -> politica abre a caixa fechada */
    Supermercado sm = make_sm(2, 120, 0, 0);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    inserir_com_compra_imediata(&sm, &hash, "000004");
    inserir_com_compra_imediata(&sm, &hash, "000005");

    int eventos = avancar_simulacao(&sm, &hash, 1);
    CHECK(eventos & EVT_CAIXA_ABRIU,
          "EVT_CAIXA_ABRIU definido quando politica abre nova caixa");

    supermercado_destruir(&sm, &hash);
}

/* Ciclo 6: política fecha caixa -> EVT_CAIXA_FECHOU */
static void test_evt_caixa_fechou(void) {
    /* minFila=3, 2 caixas abertas, filas vazias -> media=0 < 3 -> fecha */
    Supermercado sm = make_sm(2, 120, 3, 7);
    HashClientes hash;
    hash_init(&hash);

    /* abre as 2 caixas sem passar por abrir_caixa_manual (evita cooldown) */
    sm.caixas[0].estado = CAIXA_ABERTA;
    sm.caixas[1].estado = CAIXA_ABERTA;

    int eventos = avancar_simulacao(&sm, &hash, 1);
    CHECK(eventos & EVT_CAIXA_FECHOU,
          "EVT_CAIXA_FECHOU definido quando politica fecha caixa");

    supermercado_destruir(&sm, &hash);
}

int main(void) {
    printf("=== Issue 12: Eventos em avancar_simulacao ===\n\n");
    test_tick_vazio_retorna_zero();
    test_evt_cliente_entrou_fila();
    test_evt_inicio_atendimento();
    test_evt_fim_atendimento();
    test_evt_caixa_abriu();
    test_evt_caixa_fechou();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
