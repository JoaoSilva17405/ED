#include "employee.h"
#include "utils.h"

#define FUNC_GROWTH 16

ListaFuncionarios *funcionarios_carregar(const char *filename) {
    FILE *f = fopen(filename, "r");
    char linha[MAX_LINE];
    ListaFuncionarios *lista;

    if (!f) return NULL;
    lista = (ListaFuncionarios *)malloc(sizeof(ListaFuncionarios));
    if (!lista) { fclose(f); return NULL; }
    lista->lista = NULL;
    lista->tamanho = 0;
    lista->capacidade = 0;

    while (fgets(linha, sizeof(linha), f)) {
        int id;
        char nome[MAX_NOME];
        char *p = linha;

        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n' || *p == '\r') continue;

        if (sscanf(p, "%d %63[^\n\r]", &id, nome) == 2) {
            trim(nome);
            if (lista->tamanho == lista->capacidade) {
                int nova = lista->capacidade + FUNC_GROWTH;
                Funcionario *novo = (Funcionario *)realloc(lista->lista, sizeof(Funcionario) * nova);
                if (!novo) break;
                lista->lista = novo;
                lista->capacidade = nova;
            }
            lista->lista[lista->tamanho].id = id;
            strncpy(lista->lista[lista->tamanho].nome, nome, MAX_NOME - 1);
            lista->lista[lista->tamanho].nome[MAX_NOME - 1] = '\0';
            lista->tamanho++;
        }
    }

    fclose(f);
    if (lista->tamanho == 0) {
        free(lista->lista);
        free(lista);
        return NULL;
    }
    return lista;
}

Funcionario *funcionarios_obter_aleatorio(const ListaFuncionarios *lista) {
    if (!lista || lista->tamanho == 0) return NULL;
    return &lista->lista[rand() % lista->tamanho];
}

void funcionarios_destruir(ListaFuncionarios *lista) {
    if (!lista) return;
    free(lista->lista);
    free(lista);
}
