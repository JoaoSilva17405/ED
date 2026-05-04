#include "client_registry.h"
#include "utils.h"

#define REG_GROWTH 64

RegistoClientes *registo_carregar(const char *filename) {
    FILE *f = fopen(filename, "r");
    char linha[MAX_LINE];
    RegistoClientes *reg;

    if (!f) return NULL;
    reg = (RegistoClientes *)malloc(sizeof(RegistoClientes));
    if (!reg) { fclose(f); return NULL; }
    reg->lista = NULL;
    reg->tamanho = 0;
    reg->capacidade = 0;

    while (fgets(linha, sizeof(linha), f)) {
        char id[MAX_ID];
        char nome[MAX_NOME];
        char *tab;
        EntradaCliente *novo;

        trim(linha);
        if (linha[0] == '\0') continue;

        tab = strchr(linha, '\t');
        if (!tab) continue;

        *tab = '\0';
        strncpy(id, linha, MAX_ID - 1);
        id[MAX_ID - 1] = '\0';
        trim(id);

        strncpy(nome, tab + 1, MAX_NOME - 1);
        nome[MAX_NOME - 1] = '\0';
        trim(nome);

        if (id[0] == '\0' || nome[0] == '\0') continue;

        if (reg->tamanho == reg->capacidade) {
            int nova = reg->capacidade + REG_GROWTH;
            novo = (EntradaCliente *)realloc(reg->lista, sizeof(EntradaCliente) * nova);
            if (!novo) break;
            reg->lista = novo;
            reg->capacidade = nova;
        }

        strncpy(reg->lista[reg->tamanho].id,   id,   MAX_ID   - 1);
        reg->lista[reg->tamanho].id[MAX_ID - 1] = '\0';
        strncpy(reg->lista[reg->tamanho].nome, nome, MAX_NOME - 1);
        reg->lista[reg->tamanho].nome[MAX_NOME - 1] = '\0';
        reg->tamanho++;
    }

    fclose(f);
    if (reg->tamanho == 0) {
        free(reg->lista);
        free(reg);
        return NULL;
    }
    return reg;
}

EntradaCliente *registo_pesquisar_id(const RegistoClientes *registo, const char *id) {
    int i;
    if (!registo || !id) return NULL;
    for (i = 0; i < registo->tamanho; i++) {
        if (strcmp(registo->lista[i].id, id) == 0) return &registo->lista[i];
    }
    return NULL;
}

EntradaCliente *registo_obter_aleatorio(const RegistoClientes *registo) {
    if (!registo || registo->tamanho == 0) return NULL;
    return &registo->lista[rand() % registo->tamanho];
}

int registo_adicionar(RegistoClientes *registo, const char *id, const char *nome, const char *filename) {
    EntradaCliente *novo;
    FILE *f;
    int i;

    if (!registo || !id || !nome) return 0;
    if (strlen(id) != 6) return 0;
    for (i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)id[i])) return 0;
    }
    if (nome[0] == '\0') return 0;

    if (registo_pesquisar_id(registo, id)) return 1;

    if (registo->tamanho == registo->capacidade) {
        int nova = registo->capacidade + REG_GROWTH;
        novo = (EntradaCliente *)realloc(registo->lista, sizeof(EntradaCliente) * nova);
        if (!novo) return 0;
        registo->lista = novo;
        registo->capacidade = nova;
    }

    strncpy(registo->lista[registo->tamanho].id,   id,   MAX_ID   - 1);
    registo->lista[registo->tamanho].id[MAX_ID - 1] = '\0';
    strncpy(registo->lista[registo->tamanho].nome, nome, MAX_NOME - 1);
    registo->lista[registo->tamanho].nome[MAX_NOME - 1] = '\0';
    registo->tamanho++;

    if (!filename) return 1;
    f = fopen(filename, "a");
    if (!f) return 0;
    fprintf(f, "%s\t%s\n", id, nome);
    fclose(f);
    return 1;
}

void registo_destruir(RegistoClientes *registo) {
    if (!registo) return;
    free(registo->lista);
    free(registo);
}
