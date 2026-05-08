#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"
#include "hash.h"
#include "config.h"
#include "catalog.h"
#include "client_registry.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define TMP_REG "output/tmp_registo_test11.txt"

static RegistoClientes *criar_registo_teste(void) {
    FILE *f = fopen(TMP_REG, "w");
    if (!f) return NULL;
    fprintf(f, "000001\tCliente Auto Um\n");
    fprintf(f, "000002\tCliente Auto Dois\n");
    fclose(f);
    return registo_carregar(TMP_REG);
}

static Supermercado make_sm(CatalogoProdutos *cat) {
    Supermercado sm;
    Configuracao cfg;
    config_default(&cfg);
    cfg.nCaixas          = 2;
    cfg.intervaloChegada = 8;
    supermercado_init(&sm, &cfg, NULL, NULL, cat);
    return sm;
}

/* Ciclo 1: tick coincide com intervalo e cliente disponivel -> insere */
static void test_insere_quando_tick_correto(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Supermercado sm = make_sm(cat);
    HashClientes hash;
    hash_init(&hash);
    RegistoClientes *reg = criar_registo_teste();

    sm.instanteAtual = sm.cfg.intervaloChegada; /* tick 8 % 8 == 0 */
    int r = tentar_inserir_cliente_automatico(&sm, &hash, reg);

    CHECK(r == 1, "retorna 1 quando cliente inserido");
    CHECK(sm.nClientesEmLoja == 1, "nClientesEmLoja aumentou para 1");

    supermercado_destruir(&sm, &hash);
    registo_destruir(reg);
    catalog_destruir(cat);
    remove(TMP_REG);
}

/* Ciclo 2: tick nao coincide com intervalo -> nao insere */
static void test_nao_insere_tick_errado(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Supermercado sm = make_sm(cat);
    HashClientes hash;
    hash_init(&hash);
    RegistoClientes *reg = criar_registo_teste();

    sm.instanteAtual = sm.cfg.intervaloChegada - 1; /* tick 7 % 8 != 0 */
    int r = tentar_inserir_cliente_automatico(&sm, &hash, reg);

    CHECK(r == 0, "retorna 0 quando tick nao coincide com intervalo");
    CHECK(sm.nClientesEmLoja == 0, "nClientesEmLoja permanece 0");

    supermercado_destruir(&sm, &hash);
    registo_destruir(reg);
    catalog_destruir(cat);
    remove(TMP_REG);
}

/* Ciclo 3: todos os clientes ja estao na hash -> nao insere */
static void test_nao_insere_todos_na_hash(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Supermercado sm = make_sm(cat);
    HashClientes hash;
    hash_init(&hash);
    RegistoClientes *reg = criar_registo_teste();

    sm.instanteAtual = sm.cfg.intervaloChegada;

    /* inserir ambos os clientes manualmente na hash (simula que ja estao no sistema) */
    int r1 = tentar_inserir_cliente_automatico(&sm, &hash, reg);
    CHECK(r1 == 1, "primeiro cliente inserido (setup ciclo 3)");

    /* avanca para proximo tick multiplo e insere o segundo */
    sm.instanteAtual = sm.cfg.intervaloChegada * 2;
    int r2 = tentar_inserir_cliente_automatico(&sm, &hash, reg);
    CHECK(r2 == 1, "segundo cliente inserido (setup ciclo 3)");

    /* agora ambos estao no sistema — proximo tick multiplo deve retornar 0 */
    sm.instanteAtual = sm.cfg.intervaloChegada * 3;
    int r3 = tentar_inserir_cliente_automatico(&sm, &hash, reg);
    CHECK(r3 == 0, "retorna 0 quando todos os clientes ja estao no sistema");
    CHECK(sm.nClientesEmLoja == 2, "nClientesEmLoja nao aumentou alem de 2");

    supermercado_destruir(&sm, &hash);
    registo_destruir(reg);
    catalog_destruir(cat);
    remove(TMP_REG);
}

/* Ciclo 4: cliente removido da hash pode re-entrar */
static void test_reentrada_apos_remocao(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Supermercado sm = make_sm(cat);
    HashClientes hash;
    hash_init(&hash);
    RegistoClientes *reg = criar_registo_teste();

    sm.instanteAtual = sm.cfg.intervaloChegada;
    int r1 = tentar_inserir_cliente_automatico(&sm, &hash, reg);
    CHECK(r1 == 1, "cliente 000001 inserido na primeira chegada");
    CHECK(hash_pesquisar(&hash, "000001") != NULL || hash_pesquisar(&hash, "000002") != NULL,
          "pelo menos um cliente esta na hash apos insercao");

    /* simula que o cliente foi atendido e removido da hash */
    hash_remover(&hash, "000001");
    hash_remover(&hash, "000002");
    sm.nClientesEmLoja = 0;

    sm.instanteAtual = sm.cfg.intervaloChegada * 2;
    int r2 = tentar_inserir_cliente_automatico(&sm, &hash, reg);
    CHECK(r2 == 1, "cliente pode re-entrar apos ser removido da hash");
    CHECK(sm.nClientesEmLoja == 1, "nClientesEmLoja volta a 1 apos re-entrada");

    supermercado_destruir(&sm, &hash);
    registo_destruir(reg);
    catalog_destruir(cat);
    remove(TMP_REG);
}

int main(void) {
    printf("=== Issue 11: Chegada automatica de clientes ===\n\n");
    test_insere_quando_tick_correto();
    test_nao_insere_tick_errado();
    test_nao_insere_todos_na_hash();
    test_reentrada_apos_remocao();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
