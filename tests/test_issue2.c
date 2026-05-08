#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "catalog.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

static void test_carregar_nao_null(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    CHECK(cat != NULL, "catalog_carregar(Produtos.txt) retorna nao-NULL");
    catalog_destruir(cat);
}

static void test_contagem(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    CHECK(cat != NULL && cat->tamanho > 9000, "mais de 9000 produtos carregados");
    catalog_destruir(cat);
}

static void test_preco_sem_truncamento(void) {
    /* linha 2: 10002\tCroutons...\t1.69\t6.2\t2.3 -> preco deve ser ~1.69 */
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    if (!cat) { fail_count++; return; }
    CHECK(cat->lista[1].preco > 1.68f && cat->lista[1].preco < 1.70f,
          "preco do produto 2 preservado sem truncamento (1.69)");
    catalog_destruir(cat);
}

static void test_tempo_passagem_preservado_como_float(void) {
    /* linha 2: tempoPassagem=2.3 -> deve ser preservado como float ~2.3 */
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    if (!cat) { fail_count++; return; }
    CHECK(cat->lista[1].tempoPassagem > 2.29f && cat->lista[1].tempoPassagem < 2.31f,
          "tempoPassagem do produto 2 preservado como float (2.3)");
    catalog_destruir(cat);
}

static void test_tempo_compra_preservado_como_float(void) {
    /* linha 2: tempoCompra=6.2 -> deve ser preservado como float ~6.2 */
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    if (!cat) { fail_count++; return; }
    CHECK(cat->lista[1].tempoCompra > 6.19f && cat->lista[1].tempoCompra < 6.21f,
          "tempoCompra do produto 2 preservado como float (6.2)");
    catalog_destruir(cat);
}

static void test_tempo_passagem_sem_minimo_artificial(void) {
    /* catalogo tem produtos com tempoPassagem < 1 (ex: 0.4) — devem ser preservados */
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    int i, encontrou_abaixo_de_1 = 0;
    if (!cat) { fail_count++; return; }
    for (i = 0; i < cat->tamanho; i++) {
        if (cat->lista[i].tempoPassagem < 1.0f) { encontrou_abaixo_de_1 = 1; break; }
    }
    CHECK(encontrou_abaixo_de_1, "tempoPassagem < 1.0 preservado do ficheiro (sem minimo artificial)");
    catalog_destruir(cat);
}

static void test_obter_produtos_aleatorios(void) {
    Configuracao cfg;
    Produto *prods;
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    if (!cat) { fail_count++; return; }
    cfg.maxPreco = 40.0f;
    prods = catalog_obter_produtos_aleatorios(cat, 5, &cfg);
    CHECK(prods != NULL, "catalog_obter_produtos_aleatorios retorna nao-NULL");
    if (prods) {
        int i, todos_reais = 1;
        for (i = 0; i < 5; i++) {
            if (prods[i].nome[0] == '\0') { todos_reais = 0; break; }
            if (prods[i].tempoPassagem <= 0.0f) { todos_reais = 0; break; }
        }
        CHECK(todos_reais, "5 produtos com nomes reais e tempoPassagem > 0");
        free(prods);
    }
    catalog_destruir(cat);
}

static void test_ficheiro_inexistente(void) {
    CatalogoProdutos *cat = catalog_carregar("nao_existe.txt");
    CHECK(cat == NULL, "ficheiro inexistente retorna NULL");
}

int main(void) {
    printf("=== Issue 2: catalog ===\n\n");
    test_carregar_nao_null();
    test_contagem();
    test_preco_sem_truncamento();
    test_tempo_passagem_preservado_como_float();
    test_tempo_compra_preservado_como_float();
    test_tempo_passagem_sem_minimo_artificial();
    test_obter_produtos_aleatorios();
    test_ficheiro_inexistente();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
