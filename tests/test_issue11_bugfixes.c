#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"
#include "hash.h"
#include "customer.h"
#include "product.h"
#include "config.h"
#include "register.h"
#include "catalog.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define TMP_SNAP "output/test_snap_issue11.txt"

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

/* ------------------------------------------------------------------
   Ciclo 1 — Bug #1: ultimoOperador persiste apos fecho de caixa
   ------------------------------------------------------------------ */
static void test_bug1_ultimo_operador_persiste(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    hash_init(&hash);

    /* caixa comeca FECHADA; abrir atribui operador */
    abrir_caixa_manual(&sm, 0);

    CHECK(sm.caixas[0].ultimoOperador[0] != '\0',
          "bug1: ultimoOperador definido ao abrir caixa");

    /* simular fecho: limpar operador e estado */
    sm.caixas[0].operador[0] = '\0';
    sm.caixas[0].estado = CAIXA_FECHADA;

    CHECK(sm.caixas[0].ultimoOperador[0] != '\0',
          "bug1: ultimoOperador nao e limpo ao fechar caixa");
    CHECK(sm.caixas[0].operador[0] == '\0',
          "bug1: operador e limpo ao fechar caixa");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 2 — Bug #7: produtos do catalogo respeitam MAX_PRECO
   ------------------------------------------------------------------ */
static void test_bug7_catalogo_preco_clamped(void) {
    CatalogoProdutos cat;
    Configuracao cfg;
    Produto *result;
    int i;

    config_default(&cfg); /* maxPreco = 40.0f */

    /* catalogo manual com produto acima de maxPreco */
    Produto item;
    memset(&item, 0, sizeof(item));
    item.id = 1;
    strncpy(item.nome, "Caro", MAX_NOME - 1);
    item.preco        = 99.99f;  /* acima de MAX_PRECO=40 */
    item.tempoCompra  = 6;
    item.tempoPassagem = 6;
    item.oferecido    = false;

    cat.lista      = &item;
    cat.tamanho    = 1;
    cat.capacidade = 1;

    result = catalog_obter_produtos_aleatorios(&cat, 10, &cfg);
    if (!result) {
        printf("FAIL: bug7: catalog_obter_produtos_aleatorios retornou NULL (linha %d)\n", __LINE__);
        fail_count++;
        return;
    }

    for (i = 0; i < 10; i++) {
        CHECK(result[i].preco <= cfg.maxPreco,
              "bug7: preco do produto nao excede maxPreco");
        CHECK(result[i].preco > 0.0f,
              "bug7: preco do produto e positivo");
    }
    free(result);
}

/* ------------------------------------------------------------------
   Ciclo 3 — Bug #2a: fechar imediata recusa quando unica outra caixa e A_FECHAR
   ------------------------------------------------------------------ */
static void test_bug2a_fechar_imediata_recusa_se_so_a_fechar(void) {
    Supermercado sm = make_sm(2, 120, 1, 10);
    HashClientes hash;
    int r;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op0", MAX_OPERADOR - 1);
    sm.caixas[1].estado = CAIXA_A_FECHAR;
    strncpy(sm.caixas[1].operador, "Op1", MAX_OPERADOR - 1);

    /* tentar fechar caixa 0: unica outra (caixa 1) esta A_FECHAR, nao serve de destino */
    r = fechar_caixa_imediata_manual(&sm, &hash, 0);

    CHECK(r == 0,
          "bug2a: fechar_caixa_imediata_manual retorna 0 quando unica outra caixa e A_FECHAR");
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA,
          "bug2a: caixa nao e fechada quando nao ha destino valido");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 4 — Bug #2b+#3: campos do emAtendimento intactos quando fecho falha
   Cenario classico do bug original:
     - caixa 0: ABERTA (alvo), tem emAtendimento com campos conhecidos
     - caixa 1: A_FECHAR (unica outra caixa "open" pelo antigo contar_caixas_abertas)
   Codigo antigo: guard passava (count=2), campos corrompidos, restore errado
   Codigo novo:   guard rejeita (0 outras ABERTA), campos intactos
   ------------------------------------------------------------------ */
static Produto make_produto_simple(void) {
    Produto p;
    memset(&p, 0, sizeof(p));
    p.id = 1;
    strncpy(p.nome, "Test", MAX_NOME - 1);
    p.preco = 1.0f;
    p.tempoCompra = 10;
    p.tempoPassagem = 5;
    p.oferecido = false;
    return p;
}

static void test_bug2b3_campos_intactos_quando_fecho_falha(void) {
    Supermercado sm = make_sm(2, 120, 1, 10);
    HashClientes hash;
    Produto p0, *copia0;
    Cliente *c0;
    int tempo_restante_original = 77;
    int inicio_original = 5;
    bool estava_original = true;
    int r;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op0", MAX_OPERADOR - 1);
    /* caixa 1 A_FECHAR: nao serve de destino para emAtendimento nem fila */
    sm.caixas[1].estado = CAIXA_A_FECHAR;
    strncpy(sm.caixas[1].operador, "Op1", MAX_OPERADOR - 1);

    p0 = make_produto_simple();
    copia0 = (Produto *)malloc(sizeof(Produto));
    *copia0 = p0;
    c0 = criar_cliente("100001", "A", 1, 0, 0, copia0);
    c0->comprasTerminadas = true;
    c0->instanteEntradaFila = 0;
    c0->instanteInicioAtendimento = inicio_original;
    c0->estavaEmAtendimento = estava_original;
    sm.caixas[0].emAtendimento = c0;
    sm.caixas[0].tempoRestanteAtendimento = tempo_restante_original;
    hash_inserir(&hash, c0, 0);

    /* tentar fechar caixa 0: nao existe outra ABERTA para redistribuir */
    r = fechar_caixa_imediata_manual(&sm, &hash, 0);

    CHECK(r == 0,
          "bug2b: fechar_caixa_imediata_manual retorna 0 sem outra caixa ABERTA");
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA,
          "bug2b: caixa nao e fechada quando fecho falha");
    CHECK(sm.caixas[0].emAtendimento == c0,
          "bug3: emAtendimento intacto apos fecho falhado");
    CHECK(sm.caixas[0].tempoRestanteAtendimento == tempo_restante_original,
          "bug3: tempoRestanteAtendimento intacto apos fecho falhado");
    CHECK(c0->instanteInicioAtendimento == inicio_original,
          "bug3: instanteInicioAtendimento intacto apos fecho falhado");
    CHECK(c0->estavaEmAtendimento == estava_original,
          "bug3: estavaEmAtendimento intacto apos fecho falhado");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 5 — Bug #6: snapshot round-trip preserva totais globais e ultimoOperador
   ------------------------------------------------------------------ */
static void test_bug6_snapshot_preserva_totais_e_ultimo_operador(void) {
    Supermercado sm_orig = make_sm(2, 120, 1, 10);
    Supermercado sm_reload;
    HashClientes hash_orig, hash_reload;
    FILE *f;
    hash_init(&hash_orig);
    hash_init(&hash_reload);

    /* definir totais e operadores conhecidos */
    sm_orig.totalProdutosOferecidos = 7;
    sm_orig.totalValorOferecido     = 12.50f;
    strncpy(sm_orig.caixas[0].ultimoOperador, "Operador_A", MAX_OPERADOR - 1);
    strncpy(sm_orig.caixas[1].ultimoOperador, "Operador_B", MAX_OPERADOR - 1);
    sm_orig.caixas[0].estado = CAIXA_ABERTA;
    sm_orig.caixas[1].estado = CAIXA_FECHADA;

    guardar_snapshot(TMP_SNAP, &sm_orig, NULL);
    supermercado_destruir(&sm_orig, &hash_orig);

    /* verificar que o ficheiro contem as novas linhas */
    f = fopen(TMP_SNAP, "r");
    if (f) {
        char linha[256];
        int found_total_prod = 0, found_total_val = 0, found_ultimo_op = 0;
        while (fgets(linha, sizeof(linha), f)) {
            if (strncmp(linha, "TOTAL_PRODUTOS_OFERECIDOS", 25) == 0) found_total_prod = 1;
            if (strncmp(linha, "TOTAL_VALOR_OFERECIDO", 21) == 0) found_total_val = 1;
            if (strncmp(linha, "ULTIMO_OPERADOR", 15) == 0) found_ultimo_op = 1;
        }
        fclose(f);
        CHECK(found_total_prod, "bug6: snapshot contem TOTAL_PRODUTOS_OFERECIDOS");
        CHECK(found_total_val,  "bug6: snapshot contem TOTAL_VALOR_OFERECIDO");
        CHECK(found_ultimo_op,  "bug6: snapshot contem ULTIMO_OPERADOR");
    } else {
        printf("FAIL: bug6: nao foi possivel abrir snapshot (linha %d)\n", __LINE__);
        fail_count += 3;
    }

    /* recarregar e verificar valores */
    {
        Configuracao cfg;
        config_default(&cfg);
        cfg.nCaixas = 2;
        supermercado_init(&sm_reload, &cfg, NULL, NULL, NULL);
        carregar_dados_iniciais(TMP_SNAP, &sm_reload, &hash_reload);

        CHECK(sm_reload.totalProdutosOferecidos == 7,
              "bug6: totalProdutosOferecidos restaurado apos reload");
        CHECK(sm_reload.totalValorOferecido > 12.49f && sm_reload.totalValorOferecido < 12.51f,
              "bug6: totalValorOferecido restaurado apos reload");
        CHECK(strcmp(sm_reload.caixas[0].ultimoOperador, "Operador_A") == 0,
              "bug6: ultimoOperador da caixa 0 restaurado apos reload");
        CHECK(strcmp(sm_reload.caixas[1].ultimoOperador, "Operador_B") == 0,
              "bug6: ultimoOperador da caixa 1 restaurado apos reload");

        supermercado_destruir(&sm_reload, &hash_reload);
    }

    remove(TMP_SNAP);
}

int main(void) {
    printf("=== Issue 11: Bugfixes ===\n\n");
    test_bug1_ultimo_operador_persiste();
    test_bug7_catalogo_preco_clamped();
    test_bug2a_fechar_imediata_recusa_se_so_a_fechar();
    test_bug2b3_campos_intactos_quando_fecho_falha();
    test_bug6_snapshot_preserva_totais_e_ultimo_operador();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
