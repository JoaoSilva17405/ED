#include "hash.h"

void hash_init(HashClientes *hash) {
    int i;
    for (i = 0; i < HASH_SIZE; ++i) hash->tabela[i] = NULL;
}

unsigned int hash_func(const char *id) {
    unsigned int h = 5381;
    while (*id) {
        h = ((h << 5) + h) + (unsigned char)(*id);
        id++;
    }
    return h % HASH_SIZE;
}

bool hash_inserir(HashClientes *hash, Cliente *cliente, int idCaixa) {
    unsigned int pos = hash_func(cliente->id);
    NoHash *novo = (NoHash *)malloc(sizeof(NoHash));
    if (!novo) return false;
    strcpy(novo->id, cliente->id);
    novo->cliente = cliente;
    novo->idCaixa = idCaixa;
    novo->seguinte = hash->tabela[pos];
    hash->tabela[pos] = novo;
    return true;
}

NoHash *hash_pesquisar(HashClientes *hash, const char *id) {
    unsigned int pos = hash_func(id);
    NoHash *atual = hash->tabela[pos];
    while (atual) {
        if (strcmp(atual->id, id) == 0) return atual;
        atual = atual->seguinte;
    }
    return NULL;
}

void hash_atualizar_caixa(HashClientes *hash, const char *id, int idCaixa) {
    NoHash *no = hash_pesquisar(hash, id);
    if (no) no->idCaixa = idCaixa;
}

void hash_remover(HashClientes *hash, const char *id) {
    unsigned int pos = hash_func(id);
    NoHash *ant = NULL, *atual = hash->tabela[pos];
    while (atual) {
        if (strcmp(atual->id, id) == 0) {
            if (!ant) hash->tabela[pos] = atual->seguinte;
            else ant->seguinte = atual->seguinte;
            free(atual);
            return;
        }
        ant = atual;
        atual = atual->seguinte;
    }
}

void hash_listar(HashClientes *hash) {
    int i;
    for (i = 0; i < HASH_SIZE; ++i) {
        NoHash *atual = hash->tabela[i];
        if (!atual) continue;
        printf("Bucket %d:\n", i);
        while (atual) {
            printf("  %s -> caixa %d\n", atual->id, atual->idCaixa);
            atual = atual->seguinte;
        }
    }
}

size_t hash_memoria(const HashClientes *hash) {
    size_t total = sizeof(HashClientes);
    int i;
    for (i = 0; i < HASH_SIZE; ++i) {
        NoHash *atual = hash->tabela[i];
        while (atual) {
            total += sizeof(NoHash);
            atual = atual->seguinte;
        }
    }
    return total;
}

size_t hash_memoria_desperdicada(const HashClientes *hash) {
    size_t total = 0;
    int i;
    for (i = 0; i < HASH_SIZE; ++i) {
        if (hash->tabela[i] == NULL) total += sizeof(NoHash *);
    }
    return total;
}

void hash_destruir(HashClientes *hash) {
    int i;
    for (i = 0; i < HASH_SIZE; ++i) {
        NoHash *atual = hash->tabela[i];
        while (atual) {
            NoHash *prox = atual->seguinte;
            free(atual);
            atual = prox;
        }
        hash->tabela[i] = NULL;
    }
}
