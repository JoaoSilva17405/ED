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

static void test_tempo_passagem_minimo_2(void) {
    /* linha 5: tempoPassagem=0.8 -> ceil(0.8)=1 -> max(2,1)=2 */
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    int i, ok = 1;
    if (!cat) { fail_count++; return; }
    for (i = 0; i < cat->tamanho; i++) {
        if (cat->lista[i].tempoPassagem < 2) { ok = 0; break; }
    }
    CHECK(ok, "tempoPassagem >= 2 para todos os produtos apos conversao");
    catalog_destruir(cat);
}

static void test_obter_produtos_aleatorios(void) {
    Configuracao cfg;
    Produto *prods;
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    if (!cat) { fail_count++; return; }
    cfg.tempoAtendimentoProduto = 6;
    cfg.maxPreco = 40.0f;
    prods = catalog_obter_produtos_aleatorios(cat, 5, &cfg);
    CHECK(prods != NULL, "catalog_obter_produtos_aleatorios retorna nao-NULL");
    if (prods) {
        int i, todos_reais = 1;
        for (i = 0; i < 5; i++) {
            if (prods[i].nome[0] == '\0') { todos_reais = 0; break; }
            if (prods[i].tempoPassagem < 2 || prods[i].tempoPassagem > 6) { todos_reais = 0; break; }
        }
        CHECK(todos_reais, "5 produtos com nomes reais e tempo em [2, 6]");
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
    test_tempo_passagem_minimo_2();
    test_obter_produtos_aleatorios();
    test_ficheiro_inexistente();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
