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

#define TMP_SNAP "output/test_snap_issue10.txt"

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

static Produto make_produto(int id, float preco, int tempoCompra, int tempoPassagem) {
    Produto p;
    memset(&p, 0, sizeof(p));
    p.id           = id;
    strncpy(p.nome, "Test", MAX_NOME - 1);
    p.preco        = preco;
    p.tempoCompra  = tempoCompra;
    p.tempoPassagem = tempoPassagem;
    p.oferecido    = false;
    return p;
}

/* ------------------------------------------------------------------
   Ciclo 1 — Bug #2: inserir_novo_cliente valida ID com id_valido()
   ------------------------------------------------------------------ */
static void test_bug2_inserir_rejeita_id_invalido(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    Produto p = make_produto(1, 1.0f, 1, 1);
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;

    CHECK(inserir_novo_cliente(&sm, &hash, "88888",  "Teste", &p, 1) == INSERIR_CLIENTE_INVALIDO,
          "bug2: ID com 5 digitos rejeitado por inserir_novo_cliente");
    CHECK(inserir_novo_cliente(&sm, &hash, "ABC123", "Teste", &p, 1) == INSERIR_CLIENTE_INVALIDO,
          "bug2: ID com letras rejeitado por inserir_novo_cliente");
    CHECK(sm.nClientesEmLoja == 0,
          "bug2: nenhum cliente inserido com IDs invalidos");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 2 — Bug #4: cliente nao desaparece quando sem destino
   ------------------------------------------------------------------ */
static void test_bug4_cliente_nao_desaparece(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    Produto p = make_produto(1, 1.0f, 1, 1);
    hash_init(&hash);

    /* Unica caixa em A_FECHAR: nao aceita novos clientes */
    sm.caixas[0].estado = CAIXA_A_FECHAR;
    strncpy(sm.caixas[0].operador, "Op", MAX_OPERADOR - 1);

    inserir_novo_cliente(&sm, &hash, "123456", "Teste", &p, 1);
    CHECK(sm.nClientesEmLoja == 1, "bug4: cliente inserido na loja");

    /* Avancar alem do tempoCompra=1 para forcar tentativa de entrar na fila */
    avancar_simulacao(&sm, &hash, 3);

    CHECK(sm.nClientesEmLoja == 1,
          "bug4: cliente nao desaparece quando nao existe destino");
    CHECK(sm.clientesEmLoja[0]->comprasTerminadas == false,
          "bug4: comprasTerminadas nao definido enquanto cliente nao entrou em fila");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 3 — Bug #5: fecho suave de caixa vazia fecha imediatamente
   ------------------------------------------------------------------ */
static void test_bug5_fecho_suave_caixa_vazia(void) {
    Supermercado sm = make_sm(2, 120, 1, 10);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op0", MAX_OPERADOR - 1);
    sm.caixas[1].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[1].operador, "Op1", MAX_OPERADOR - 1);

    /* Caixa 1 vazia: fecho suave deve fechar imediatamente */
    int r = fechar_caixa_suave_manual(&sm, 1);

    CHECK(r == 1,
          "bug5: fechar_caixa_suave_manual retorna 1");
    CHECK(sm.caixas[1].estado == CAIXA_FECHADA,
          "bug5: caixa vazia fecha imediatamente no fecho suave");
    CHECK(sm.caixas[1].operador[0] == '\0',
          "bug5: operador limpo apos fecho imediato de caixa vazia");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 4 — Bug #6a: abrir caixa ja aberta retorna 0, nao troca operador
   ------------------------------------------------------------------ */
static void test_bug6a_abrir_caixa_ja_aberta(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "OpOriginal", MAX_OPERADOR - 1);

    int r = abrir_caixa_manual(&sm, 0);

    CHECK(r == 0,
          "bug6a: abrir caixa ja aberta retorna 0");
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA,
          "bug6a: estado nao muda");
    CHECK(strcmp(sm.caixas[0].operador, "OpOriginal") == 0,
          "bug6a: operador nao e substituido ao tentar abrir caixa ja aberta");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 5 — Bug #6b: abrir caixa A_FECHAR retorna 0, estado nao muda
   ------------------------------------------------------------------ */
static void test_bug6b_abrir_caixa_a_fechar(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_A_FECHAR;
    strncpy(sm.caixas[0].operador, "OpFechando", MAX_OPERADOR - 1);

    int r = abrir_caixa_manual(&sm, 0);

    CHECK(r == 0,
          "bug6b: abrir caixa A_FECHAR retorna 0");
    CHECK(sm.caixas[0].estado == CAIXA_A_FECHAR,
          "bug6b: estado A_FECHAR nao muda");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 6 — Bug #7: MAX_ESPERA usa espera frozen para emAtendimento
   ------------------------------------------------------------------ */
static void test_bug7_max_espera_espera_frozen(void) {
    Supermercado sm = make_sm(1, 5, 3, 7); /* maxEspera = 5 */
    HashClientes hash;
    Produto p = make_produto(1, 2.0f, 1, 100);
    Produto *copia;
    Cliente *c;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;

    copia = (Produto *)malloc(sizeof(Produto));
    *copia = p;
    c = criar_cliente("654321", "Test", 1, 0, 0, copia);
    c->instanteEntradaFila        = 0;
    c->instanteInicioAtendimento  = 1; /* espera real na fila = 1 < maxEspera=5 */
    c->comprasTerminadas          = true;
    sm.caixas[0].emAtendimento              = c;
    sm.caixas[0].tempoRestanteAtendimento   = 98;
    hash_inserir(&hash, c, 0);

    /* Avancar 10 passos: instanteAtual vai de 0 a 10.
       BUG: espera = 10 - 0 = 10 > 5 -> oferta (errado)
       FIX: espera = 1  - 0 = 1  < 5 -> sem oferta (correto) */
    avancar_simulacao(&sm, &hash, 10);

    CHECK(sm.totalProdutosOferecidos == 0,
          "bug7: MAX_ESPERA nao accionado quando espera frozen < maxEspera");
    CHECK(c->oferecimentoFeito == false,
          "bug7: oferecimentoFeito permanece false com espera real < maxEspera");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 7 — Bug #8: auto-fecho usa menos pessoas (nao menor fila)
   ------------------------------------------------------------------ */
static void test_bug8_auto_fechar_menos_pessoas(void) {
    Supermercado sm = make_sm(2, 120, 1, 10);
    HashClientes hash;
    Produto p = make_produto(1, 2.0f, 1, 50);
    Produto *copia;
    Cliente *c;
    hash_init(&hash);

    sm.caixas[0].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[0].operador, "Op0", MAX_OPERADOR - 1);
    sm.caixas[1].estado = CAIXA_ABERTA;
    strncpy(sm.caixas[1].operador, "Op1", MAX_OPERADOR - 1);

    /* Caixa 0: tem 1 cliente em atendimento -> total=1, fila=0 */
    copia = (Produto *)malloc(sizeof(Produto));
    *copia = p;
    c = criar_cliente("111111", "Test", 1, 0, 0, copia);
    c->comprasTerminadas = true;
    c->instanteEntradaFila = 0;
    sm.caixas[0].emAtendimento            = c;
    sm.caixas[0].tempoRestanteAtendimento = 45;
    hash_inserir(&hash, c, 0);

    /* Caixa 1: completamente vazia -> total=0, fila=0
       media = (0+0)/2 = 0 < minFila=1 -> politica de fecho activa */
    sm.instanteAtual               = 10;
    sm.instanteUltimaAberturaManual = 0;

    verificar_politica_caixas(&sm, &hash);

    /* BUG: menor fila -> empate -> fecha caixa 0 (tem cliente em atendimento!)
       FIX: menos pessoas -> fecha caixa 1 (vazia, fecha imediatamente) */
    CHECK(sm.caixas[0].estado == CAIXA_ABERTA,
          "bug8: caixa com cliente em atendimento nao e seleccionada para fecho");
    CHECK(sm.caixas[1].estado == CAIXA_FECHADA,
          "bug8: caixa vazia e seleccionada e fecha imediatamente");

    supermercado_destruir(&sm, &hash);
}

/* ------------------------------------------------------------------
   Ciclo 8 — Bug #10: snapshot recompoe campos de ofertas do cliente
   ------------------------------------------------------------------ */
static void test_bug10_snapshot_recomputa_ofertas(void) {
    Supermercado sm = make_sm(1, 120, 3, 7);
    HashClientes hash;
    FILE *f;
    hash_init(&hash);

    /* Escrever snapshot com cliente que tem produto oferecido */
    f = fopen(TMP_SNAP, "w");
    fprintf(f, "INSTANTE 10\n");
    fprintf(f, "ULTIMO_FECHO -1\n");
    fprintf(f, "ULTIMA_ABERTURA_MANUAL -4\n");
    fprintf(f, "EM_LOJA 1\n");
    fprintf(f, "  LOJA_CLIENTE 123456 Teste entrada_loja=0 tempo_compra=100\n");
    fprintf(f, "    PRODUTO 1 Leite 1.50 10 2 true\n");
    fprintf(f, "CAIXAS 1\n");
    fprintf(f, "CAIXA 1 FECHADA -\n");
    fprintf(f, "  FILA 0\n");
    fclose(f);

    carregar_dados_iniciais(TMP_SNAP, &sm, &hash);

    CHECK(sm.nClientesEmLoja == 1,
          "bug10: cliente carregado do snapshot");
    if (sm.nClientesEmLoja == 1) {
        Cliente *cl = sm.clientesEmLoja[0];
        CHECK(cl->oferecimentoFeito == true,
              "bug10: oferecimentoFeito recalculado apos carregar snapshot");
        CHECK(cl->produtosOferecidos == 1,
              "bug10: produtosOferecidos recalculado apos carregar snapshot");
        CHECK(cl->valorOferecido > 0.0f,
              "bug10: valorOferecido recalculado apos carregar snapshot");
    }

    remove(TMP_SNAP);
    supermercado_destruir(&sm, &hash);
}

int main(void) {
    printf("=== Issue 10: Bugfixes ===\n\n");
    test_bug2_inserir_rejeita_id_invalido();
    test_bug4_cliente_nao_desaparece();
    test_bug5_fecho_suave_caixa_vazia();
    test_bug6a_abrir_caixa_ja_aberta();
    test_bug6b_abrir_caixa_a_fechar();
    test_bug7_max_espera_espera_frozen();
    test_bug8_auto_fechar_menos_pessoas();
    test_bug10_snapshot_recomputa_ofertas();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
