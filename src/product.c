#include "product.h"

static const char *nomesBase[] = {
    "Leite", "Pao", "Massa", "Arroz", "Iogurte", "Cafe", "Acucar", "Atum",
    "Bolachas", "Chocolate", "Agua", "Sumo", "Fruta", "Queijo", "Detergente"
};

Produto *gerar_produtos_aleatorios(int quantidade, const Configuracao *cfg) {
    int i;
    Produto *produtos = (Produto *)malloc(sizeof(Produto) * quantidade);
    if (!produtos) return NULL;

    for (i = 0; i < quantidade; ++i) {
        int idx = rand() % (int)(sizeof(nomesBase) / sizeof(nomesBase[0]));
        produtos[i].id = 0;
        snprintf(produtos[i].nome, sizeof(produtos[i].nome), "%s_%d", nomesBase[idx], i + 1);
        produtos[i].preco = 0.10f + ((float)(rand() % (int)(cfg->maxPreco * 100))) / 100.0f;
        if (produtos[i].preco > cfg->maxPreco) produtos[i].preco = cfg->maxPreco;
        produtos[i].tempoCompra   = 2 + rand() % ((cfg->tempoAtendimentoProduto >= 2) ? (cfg->tempoAtendimentoProduto - 1) : 1);
        produtos[i].tempoPassagem = 2 + rand() % ((cfg->tempoAtendimentoProduto >= 2) ? (cfg->tempoAtendimentoProduto - 1) : 1);
        produtos[i].oferecido = false;
    }
    return produtos;
}

int tempo_total_produtos(const Produto *produtos, int nProdutos) {
    int total = 0;
    int i;
    for (i = 0; i < nProdutos; ++i) total += produtos[i].tempoPassagem;
    return total;
}

int tempo_compra_total_produtos(const Produto *produtos, int nProdutos) {
    int total = 0, i;
    if (!produtos || nProdutos <= 0) return 0;
    for (i = 0; i < nProdutos; ++i) total += produtos[i].tempoCompra;
    return total;
}

float valor_total_produtos(const Produto *produtos, int nProdutos) {
    float total = 0.0f;
    int i;
    for (i = 0; i < nProdutos; ++i) total += produtos[i].preco;
    return total;
}

void libertar_produtos(Produto *produtos) {
    free(produtos);
}
