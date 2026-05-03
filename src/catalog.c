#include "catalog.h"
#include "utils.h"

#define CAT_GROWTH 64

/* ceil manual sem depender de math.h; depois aplica max(2, resultado) */
static int tempo_para_int(float f) {
    int i = (int)f;
    if ((float)i < f) i++;
    return i < 2 ? 2 : i;
}

CatalogoProdutos *catalog_carregar(const char *filename) {
    FILE *f = fopen(filename, "r");
    char linha[MAX_LINE];
    CatalogoProdutos *cat;

    if (!f) return NULL;
    cat = (CatalogoProdutos *)malloc(sizeof(CatalogoProdutos));
    if (!cat) { fclose(f); return NULL; }
    cat->lista = NULL;
    cat->tamanho = 0;
    cat->capacidade = 0;

    while (fgets(linha, sizeof(linha), f)) {
        char *p1, *p2, *p3, *p4;
        int id;
        char nome[MAX_NOME];
        float preco, stock, tempo;
        Produto *novo;

        trim(linha);
        if (linha[0] == '\0') continue;

        p1 = strchr(linha, '\t');
        if (!p1) continue;
        *p1 = '\0';
        id = atoi(linha);

        p2 = strchr(p1 + 1, '\t');
        if (!p2) continue;
        *p2 = '\0';
        strncpy(nome, p1 + 1, MAX_NOME - 1);
        nome[MAX_NOME - 1] = '\0';
        trim(nome);

        p3 = strchr(p2 + 1, '\t');
        if (!p3) continue;
        *p3 = '\0';
        if (sscanf(p2 + 1, "%f", &preco) != 1) continue;

        p4 = strchr(p3 + 1, '\t');
        if (!p4) continue;
        *p4 = '\0';
        if (sscanf(p3 + 1, "%f", &stock) != 1) continue;

        if (sscanf(p4 + 1, "%f", &tempo) != 1) continue;

        if (cat->tamanho == cat->capacidade) {
            int nova = cat->capacidade + CAT_GROWTH;
            novo = (Produto *)realloc(cat->lista, sizeof(Produto) * nova);
            if (!novo) break;
            cat->lista = novo;
            cat->capacidade = nova;
        }

        cat->lista[cat->tamanho].id            = id;
        strncpy(cat->lista[cat->tamanho].nome, nome, MAX_NOME - 1);
        cat->lista[cat->tamanho].nome[MAX_NOME - 1] = '\0';
        cat->lista[cat->tamanho].preco         = preco;
        cat->lista[cat->tamanho].stock         = stock;
        cat->lista[cat->tamanho].tempoPassagem = tempo_para_int(tempo);
        cat->lista[cat->tamanho].oferecido     = false;
        cat->tamanho++;
    }

    fclose(f);
    if (cat->tamanho == 0) {
        free(cat->lista);
        free(cat);
        return NULL;
    }
    return cat;
}

Produto *catalog_obter_produtos_aleatorios(const CatalogoProdutos *cat, int n, const Configuracao *cfg) {
    Produto *produtos;
    int i;

    if (!cat || n <= 0) return NULL;
    produtos = (Produto *)malloc(sizeof(Produto) * n);
    if (!produtos) return NULL;

    for (i = 0; i < n; i++) {
        produtos[i] = cat->lista[rand() % cat->tamanho];
        produtos[i].oferecido = false;
        if (produtos[i].tempoPassagem < 2) produtos[i].tempoPassagem = 2;
        if (cfg && produtos[i].tempoPassagem > cfg->tempoAtendimentoProduto)
            produtos[i].tempoPassagem = cfg->tempoAtendimentoProduto;
    }
    return produtos;
}

void catalog_destruir(CatalogoProdutos *cat) {
    if (!cat) return;
    free(cat->lista);
    free(cat);
}
