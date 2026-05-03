#include "register.h"

void caixa_init(Caixa *caixa, int id, const char *operador, EstadoCaixa estado) {
    caixa->id = id;
    strncpy(caixa->operador, operador, sizeof(caixa->operador) - 1);
    caixa->operador[sizeof(caixa->operador) - 1] = '\0';
    caixa->estado = estado;
    fila_init(&caixa->fila);
    caixa->emAtendimento = NULL;
    caixa->tempoRestanteAtendimento = 0;
    caixa->totalClientesAtendidos = 0;
    caixa->totalProdutosVendidos = 0;
    caixa->totalValorVendido = 0.0f;
    caixa->historicoAtendidos = NULL;
    caixa->nHistorico = 0;
    caixa->capHistorico = 0;
}

void caixa_abrir(Caixa *caixa) {
    if (caixa->estado == CAIXA_FECHADA) caixa->estado = CAIXA_ABERTA;
}

void caixa_fechar_suave(Caixa *caixa) {
    if (caixa->estado == CAIXA_ABERTA) caixa->estado = CAIXA_A_FECHAR;
}

void caixa_fechar_imediata(Caixa *caixa) {
    caixa->estado = CAIXA_FECHADA;
}

int caixa_total_pessoas(const Caixa *caixa) {
    return caixa->fila.tamanho + (caixa->emAtendimento ? 1 : 0);
}

void caixa_adicionar_historico(Caixa *caixa, Cliente *cliente) {
    if (caixa->nHistorico == caixa->capHistorico) {
        int novaCap = caixa->capHistorico + HIST_GROWTH;
        Cliente **novo = (Cliente **)realloc(caixa->historicoAtendidos, sizeof(Cliente *) * novaCap);
        if (!novo) return;
        caixa->historicoAtendidos = novo;
        caixa->capHistorico = novaCap;
    }
    caixa->historicoAtendidos[caixa->nHistorico++] = cliente;
}

void caixa_listar_historico(const Caixa *caixa) {
    int i;
    printf("Historico da caixa %d (%s):\n", caixa->id + 1, caixa->operador);
    if (caixa->nHistorico == 0) {
        printf("  (sem clientes atendidos)\n");
        return;
    }
    for (i = 0; i < caixa->nHistorico; ++i) {
        printf("  [%d] ", i + 1);
        mostrar_cliente(caixa->historicoAtendidos[i]);
    }
}

size_t caixa_memoria(const Caixa *caixa) {
    size_t total = sizeof(Caixa);
    total += fila_memoria_nodos(&caixa->fila);
    total += sizeof(Cliente *) * caixa->capHistorico;
    return total;
}

size_t caixa_memoria_desperdicada(const Caixa *caixa) {
    size_t total = 0;
    total += (size_t)(caixa->capHistorico - caixa->nHistorico) * sizeof(Cliente *);
    if (caixa->estado == CAIXA_FECHADA && caixa->fila.tamanho == 0 && caixa->emAtendimento == NULL) {
        total += sizeof(FilaClientes);
    }
    return total;
}

void caixa_destruir(Caixa *caixa, bool destruirClientesRestantes) {
    int i;
    if (caixa->emAtendimento && destruirClientesRestantes) destruir_cliente(caixa->emAtendimento);
    fila_destruir(&caixa->fila, destruirClientesRestantes);
    if (destruirClientesRestantes) {
        for (i = 0; i < caixa->nHistorico; ++i) destruir_cliente(caixa->historicoAtendidos[i]);
    }
    free(caixa->historicoAtendidos);
    caixa->historicoAtendidos = NULL;
    caixa->nHistorico = 0;
    caixa->capHistorico = 0;
    caixa->emAtendimento = NULL;
}
