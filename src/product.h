#ifndef PRODUCT_H
#define PRODUCT_H

#include "common.h"

typedef struct {
    int id;
    char nome[MAX_NOME];
    float preco;
    float tempoCompra;
    float tempoPassagem;
    bool oferecido;
} Produto;

float tempo_total_produtos(const Produto *produtos, int nProdutos);
float tempo_compra_total_produtos(const Produto *produtos, int nProdutos);
float valor_total_produtos(const Produto *produtos, int nProdutos);
void libertar_produtos(Produto *produtos);

#endif
