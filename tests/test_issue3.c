#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client_registry.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s\n", desc); fail_count++; } \
} while(0)

static void test_carregar_nao_null(void) {
    RegistoClientes *reg = registo_carregar("Clientes.txt");
    CHECK(reg != NULL, "registo_carregar(Clientes.txt) retorna nao-NULL");
    if (reg) registo_destruir(reg);
}

static void test_contagem_entradas(void) {
    RegistoClientes *reg = registo_carregar("Clientes.txt");
    CHECK(reg != NULL && reg->tamanho == 10000, "10000 entradas carregadas de Clientes.txt");
    if (reg) registo_destruir(reg);
}

static void test_zeros_iniciais_preservados(void) {
    FILE *tmp = fopen("output/tmp_reg_test.txt", "w");
    if (tmp) {
        fprintf(tmp, "043001\tAlberto Zero Pinto\n");
        fprintf(tmp, "100200\tBeatriz Normal Silva\n");
        fclose(tmp);
    }
    RegistoClientes *reg = registo_carregar("output/tmp_reg_test.txt");
    CHECK(reg != NULL && reg->tamanho == 2, "2 entradas no ficheiro temporario");
    if (reg) {
        CHECK(strcmp(reg->lista[0].id, "043001") == 0,
              "ID com zeros iniciais preservado como string '043001'");
        CHECK(strcmp(reg->lista[0].nome, "Alberto Zero Pinto") == 0,
              "Nome completo preservado");
        registo_destruir(reg);
    }
    remove("output/tmp_reg_test.txt");
}

static void test_obter_aleatorio(void) {
    RegistoClientes *reg = registo_carregar("Clientes.txt");
    if (!reg) { fail_count++; printf("FAIL: registo nao carregado para teste aleatorio\n"); return; }
    EntradaCliente *e = registo_obter_aleatorio(reg);
    CHECK(e != NULL,               "registo_obter_aleatorio retorna nao-NULL");
    CHECK(e && strlen(e->id) > 0,  "entrada aleatoria tem ID nao vazio");
    CHECK(e && strlen(e->nome) > 0,"entrada aleatoria tem nome nao vazio");
    registo_destruir(reg);
}

static void test_ficheiro_inexistente(void) {
    RegistoClientes *reg = registo_carregar("nao_existe.txt");
    CHECK(reg == NULL, "ficheiro inexistente retorna NULL");
}

int main(void) {
    printf("=== Issue 3: client_registry ===\n\n");
    test_carregar_nao_null();
    test_contagem_entradas();
    test_zeros_iniciais_preservados();
    test_obter_aleatorio();
    test_ficheiro_inexistente();
    printf("\n%d/%d testes passaram\n", pass_count, pass_count + fail_count);
    return fail_count > 0 ? 1 : 0;
}
