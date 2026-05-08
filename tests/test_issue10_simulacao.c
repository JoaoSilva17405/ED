#include <stdio.h>
#include <string.h>
#include "simulation.h"
#include "config.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

static void test_nome_fluxo_calmo(void) {
    CHECK(strcmp(sim_nome_fluxo(FLUXO_CALMO), "CALMO") == 0,
          "sim_nome_fluxo(FLUXO_CALMO) retorna \"CALMO\"");
}

static void test_nome_fluxo_normal(void) {
    CHECK(strcmp(sim_nome_fluxo(FLUXO_NORMAL), "NORMAL") == 0,
          "sim_nome_fluxo(FLUXO_NORMAL) retorna \"NORMAL\"");
}

static void test_nome_fluxo_hora_de_ponta(void) {
    CHECK(strcmp(sim_nome_fluxo(FLUXO_HORA_DE_PONTA), "HORA DE PONTA") == 0,
          "sim_nome_fluxo(FLUXO_HORA_DE_PONTA) retorna \"HORA DE PONTA\"");
}

static void test_nome_fluxo_custom(void) {
    CHECK(strcmp(sim_nome_fluxo(99), "CUSTOM") == 0,
          "sim_nome_fluxo(intervalo desconhecido) retorna \"CUSTOM\"");
}

static void test_estados_enum(void) {
    EstadoSimulacao e = SIM_PAUSADA;
    CHECK(e == SIM_PAUSADA, "SIM_PAUSADA existe no enum EstadoSimulacao");
    e = SIM_A_CORRER;
    CHECK(e == SIM_A_CORRER, "SIM_A_CORRER existe no enum EstadoSimulacao");
    CHECK(SIM_PAUSADA != SIM_A_CORRER, "SIM_PAUSADA e SIM_A_CORRER sao valores distintos");
}

int main(void) {
    printf("=== Issue 10: Modulo simulation — nome_fluxo e EstadoSimulacao ===\n\n");
    test_nome_fluxo_calmo();
    test_nome_fluxo_normal();
    test_nome_fluxo_hora_de_ponta();
    test_nome_fluxo_custom();
    test_estados_enum();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
