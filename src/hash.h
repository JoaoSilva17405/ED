#ifndef HASH_H
#define HASH_H

#include "common.h"
#include "customer.h"

typedef struct NoHash {
    char id[MAX_ID];
    Cliente *cliente;
    int idCaixa;
    struct NoHash *seguinte;
} NoHash;

typedef struct {
    NoHash *tabela[HASH_SIZE];
} HashClientes;

void hash_init(HashClientes *hash);
unsigned int hash_func(const char *id);
bool hash_inserir(HashClientes *hash, Cliente *cliente, int idCaixa);
NoHash *hash_pesquisar(HashClientes *hash, const char *id);
void hash_atualizar_caixa(HashClientes *hash, const char *id, int idCaixa);
void hash_remover(HashClientes *hash, const char *id);
void hash_listar(HashClientes *hash);
size_t hash_memoria(const HashClientes *hash);
size_t hash_memoria_desperdicada(const HashClientes *hash);
void hash_destruir(HashClientes *hash);

#endif
