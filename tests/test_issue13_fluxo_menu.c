#include <stdio.h>
#include <string.h>
#include "simulation.h"
#include "config.h"
#include "store.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

/* Ciclo 1: alterar para CALMO */
static void test_alterar_fluxo_calmo(void) {
    Configuracao cfg;
    config_default(&cfg);

    int evt = sim_alterar_fluxo(&cfg, FLUXO_CALMO);

    CHECK(cfg.intervaloChegada == FLUXO_CALMO,
          "intervaloChegada actualizado para FLUXO_CALMO");
    CHECK(evt == EVT_FLUXO_ALTERADO,
          "sim_alterar_fluxo retorna EVT_FLUXO_ALTERADO");
}

/* Ciclo 2: alterar para NORMAL */
static void test_alterar_fluxo_normal(void) {
    Configuracao cfg;
    config_default(&cfg);
    cfg.intervaloChegada = FLUXO_CALMO; /* começa num valor diferente */

    int evt = sim_alterar_fluxo(&cfg, FLUXO_NORMAL);

    CHECK(cfg.intervaloChegada == FLUXO_NORMAL,
          "intervaloChegada actualizado para FLUXO_NORMAL");
    CHECK(evt == EVT_FLUXO_ALTERADO,
          "retorna EVT_FLUXO_ALTERADO ao mudar para NORMAL");
}

/* Ciclo 3: alterar para HORA_DE_PONTA */
static void test_alterar_fluxo_hora_ponta(void) {
    Configuracao cfg;
    config_default(&cfg);

    int evt = sim_alterar_fluxo(&cfg, FLUXO_HORA_DE_PONTA);

    CHECK(cfg.intervaloChegada == FLUXO_HORA_DE_PONTA,
          "intervaloChegada actualizado para FLUXO_HORA_DE_PONTA");
    CHECK(evt == EVT_FLUXO_ALTERADO,
          "retorna EVT_FLUXO_ALTERADO ao mudar para HORA_DE_PONTA");
}

/* Ciclo 4: valor invalido (<= 0) nao altera e retorna EVT_NENHUM */
static void test_alterar_fluxo_invalido(void) {
    Configuracao cfg;
    config_default(&cfg);
    int original = cfg.intervaloChegada;

    int evt = sim_alterar_fluxo(&cfg, 0);

    CHECK(cfg.intervaloChegada == original,
          "intervaloChegada nao alterado para valor invalido (0)");
    CHECK(evt == EVT_NENHUM,
          "retorna EVT_NENHUM para intervalo invalido");
}

int main(void) {
    printf("=== Issue 13: sim_alterar_fluxo e menu de presets ===\n\n");
    test_alterar_fluxo_calmo();
    test_alterar_fluxo_normal();
    test_alterar_fluxo_hora_ponta();
    test_alterar_fluxo_invalido();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
