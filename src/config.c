#include "config.h"
#include "utils.h"

void config_default(Configuracao *cfg) {
    cfg->maxEspera = 120;
    cfg->nCaixas = 6;
    cfg->tempoAtendimentoProduto = 6;
    cfg->maxPreco = 40.0f;
    cfg->maxFila = 7;
    cfg->minFila = 3;
    cfg->minProdutos = 1;
    cfg->maxProdutos = 20;
}

int carregar_configuracao(const char *filename, Configuracao *cfg) {
    FILE *f = fopen(filename, "r");
    char linha[MAX_LINE];
    char chave[128];
    float valorFloat;

    if (!f) return 0;

    config_default(cfg);

    while (fgets(linha, sizeof(linha), f)) {
        trim(linha);
        if (linha[0] == '\0') continue;
        if (sscanf(linha, "%127s %f", chave, &valorFloat) != 2) continue;

        if (strcmp(chave, "MAX_ESPERA") == 0) cfg->maxEspera = (int)valorFloat;
        else if (strcmp(chave, "N_CAIXAS") == 0) cfg->nCaixas = (int)valorFloat;
        else if (strcmp(chave, "TEMPO_ATENDIMENTO_PRODUTO") == 0) cfg->tempoAtendimentoProduto = (int)valorFloat;
        else if (strcmp(chave, "MAX_PRECO") == 0) cfg->maxPreco = valorFloat;
        else if (strcmp(chave, "MAX_FILA") == 0) cfg->maxFila = (int)valorFloat;
        else if (strcmp(chave, "MIN_FILA") == 0) cfg->minFila = (int)valorFloat;
        else if (strcmp(chave, "MIN_PRODUTOS") == 0) cfg->minProdutos = (int)valorFloat;
        else if (strcmp(chave, "MAX_PRODUTOS") == 0) cfg->maxProdutos = (int)valorFloat;
    }

    fclose(f);
    return 1;
}

void mostrar_configuracao(const Configuracao *cfg) {
    printf("\n=== Configuracao ===\n");
    printf("MAX_ESPERA: %d\n", cfg->maxEspera);
    printf("N_CAIXAS: %d\n", cfg->nCaixas);
    printf("TEMPO_ATENDIMENTO_PRODUTO: %d\n", cfg->tempoAtendimentoProduto);
    printf("MAX_PRECO: %.2f\n", cfg->maxPreco);
    printf("MAX_FILA: %d\n", cfg->maxFila);
    printf("MIN_FILA: %d\n", cfg->minFila);
    printf("MIN_PRODUTOS: %d\n", cfg->minProdutos);
    printf("MAX_PRODUTOS: %d\n", cfg->maxProdutos);
}
