#ifndef REGISTER_H
#define REGISTER_H

#include "common.h"
#include "queue.h"

typedef struct {
    int id;
    char operador[MAX_OPERADOR];
    EstadoCaixa estado;
    FilaClientes fila;
    Cliente *emAtendimento;
    int tempoRestanteAtendimento;
    int totalClientesAtendidos;
    int totalProdutosVendidos;
    float totalValorVendido;
    Cliente **historicoAtendidos;
    int nHistorico;
    int capHistorico;
} Caixa;

void caixa_init(Caixa *caixa, int id, const char *operador, EstadoCaixa estado);
void caixa_abrir(Caixa *caixa);
void caixa_fechar_suave(Caixa *caixa);
void caixa_fechar_imediata(Caixa *caixa);
int caixa_total_pessoas(const Caixa *caixa);
void caixa_adicionar_historico(Caixa *caixa, Cliente *cliente);
void caixa_listar_historico(const Caixa *caixa);
size_t caixa_memoria(const Caixa *caixa);
size_t caixa_memoria_desperdicada(const Caixa *caixa);
void caixa_destruir(Caixa *caixa, bool destruirClientesRestantes);

#endif
