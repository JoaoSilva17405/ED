#include "customer.h"

Cliente *criar_cliente(const char *id, const char *nome, int nProdutos, int instanteAtual, int caixaAtual, Produto *produtos) {
    Cliente *cliente = (Cliente *)malloc(sizeof(Cliente));
    if (!cliente) return NULL;

    strncpy(cliente->id, id, sizeof(cliente->id) - 1);
    cliente->id[sizeof(cliente->id) - 1] = '\0';
    strncpy(cliente->nome, nome ? nome : "", sizeof(cliente->nome) - 1);
    cliente->nome[sizeof(cliente->nome) - 1] = '\0';
    cliente->nProdutos = nProdutos;
    cliente->produtos = produtos;
    if (!cliente->produtos) {
        free(cliente);
        return NULL;
    }
    cliente->instanteEntradaLoja = instanteAtual;
    cliente->tempoCompraTotal = 0;
    cliente->comprasTerminadas = false;
    cliente->instanteEntradaFila = instanteAtual;
    cliente->instanteInicioAtendimento = -1;
    cliente->tempoEsperaTotal = 0;
    cliente->caixaAtual = caixaAtual;
    cliente->produtosOferecidos = 0;
    cliente->valorOferecido = 0.0f;
    cliente->estavaEmAtendimento = false;
    cliente->atendido = false;
    cliente->oferecimentoFeito = false;
    return cliente;
}

void destruir_cliente(Cliente *cliente) {
    if (!cliente) return;
    libertar_produtos(cliente->produtos);
    free(cliente);
}

int cliente_tempo_atendimento(const Cliente *cliente) {
    return tempo_total_produtos(cliente->produtos, cliente->nProdutos);
}

float cliente_valor_total(const Cliente *cliente) {
    return valor_total_produtos(cliente->produtos, cliente->nProdutos);
}

float oferecer_um_produto(Cliente *cliente) {
    int i;
    if (!cliente) return 0.0f;
    for (i = 0; i < cliente->nProdutos; ++i) {
        if (!cliente->produtos[i].oferecido) {
            cliente->produtos[i].oferecido = true;
            cliente->produtosOferecidos++;
            cliente->valorOferecido += cliente->produtos[i].preco;
            return cliente->produtos[i].preco;
        }
    }
    return 0.0f;
}

void mostrar_cliente(const Cliente *cliente) {
    if (!cliente) return;
    if (cliente->caixaAtual >= 0) {
        if (cliente->nome[0])
            printf("Cliente %s (%s) | produtos=%d | caixa=%d | espera=%d | oferecidos=%d | valor_oferecido=%.2f\n",
                   cliente->id, cliente->nome,
                   cliente->nProdutos, cliente->caixaAtual + 1,
                   cliente->tempoEsperaTotal, cliente->produtosOferecidos, cliente->valorOferecido);
        else
            printf("Cliente %s | produtos=%d | caixa=%d | espera=%d | oferecidos=%d | valor_oferecido=%.2f\n",
                   cliente->id,
                   cliente->nProdutos, cliente->caixaAtual + 1,
                   cliente->tempoEsperaTotal, cliente->produtosOferecidos, cliente->valorOferecido);
    } else {
        if (cliente->nome[0])
            printf("Cliente %s (%s) | produtos=%d | (em compras) | espera=%d | oferecidos=%d | valor_oferecido=%.2f\n",
                   cliente->id, cliente->nome,
                   cliente->nProdutos,
                   cliente->tempoEsperaTotal, cliente->produtosOferecidos, cliente->valorOferecido);
        else
            printf("Cliente %s | produtos=%d | (em compras) | espera=%d | oferecidos=%d | valor_oferecido=%.2f\n",
                   cliente->id,
                   cliente->nProdutos,
                   cliente->tempoEsperaTotal, cliente->produtosOferecidos, cliente->valorOferecido);
    }
}
