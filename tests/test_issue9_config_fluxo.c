#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

static void test_default_intervalo_chegada(void) {
    Configuracao cfg;
    config_default(&cfg);
    CHECK(FLUXO_CALMO == 15,         "FLUXO_CALMO vale 15");
    CHECK(FLUXO_NORMAL == 8,         "FLUXO_NORMAL vale 8");
    CHECK(FLUXO_HORA_DE_PONTA == 3,  "FLUXO_HORA_DE_PONTA vale 3");
    CHECK(cfg.intervaloChegada == FLUXO_NORMAL,
          "config_default define intervaloChegada como FLUXO_NORMAL");
}

static void test_carregar_intervalo_chegada(void) {
    FILE *tmp = fopen("output/tmp_cfg_test.txt", "w");
    if (!tmp) { fail_count++; printf("FAIL: nao foi possivel criar ficheiro temp\n"); return; }
    fprintf(tmp, "INTERVALO_CHEGADA 15\n");
    fclose(tmp);

    Configuracao cfg;
    int ok = carregar_configuracao("output/tmp_cfg_test.txt", &cfg);
    CHECK(ok == 1, "carregar_configuracao retorna 1 para ficheiro valido");
    CHECK(cfg.intervaloChegada == 15, "INTERVALO_CHEGADA 15 lido correctamente");
    remove("output/tmp_cfg_test.txt");
}

static void test_intervalo_chegada_fallback(void) {
    FILE *tmp = fopen("output/tmp_cfg_nointervalo.txt", "w");
    if (!tmp) { fail_count++; printf("FAIL: nao foi possivel criar ficheiro temp\n"); return; }
    fprintf(tmp, "MAX_ESPERA 60\n");
    fclose(tmp);

    Configuracao cfg;
    carregar_configuracao("output/tmp_cfg_nointervalo.txt", &cfg);
    CHECK(cfg.intervaloChegada == FLUXO_NORMAL,
          "intervaloChegada usa FLUXO_NORMAL quando ausente do ficheiro");
    remove("output/tmp_cfg_nointervalo.txt");
}

int main(void) {
    printf("=== Issue 9: Config INTERVALO_CHEGADA e constantes de fluxo ===\n\n");
    test_default_intervalo_chegada();
    test_carregar_intervalo_chegada();
    test_intervalo_chegada_fallback();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
