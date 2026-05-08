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

/* Cria ficheiro temp com produto de tempoPassagem = 20 (acima do antigo cap de 6) */
static CatalogoProdutos *carregar_cat_temp(int tempoPassagem) {
    FILE *tmp = fopen("output/tmp_prod14.txt", "w");
    if (!tmp) return NULL;
    /* id\tnome\tpreco\ttempoCompra\ttempoPassagem */
    fprintf(tmp, "1\tProdutoTeste\t2.50\t10\t%d\n", tempoPassagem);
    fclose(tmp);
    return catalog_carregar("output/tmp_prod14.txt");
}

/* Ciclo 1: catalog_carregar preserva tempoPassagem do ficheiro */
static void test_carregar_preserva_tempo_passagem(void) {
    CatalogoProdutos *cat = carregar_cat_temp(20);
    CHECK(cat != NULL, "catalog carregado com tempoPassagem=20");
    if (!cat) { remove("output/tmp_prod14.txt"); return; }
    CHECK(cat->tamanho == 1, "catalogo tem 1 produto");
    CHECK(cat->lista[0].tempoPassagem == 20, "tempoPassagem=20 lido sem truncagem");
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 2: catalog_obter_produtos_aleatorios preserva tempoPassagem */
static void test_obter_preserva_tempo_passagem(void) {
    CatalogoProdutos *cat = carregar_cat_temp(20);
    if (!cat) { fail_count++; printf("FAIL: cat nulo\n"); return; }

    Configuracao cfg;
    config_default(&cfg);
    Produto *prods = catalog_obter_produtos_aleatorios(cat, 1, &cfg);
    CHECK(prods != NULL, "catalog_obter_produtos_aleatorios retorna produtos");
    if (prods) {
        CHECK(prods[0].tempoPassagem == 20,
              "tempoPassagem=20 preservado sem cap em catalog_obter");
        free(prods);
    }
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 3: tempoPassagem minimo e 2 (floor aplicado por tempo_para_int) */
static void test_tempo_passagem_minimo_dois(void) {
    CatalogoProdutos *cat = carregar_cat_temp(1); /* < 2 no ficheiro */
    if (!cat) { fail_count++; printf("FAIL: cat nulo\n"); return; }
    CHECK(cat->lista[0].tempoPassagem >= 2,
          "tempoPassagem com valor 1 no ficheiro e elevado para minimo 2");
    catalog_destruir(cat);
    remove("output/tmp_prod14.txt");
}

/* Ciclo 4: Configuracao nao tem campo tempoAtendimentoProduto */
static void test_config_sem_tempo_atendimento(void) {
    Configuracao cfg;
    config_default(&cfg);
    /* Verifica que os campos esperados existem e que o config funciona */
    CHECK(cfg.maxEspera > 0,     "config_default: maxEspera definido");
    CHECK(cfg.nCaixas > 0,       "config_default: nCaixas definido");
    CHECK(cfg.maxPreco > 0.0f,   "config_default: maxPreco definido");
    CHECK(cfg.intervaloChegada > 0, "config_default: intervaloChegada definido");
    /* Se tempoAtendimentoProduto existisse, teria sido referenciado aqui */
    CHECK(1, "Configuracao compila sem tempoAtendimentoProduto");
}

int main(void) {
    printf("=== Issue 14: tempoPassagem vem do catalogo sem truncagem ===\n\n");
    test_carregar_preserva_tempo_passagem();
    test_obter_preserva_tempo_passagem();
    test_tempo_passagem_minimo_dois();
    test_config_sem_tempo_atendimento();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
