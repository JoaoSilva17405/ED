#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "catalog.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

static void test_pesquisa_nome_parcial_devolve_resultados(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Produto *resultados[10];
    int n;
    if (!cat) { fail_count++; return; }
    n = catalog_pesquisar(cat, "Azeitona", resultados, 10);
    CHECK(n > 0, "pesquisa 'Azeitona' devolve pelo menos 1 resultado");
    if (n > 0) {
        int i, todos_tem_azeitona = 1;
        for (i = 0; i < n; i++) {
            if (!strstr(resultados[i]->nome, "Azeitona") &&
                !strstr(resultados[i]->nome, "azeitona")) {
                todos_tem_azeitona = 0;
                break;
            }
        }
        CHECK(todos_tem_azeitona, "todos os resultados contêm 'Azeitona' no nome");
    }
    catalog_destruir(cat);
}

static void test_sem_resultados_devolve_zero(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Produto *resultados[10];
    int n;
    if (!cat) { fail_count++; return; }
    n = catalog_pesquisar(cat, "xyzxyzxyz_inexistente", resultados, 10);
    CHECK(n == 0, "pesquisa sem resultados devolve 0");
    catalog_destruir(cat);
}

static void test_pesquisa_id_exato(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Produto *resultados[10];
    int n;
    if (!cat) { fail_count++; return; }
    n = catalog_pesquisar(cat, "10001", resultados, 10);
    CHECK(n == 1, "pesquisa por ID '10001' devolve exatamente 1 resultado");
    if (n == 1) {
        CHECK(resultados[0]->id == 10001, "resultado tem id == 10001");
    }
    catalog_destruir(cat);
}

static void test_limite_max_respeitado(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Produto *resultados[3];
    int n;
    if (!cat) { fail_count++; return; }
    n = catalog_pesquisar(cat, "Azeitona", resultados, 3);
    CHECK(n <= 3, "pesquisa com max=3 devolve no maximo 3 resultados");
    catalog_destruir(cat);
}

static void test_pesquisa_case_insensitive(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Produto *r_lower[10], *r_upper[10];
    int n_lower, n_upper;
    if (!cat) { fail_count++; return; }
    n_lower = catalog_pesquisar(cat, "azeitona", r_lower, 10);
    n_upper = catalog_pesquisar(cat, "Azeitona", r_upper, 10);
    CHECK(n_lower > 0 && n_lower == n_upper,
          "pesquisa 'azeitona' e 'Azeitona' devolvem o mesmo numero de resultados");
    catalog_destruir(cat);
}

static void test_catalogo_null_devolve_zero(void) {
    Produto *resultados[10];
    int n = catalog_pesquisar(NULL, "Azeitona", resultados, 10);
    CHECK(n == 0, "catalogo NULL devolve 0");
}

static void test_termo_vazio_devolve_zero(void) {
    CatalogoProdutos *cat = catalog_carregar("Produtos.txt");
    Produto *resultados[10];
    int n;
    if (!cat) { fail_count++; return; }
    n = catalog_pesquisar(cat, "", resultados, 10);
    CHECK(n == 0, "termo vazio devolve 0");
    catalog_destruir(cat);
}

int main(void) {
    printf("=== Issue 5: catalog_pesquisar ===\n\n");
    test_pesquisa_nome_parcial_devolve_resultados();
    test_sem_resultados_devolve_zero();
    test_pesquisa_id_exato();
    test_limite_max_respeitado();
    test_pesquisa_case_insensitive();
    test_catalogo_null_devolve_zero();
    test_termo_vazio_devolve_zero();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
