#ifndef PRODUCT_H
#define PRODUCT_H

#include "common.h"
#include "config.h"

typedef struct {
    char nome[MAX_NOME];
    float preco;
    int tempoPassagem;
    bool oferecido;
} Produto;

Produto *gerar_produtos_aleatorios(int quantidade, const Configuracao *cfg);
int tempo_total_produtos(const Produto *produtos, int nProdutos);
float valor_total_produtos(const Produto *produtos, int nProdutos);
void libertar_produtos(Produto *produtos);

#endif
