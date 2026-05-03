#include "queue.h"

void fila_init(FilaClientes *fila) {
    fila->cabeca = NULL;
    fila->cauda = NULL;
    fila->tamanho = 0;
}

bool fila_vazia(const FilaClientes *fila) {
    return fila->cabeca == NULL;
}

bool fila_inserir(FilaClientes *fila, Cliente *cliente) {
    NoFila *novo = (NoFila *)malloc(sizeof(NoFila));
    if (!novo) return false;
    novo->cliente = cliente;
    novo->seguinte = NULL;
    if (fila->cabeca == NULL) fila->cabeca = fila->cauda = novo;
    else {
        fila->cauda->seguinte = novo;
        fila->cauda = novo;
    }
    fila->tamanho++;
    return true;
}

Cliente *fila_remover(FilaClientes *fila) {
    NoFila *rem;
    Cliente *cliente;
    if (fila->cabeca == NULL) return NULL;
    rem = fila->cabeca;
    fila->cabeca = rem->seguinte;
    if (fila->cabeca == NULL) fila->cauda = NULL;
    cliente = rem->cliente;
    free(rem);
    fila->tamanho--;
    return cliente;
}

Cliente *fila_remover_por_id(FilaClientes *fila, const char *id) {
    NoFila *ant = NULL, *atual = fila->cabeca;
    while (atual) {
        if (strcmp(atual->cliente->id, id) == 0) {
            Cliente *cliente = atual->cliente;
            if (ant == NULL) fila->cabeca = atual->seguinte;
            else ant->seguinte = atual->seguinte;
            if (fila->cauda == atual) fila->cauda = ant;
            free(atual);
            fila->tamanho--;
            return cliente;
        }
        ant = atual;
        atual = atual->seguinte;
    }
    return NULL;
}

int fila_posicao_cliente(const FilaClientes *fila, const char *id) {
    NoFila *atual = fila->cabeca;
    int pos = 1;
    while (atual) {
        if (strcmp(atual->cliente->id, id) == 0) return pos;
        atual = atual->seguinte;
        pos++;
    }
    return -1;
}

void fila_listar(const FilaClientes *fila) {
    NoFila *atual = fila->cabeca;
    int i = 1;
    while (atual) {
        printf("  [%d] ", i++);
        mostrar_cliente(atual->cliente);
        atual = atual->seguinte;
    }
    if (fila->cabeca == NULL) printf("  (fila vazia)\n");
}

size_t fila_memoria_nodos(const FilaClientes *fila) {
    size_t total = sizeof(FilaClientes);
    NoFila *atual = fila->cabeca;
    while (atual) {
        total += sizeof(NoFila);
        atual = atual->seguinte;
    }
    return total;
}

void fila_destruir(FilaClientes *fila, bool destruirClientes) {
    NoFila *atual = fila->cabeca;
    while (atual) {
        NoFila *prox = atual->seguinte;
        if (destruirClientes) destruir_cliente(atual->cliente);
        free(atual);
        atual = prox;
    }
    fila_init(fila);
}
