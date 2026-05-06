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
        float preco, tempoCompraF, tempo;
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
        if (sscanf(p3 + 1, "%f", &tempoCompraF) != 1) continue;

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
        cat->lista[cat->tamanho].tempoCompra   = tempo_para_int(tempoCompraF);
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
        if (cfg && produtos[i].preco > cfg->maxPreco)
            produtos[i].preco = cfg->maxPreco;
        if (produtos[i].preco <= 0.0f)
            produtos[i].preco = 0.01f;
    }
    return produtos;
}

static int str_icontains(const char *haystack, const char *needle) {
    size_t hlen = strlen(haystack), nlen = strlen(needle);
    size_t i, j;
    if (nlen == 0) return 1;
    if (nlen > hlen) return 0;
    for (i = 0; i <= hlen - nlen; i++) {
        int match = 1;
        for (j = 0; j < nlen; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

int catalog_pesquisar(const CatalogoProdutos *cat, const char *termo, Produto **resultados, int max) {
    int i, n = 0;
    int por_id;
    int id_alvo = 0;

    if (!cat || !termo || termo[0] == '\0' || !resultados || max <= 0) return 0;

    por_id = isdigit((unsigned char)termo[0]);
    if (por_id) id_alvo = atoi(termo);

    for (i = 0; i < cat->tamanho && n < max; i++) {
        if (por_id) {
            if (cat->lista[i].id == id_alvo) {
                resultados[n++] = &cat->lista[i];
            }
        } else {
            if (str_icontains(cat->lista[i].nome, termo)) {
                resultados[n++] = &cat->lista[i];
            }
        }
    }
    return n;
}

void catalog_destruir(CatalogoProdutos *cat) {
    if (!cat) return;
    free(cat->lista);
    free(cat);
}
