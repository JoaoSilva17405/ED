#ifndef CLIENT_REGISTRY_H
#define CLIENT_REGISTRY_H

#include "common.h"

typedef struct {
    char id[MAX_ID];
    char nome[MAX_NOME];
} EntradaCliente;

typedef struct {
    EntradaCliente *lista;
    int tamanho;
    int capacidade;
} RegistoClientes;

RegistoClientes *registo_carregar(const char *filename);
EntradaCliente  *registo_obter_aleatorio(const RegistoClientes *registo);
EntradaCliente  *registo_pesquisar_id(const RegistoClientes *registo, const char *id);
int              registo_adicionar(RegistoClientes *registo, const char *id, const char *nome, const char *filename);
void             registo_destruir(RegistoClientes *registo);

#endif
