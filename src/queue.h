#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include "customer.h"

typedef struct NoFila {
    Cliente *cliente;
    struct NoFila *seguinte;
} NoFila;

typedef struct {
    NoFila *cabeca;
    NoFila *cauda;
    int tamanho;
} FilaClientes;

void fila_init(FilaClientes *fila);
bool fila_vazia(const FilaClientes *fila);
bool fila_inserir(FilaClientes *fila, Cliente *cliente);
Cliente *fila_remover(FilaClientes *fila);
Cliente *fila_remover_por_id(FilaClientes *fila, const char *id);
int fila_posicao_cliente(const FilaClientes *fila, const char *id);
void fila_listar(const FilaClientes *fila);
size_t fila_memoria_nodos(const FilaClientes *fila);
void fila_destruir(FilaClientes *fila, bool destruirClientes);

#endif
