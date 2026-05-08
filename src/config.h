#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

#define FLUXO_CALMO         15
#define FLUXO_NORMAL         8
#define FLUXO_HORA_DE_PONTA  3

typedef struct {
    int maxEspera;
    int nCaixas;
    float maxPreco;
    int maxFila;
    int minFila;
    int minProdutos;
    int maxProdutos;
    int intervaloChegada;
} Configuracao;

void config_default(Configuracao *cfg);
int carregar_configuracao(const char *filename, Configuracao *cfg);
void mostrar_configuracao(const Configuracao *cfg);

#endif
