#include "product.h"

float tempo_total_produtos(const Produto *produtos, int nProdutos) {
    float total = 0.0f;
    int i;
    for (i = 0; i < nProdutos; ++i) total += produtos[i].tempoPassagem;
    return total;
}

float tempo_compra_total_produtos(const Produto *produtos, int nProdutos) {
    float total = 0.0f;
    int i;
    if (!produtos || nProdutos <= 0) return 0.0f;
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
