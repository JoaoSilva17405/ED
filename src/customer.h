#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "common.h"
#include "config.h"
#include "product.h"

typedef struct Cliente {
    char id[MAX_ID];
    int nProdutos;
    Produto *produtos;
    int instanteEntradaFila;
    int instanteInicioAtendimento;
    int tempoEsperaTotal;
    int caixaAtual;
    int produtosOferecidos;
    float valorOferecido;
    bool estavaEmAtendimento;
    bool atendido;
    bool oferecimentoFeito;
} Cliente;

Cliente *criar_cliente(const char *id, int nProdutos, int instanteAtual, int caixaAtual, const Configuracao *cfg);
void destruir_cliente(Cliente *cliente);
int cliente_tempo_atendimento(const Cliente *cliente);
float cliente_valor_total(const Cliente *cliente);
float oferecer_um_produto(Cliente *cliente);
void mostrar_cliente(const Cliente *cliente);

#endif
