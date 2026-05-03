#ifndef CATALOG_H
#define CATALOG_H

#include "common.h"
#include "config.h"
#include "product.h"

typedef struct {
    Produto *lista;
    int tamanho;
    int capacidade;
} CatalogoProdutos;

CatalogoProdutos *catalog_carregar(const char *filename);
Produto          *catalog_obter_produtos_aleatorios(const CatalogoProdutos *cat, int n, const Configuracao *cfg);
int               catalog_pesquisar(const CatalogoProdutos *cat, const char *termo, Produto **resultados, int max);
void              catalog_destruir(CatalogoProdutos *cat);

#endif
