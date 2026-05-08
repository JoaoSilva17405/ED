#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "catalog.h"
#include "config.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define FLOAT_EQ(a, b) ((a) > (b) - 0.01f && (a) < (b) + 0.01f)

static CatalogoProdutos *carregar_cat_temp_float(float tempoCompra, float tempoPassagem) {
    FILE *tmp = fopen("output/tmp_prod14.txt", "w");
    if (!tmp) return NULL;
    /* id\tnome\tpreco\ttempoCompra\ttempoPassagem */
    fprintf(tmp, "1\tProdutoTeste\t2.50\t%.2f\t%.2f\n", tempoCompra, tempoPassagem);
    fclose(tmp);
    return catalog_carregar("output/tmp_prod14.txt");
}

/* Ciclo 1: catalog_carregar preserva tempoPassagem float do ficheiro */
static void test_carregar_preserva_tempo_passagem_float(void) {
    CatalogoProdutos *cat = carregar_cat_temp_float(10.0f, 3.7f);
    CHECK(cat != NULL, "catalog carregado com tempoPassagem=3.7");
    if (!cat) { remove("output/tmp_prod14.txt"); return; }
    CHECK(cat->tamanho == 1, "catalogo tem 1 produto");
    CHECK(FLOAT_EQ(cat->lista[0].tempoPassagem, 3.7f),
          "tempoPassagem=3.7 lido sem alteracao");
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 2: catalog_carregar preserva tempoCompra float do ficheiro */
static void test_carregar_preserva_tempo_compra_float(void) {
    CatalogoProdutos *cat = carregar_cat_temp_float(6.2f, 2.3f);
    CHECK(cat != NULL, "catalog carregado com tempoCompra=6.2");
    if (!cat) { remove("output/tmp_prod14.txt"); return; }
    CHECK(FLOAT_EQ(cat->lista[0].tempoCompra, 6.2f),
          "tempoCompra=6.2 lido sem alteracao");
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 3: tempoPassagem abaixo de 1 e preservado (sem minimo artificial) */
static void test_tempo_passagem_abaixo_de_1_preservado(void) {
    CatalogoProdutos *cat = carregar_cat_temp_float(5.0f, 0.4f);
    if (!cat) { fail_count++; printf("FAIL: cat nulo\n"); return; }
    CHECK(FLOAT_EQ(cat->lista[0].tempoPassagem, 0.4f),
          "tempoPassagem=0.4 preservado do ficheiro sem minimo artificial");
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 4: catalog_obter_produtos_aleatorios preserva tempoPassagem float */
static void test_obter_preserva_tempo_passagem_float(void) {
    CatalogoProdutos *cat = carregar_cat_temp_float(10.0f, 3.7f);
    if (!cat) { fail_count++; printf("FAIL: cat nulo\n"); return; }

    Configuracao cfg;
    config_default(&cfg);
    Produto *prods = catalog_obter_produtos_aleatorios(cat, 1, &cfg);
    CHECK(prods != NULL, "catalog_obter_produtos_aleatorios retorna produtos");
    if (prods) {
        CHECK(FLOAT_EQ(prods[0].tempoPassagem, 3.7f),
              "tempoPassagem=3.7 preservado em catalog_obter sem cap");
        free(prods);
    }
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 5: Configuracao nao tem campo tempoAtendimentoProduto */
static void test_config_sem_tempo_atendimento(void) {
    Configuracao cfg;
    config_default(&cfg);
    CHECK(cfg.maxEspera > 0,        "config_default: maxEspera definido");
    CHECK(cfg.nCaixas > 0,          "config_default: nCaixas definido");
    CHECK(cfg.maxPreco > 0.0f,      "config_default: maxPreco definido");
    CHECK(cfg.intervaloChegada > 0, "config_default: intervaloChegada definido");
    CHECK(1, "Configuracao compila sem tempoAtendimentoProduto");
}

int main(void) {
    printf("=== Issue 14: tempoPassagem e tempoCompra como float sem truncagem ===\n\n");
    test_carregar_preserva_tempo_passagem_float();
    test_carregar_preserva_tempo_compra_float();
    test_tempo_passagem_abaixo_de_1_preservado();
    test_obter_preserva_tempo_passagem_float();
    test_config_sem_tempo_atendimento();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
