#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

typedef struct {
    int maxEspera;
    int nCaixas;
    int tempoAtendimentoProduto;
    float maxPreco;
    int maxFila;
    int minFila;
    int minProdutos;
    int maxProdutos;
} Configuracao;

void config_default(Configuracao *cfg);
int carregar_configuracao(const char *filename, Configuracao *cfg);
void mostrar_configuracao(const Configuracao *cfg);

#endif
