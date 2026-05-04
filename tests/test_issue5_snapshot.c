#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client_registry.h"
#include "store.h"
#include "hash.h"
#include "utils.h"
#include "config.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define TMP_REG  "output/test_registo_tmp.txt"
#define TMP_SNAP "output/test_snap_tmp.txt"

/* ------------------------------------------------------------------ */
/* Cycle 1: registo_adicionar — ID válido + nome → em memória e ficheiro */
/* ------------------------------------------------------------------ */

static void test_adicionar_valido(void) {
    RegistoClientes reg = {NULL, 0, 0};
    int r;
    EntradaCliente *e;

    remove(TMP_REG);
    r = registo_adicionar(&reg, "123456", "Joao Teste", TMP_REG);
    CHECK(r == 1, "registo_adicionar retorna 1 para ID valido");
    CHECK(reg.tamanho == 1, "tamanho aumenta para 1 apos adicionar");
    e = registo_pesquisar_id(&reg, "123456");
    CHECK(e != NULL, "registo_pesquisar_id encontra o ID recem adicionado");
    if (e) {
        CHECK(strcmp(e->nome, "Joao Teste") == 0, "nome guardado correctamente em memoria");
    }
    /* check file */
    {
        FILE *f = fopen(TMP_REG, "r");
        char linha[256] = {0};
        int found = 0;
        if (f) {
            while (fgets(linha, sizeof(linha), f)) {
                if (strstr(linha, "123456") && strstr(linha, "Joao Teste")) found = 1;
            }
            fclose(f);
        }
        CHECK(found, "entrada gravada no ficheiro com ID e nome");
    }
    free(reg.lista);
}

static void test_adicionar_id_curto(void) {
    RegistoClientes reg = {NULL, 0, 0};
    int r = registo_adicionar(&reg, "12345", "Curto Demais", TMP_REG);
    CHECK(r == 0, "registo_adicionar rejeita ID com 5 chars");
    CHECK(reg.tamanho == 0, "tamanho nao aumenta com ID curto");
    free(reg.lista);
}

static void test_adicionar_id_nao_numerico(void) {
    RegistoClientes reg = {NULL, 0, 0};
    int r = registo_adicionar(&reg, "12345A", "Nome Valido", TMP_REG);
    CHECK(r == 0, "registo_adicionar rejeita ID nao numerico");
    CHECK(reg.tamanho == 0, "tamanho nao aumenta com ID nao numerico");
    free(reg.lista);
}

static void test_adicionar_nome_vazio(void) {
    RegistoClientes reg = {NULL, 0, 0};
    int r = registo_adicionar(&reg, "654321", "", TMP_REG);
    CHECK(r == 0, "registo_adicionar rejeita nome vazio");
    CHECK(reg.tamanho == 0, "tamanho nao aumenta com nome vazio");
    free(reg.lista);
}

static void test_adicionar_duplicado_nao_duplica(void) {
    RegistoClientes reg = {NULL, 0, 0};
    remove(TMP_REG);
    registo_adicionar(&reg, "111111", "Primeiro", TMP_REG);
    int r = registo_adicionar(&reg, "111111", "Duplicado", TMP_REG);
    CHECK(r == 1, "registo_adicionar retorna 1 para ID ja existente");
    CHECK(reg.tamanho == 1, "tamanho nao aumenta ao adicionar duplicado");
    free(reg.lista);
}

/* helper: cria Supermercado mínimo com 2 caixas */
static Supermercado make_sm(void) {
    Supermercado sm;
    Configuracao cfg;
    config_default(&cfg);
    cfg.nCaixas = 2;
    cfg.minFila = 1;
    cfg.maxFila = 10;
    supermercado_init(&sm, &cfg, NULL, NULL, NULL);
    sm.caixas[0].estado = CAIXA_ABERTA;
    return sm;
}

/* ------------------------------------------------------------------ */
/* Cycle 6: roundtrip — instanteAtual preservado                       */
/* ------------------------------------------------------------------ */
static void test_roundtrip_instante(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h2;
    hash_init(&h2);

    sm.instanteAtual = 42;
    guardar_snapshot(TMP_SNAP, &sm, NULL);

    sm2.instanteAtual = 0;
    carregar_dados_iniciais(TMP_SNAP, &sm2, &h2);

    CHECK(sm2.instanteAtual == 42, "roundtrip: instanteAtual restaurado para 42");

    supermercado_destruir(&sm, NULL);
    supermercado_destruir(&sm2, &h2);
}

/* helper: cria Produto simples */
static Produto make_produto(int id, const char *nome, float preco, int tempo) {
    Produto p;
    p.id = id;
    strncpy(p.nome, nome, MAX_NOME - 1);
    p.nome[MAX_NOME - 1] = '\0';
    p.preco = preco;
    p.stock = 10.0f;
    p.tempoPassagem = tempo;
    p.oferecido = false;
    return p;
}

/* helper: insere cliente com produtos na fila da caixa */
static void inserir_na_fila(Supermercado *sm, HashClientes *hash, int caixa_idx,
                             const char *id, const char *nome, Produto *prods, int np) {
    Produto *copia = (Produto *)malloc(sizeof(Produto) * np);
    int k;
    Cliente *c;
    for (k = 0; k < np; k++) copia[k] = prods[k];
    c = criar_cliente(id, nome, np, sm->instanteAtual, caixa_idx, copia);
    if (!c) return;
    fila_inserir(&sm->caixas[caixa_idx].fila, c);
    hash_inserir(hash, c, caixa_idx);
}

/* ------------------------------------------------------------------ */
/* Cycle 7: roundtrip — numero de clientes em fila preservado          */
/* ------------------------------------------------------------------ */
static void test_roundtrip_clientes_fila(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h, h2;
    Produto p1, p2;
    hash_init(&h); hash_init(&h2);

    p1 = make_produto(10001, "Azeitona", 3.0f, 3);
    p2 = make_produto(10002, "Pao", 1.0f, 2);

    inserir_na_fila(&sm, &h, 0, "111111", "Ana Lima", &p1, 1);
    inserir_na_fila(&sm, &h, 0, "222222", "Bruno Mar", &p2, 1);

    guardar_snapshot(TMP_SNAP, &sm, NULL);

    carregar_dados_iniciais(TMP_SNAP, &sm2, &h2);

    CHECK(sm2.caixas[0].fila.tamanho == 2,
          "roundtrip: 2 clientes na fila da caixa 0 restaurados");

    supermercado_destruir(&sm, &h);
    supermercado_destruir(&sm2, &h2);
}

/* ------------------------------------------------------------------ */
/* Cycle 8: roundtrip — produtos do cliente em fila preservados        */
/* ------------------------------------------------------------------ */
static void test_roundtrip_produtos_cliente(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h, h2;
    Produto prods[2];
    hash_init(&h); hash_init(&h2);

    prods[0] = make_produto(10001, "Azeitona Verde", 3.0f, 4);
    prods[1] = make_produto(10002, "Pao Tostado", 1.69f, 3);

    inserir_na_fila(&sm, &h, 0, "333333", "Carlos So", prods, 2);
    guardar_snapshot(TMP_SNAP, &sm, NULL);
    carregar_dados_iniciais(TMP_SNAP, &sm2, &h2);

    {
        const NoFila *no = sm2.caixas[0].fila.cabeca;
        CHECK(no != NULL && no->cliente->nProdutos == 2,
              "roundtrip: cliente em fila tem 2 produtos restaurados");
        if (no && no->cliente->nProdutos == 2) {
            CHECK(no->cliente->produtos[0].id == 10001,
                  "roundtrip: id do primeiro produto preservado (10001)");
            CHECK(no->cliente->produtos[1].id == 10002,
                  "roundtrip: id do segundo produto preservado (10002)");
        }
    }

    supermercado_destruir(&sm, &h);
    supermercado_destruir(&sm2, &h2);
}

/* ------------------------------------------------------------------ */
/* Cycle 9: roundtrip — tempoRestanteAtendimento do cliente em serviço */
/* ------------------------------------------------------------------ */
static void test_roundtrip_tempo_restante_atendimento(void) {
    Supermercado sm = make_sm();
    Supermercado sm2 = make_sm();
    HashClientes h, h2;
    Produto p;
    Produto *copia;
    Cliente *c;
    hash_init(&h); hash_init(&h2);

    p = make_produto(10003, "Leite", 0.80f, 2);
    copia = (Produto *)malloc(sizeof(Produto));
    *copia = p;
    c = criar_cliente("444444", "Diana Dias", 1, sm.instanteAtual, 0, copia);
    c->instanteInicioAtendimento = 10;
    sm.caixas[0].emAtendimento = c;
    sm.caixas[0].tempoRestanteAtendimento = 7;

    guardar_snapshot(TMP_SNAP, &sm, NULL);
    carregar_dados_iniciais(TMP_SNAP, &sm2, &h2);

    CHECK(sm2.caixas[0].emAtendimento != NULL,
          "roundtrip: cliente em atendimento restaurado");
    CHECK(sm2.caixas[0].tempoRestanteAtendimento == 7,
          "roundtrip: tempoRestanteAtendimento == 7 restaurado");

    supermercado_destruir(&sm, &h);
    supermercado_destruir(&sm2, &h2);
}

int main(void) {
    printf("=== Issue 5: snapshot e registo_adicionar ===\n\n");
    test_adicionar_valido();
    test_adicionar_id_curto();
    test_adicionar_id_nao_numerico();
    test_adicionar_nome_vazio();
    test_adicionar_duplicado_nao_duplica();
    test_roundtrip_instante();
    test_roundtrip_clientes_fila();
    test_roundtrip_produtos_cliente();
    test_roundtrip_tempo_restante_atendimento();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
