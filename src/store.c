#include "store.h"
#include "utils.h"

static int contar_caixas_abertas(const Supermercado *sm) {
    int i, total = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        if (sm->caixas[i].estado == CAIXA_ABERTA || sm->caixas[i].estado == CAIXA_A_FECHAR) total++;
    }
    return total;
}

static int contar_caixas_realmente_abertas(const Supermercado *sm) {
    int i, total = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        if (sm->caixas[i].estado == CAIXA_ABERTA) total++;
    }
    return total;
}

static int total_clientes_em_filas(const Supermercado *sm) {
    int i, total = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) total += sm->caixas[i].fila.tamanho;
    return total;
}

static int encontrar_caixa_menos_pessoas_aberta(const Supermercado *sm, int ignorar) {
    int i, idx = -1, min = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        const Caixa *c = &sm->caixas[i];
        int total;
        if (i == ignorar) continue;
        if (c->estado != CAIXA_ABERTA) continue;
        total = caixa_total_pessoas(c);
        if (idx == -1 || total < min) {
            idx = i;
            min = total;
        }
    }
    return idx;
}

static int abrir_primeira_caixa_fechada(Supermercado *sm) {
    int i;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        if (sm->caixas[i].estado == CAIXA_FECHADA) {
            char detalhes[80];
            sm->caixas[i].estado = CAIXA_ABERTA;
            snprintf(detalhes, sizeof(detalhes), "Caixa %d aberta para receber novo cliente", i + 1);
            log_acao(sm->logFile, "ABRIR_CAIXA_CLIENTE", detalhes);
            return i;
        }
    }
    return -1;
}

static int encontrar_caixa_para_novo_cliente(Supermercado *sm) {
    int destino = encontrar_caixa_menos_pessoas_aberta(sm, -1);
    if (destino != -1) return destino;
    return abrir_primeira_caixa_fechada(sm);
}

static void iniciar_atendimento(Caixa *caixa, int instanteAtual) {
    if (caixa->emAtendimento == NULL && !fila_vazia(&caixa->fila)) {
        caixa->emAtendimento = fila_remover(&caixa->fila);
        if (!caixa->emAtendimento) return;
        caixa->emAtendimento->estavaEmAtendimento = true;
        caixa->emAtendimento->instanteInicioAtendimento = instanteAtual;
        caixa->tempoRestanteAtendimento = cliente_tempo_atendimento(caixa->emAtendimento);
    }
}

static void redistribuir_fila(Supermercado *sm, HashClientes *hash, int idCaixa) {
    Caixa *origem = &sm->caixas[idCaixa];
    while (!fila_vazia(&origem->fila)) {
        Cliente *cliente = fila_remover(&origem->fila);
        int destino = encontrar_caixa_menos_pessoas_aberta(sm, idCaixa);
        if (!cliente) break;
        if (destino == -1 || !fila_inserir(&sm->caixas[destino].fila, cliente)) {
            fila_inserir(&origem->fila, cliente);
            break;
        }
        cliente->caixaAtual = destino;
        hash_atualizar_caixa(hash, cliente->id, destino);
    }
}

int supermercado_init(Supermercado *sm, const Configuracao *cfg, FILE *logFile, ListaFuncionarios *funcionarios, CatalogoProdutos *catalogo) {
    int i;
    sm->cfg = *cfg;
    sm->caixas = (Caixa *)malloc(sizeof(Caixa) * cfg->nCaixas);
    if (!sm->caixas) return 0;
    sm->catalogo = catalogo;
    sm->instanteAtual = 0;
    sm->totalClientesAtendidos = 0;
    sm->totalProdutosVendidos = 0;
    sm->totalProdutosOferecidos = 0;
    sm->totalValorOferecido = 0.0f;
    sm->somaTemposEspera = 0;
    sm->logFile = logFile;
    for (i = 0; i < cfg->nCaixas; ++i) {
        char operador[MAX_OPERADOR];
        Funcionario *func = funcionarios ? funcionarios_obter_aleatorio(funcionarios) : NULL;
        if (func) {
            strncpy(operador, func->nome, sizeof(operador) - 1);
            operador[sizeof(operador) - 1] = '\0';
        } else {
            snprintf(operador, sizeof(operador), "Operador_%d", i + 1);
        }
        caixa_init(&sm->caixas[i], i, operador, CAIXA_FECHADA);
    }
    return 1;
}

void supermercado_destruir(Supermercado *sm, HashClientes *hash) {
    int i;
    if (!sm || !sm->caixas) return;
    for (i = 0; i < sm->cfg.nCaixas; ++i) caixa_destruir(&sm->caixas[i], true);
    free(sm->caixas);
    sm->caixas = NULL;
    if (hash) hash_destruir(hash);
}

int carregar_dados_iniciais(const char *filename, Supermercado *sm, HashClientes *hash) {
    FILE *f = fopen(filename, "r");
    char linha[MAX_LINE];
    int caixasNoFicheiro = -1;
    int caixasDeclaradas = 0;
    int caixaAtual = -1;
    int clientesEsperados = -1;
    int clientesLidosParaCaixa = 0;

    if (!f) return 0;

    while (fgets(linha, sizeof(linha), f)) {
        char *coment;
        trim(linha);
        if (linha[0] == '\0') continue;
        coment = strstr(linha, "//");
        if (coment) *coment = '\0';
        trim(linha);
        if (linha[0] == '\0') continue;

        if (caixasNoFicheiro == -1 && isdigit((unsigned char)linha[0])) {
            caixasNoFicheiro = atoi(linha);
            continue;
        }

        if (strncmp(linha, "Caixa", 5) == 0) {
            int numero, ativa;
            if (sscanf(linha, "Caixa%d : %d", &numero, &ativa) == 2 || sscanf(linha, "Caixa%d:%d", &numero, &ativa) == 2) {
                caixaAtual = numero - 1;
                clientesEsperados = -1;
                clientesLidosParaCaixa = 0;
                caixasDeclaradas++;
                if (caixaAtual >= 0 && caixaAtual < sm->cfg.nCaixas) {
                    sm->caixas[caixaAtual].estado = ativa ? CAIXA_ABERTA : CAIXA_FECHADA;
                }
            }
            continue;
        }

        if (caixaAtual >= 0 && clientesEsperados == -1 && isdigit((unsigned char)linha[0])) {
            clientesEsperados = atoi(linha);
            continue;
        }

        if (caixaAtual >= 0 && clientesEsperados >= 0 && clientesLidosParaCaixa < clientesEsperados) {
            char id[MAX_ID];
            int nProdutos;
            if (sscanf(linha, "%31[^:]: %d", id, &nProdutos) == 2 || sscanf(linha, "%31[^:]:%d", id, &nProdutos) == 2) {
                Cliente *cliente;
                trim(id);
                {
                    Produto *prods = sm->catalogo
                        ? catalog_obter_produtos_aleatorios(sm->catalogo, nProdutos, &sm->cfg)
                        : gerar_produtos_aleatorios(nProdutos, &sm->cfg);
                    cliente = criar_cliente(id, "", nProdutos, sm->instanteAtual, caixaAtual, prods);
                }
                if (cliente) {
                    if (fila_inserir(&sm->caixas[caixaAtual].fila, cliente) && hash_inserir(hash, cliente, caixaAtual)) {
                        clientesLidosParaCaixa++;
                    } else {
                        fila_remover_por_id(&sm->caixas[caixaAtual].fila, cliente->id);
                        destruir_cliente(cliente);
                    }
                }
            }
        }
    }

    fclose(f);
    if (caixasNoFicheiro >= 0 && caixasDeclaradas != caixasNoFicheiro) {
        printf("Aviso: o ficheiro declarava %d caixas, mas foram lidas %d.\n", caixasNoFicheiro, caixasDeclaradas);
    }
    log_acao(sm->logFile, "CARREGAR_DADOS", filename);
    return 1;
}

void mostrar_caixa(const Caixa *caixa) {
    const char *estado = (caixa->estado == CAIXA_ABERTA) ? "ABERTA" : (caixa->estado == CAIXA_A_FECHAR ? "A_FECHAR" : "FECHADA");
    printf("\nCaixa %d | operador=%s | estado=%s\n", caixa->id + 1, caixa->operador, estado);
    if (caixa->emAtendimento) {
        printf("  Em atendimento: %s | tempo_restante=%d\n", caixa->emAtendimento->id, caixa->tempoRestanteAtendimento);
    } else {
        printf("  Em atendimento: (nenhum)\n");
    }
    printf("  Pessoas na fila: %d\n", caixa->fila.tamanho);
    fila_listar(&caixa->fila);
}

void mostrar_estado_supermercado(const Supermercado *sm) {
    int i;
    printf("\n================ ESTADO DO SUPERMERCADO ================\n");
    printf("Instante atual: %d\n", sm->instanteAtual);
    for (i = 0; i < sm->cfg.nCaixas; ++i) mostrar_caixa(&sm->caixas[i]);
}

int inserir_novo_cliente(Supermercado *sm, HashClientes *hash, const char *idOriginal, const char *nome, Produto *produtos, int nProdutos) {
    int destino;
    char detalhes[128];
    char id[MAX_ID];
    Cliente *cliente;
    Produto *prods_copy;
    int i;

    strncpy(id, idOriginal, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    trim(id);
    if (id[0] == '\0' || !produtos || nProdutos <= 0) return INSERIR_CLIENTE_INVALIDO;

    if (hash_pesquisar(hash, id)) return INSERIR_CLIENTE_DUPLICADO;

    destino = encontrar_caixa_para_novo_cliente(sm);
    if (destino == -1) return INSERIR_CLIENTE_SEM_CAIXA;

    prods_copy = (Produto *)malloc(sizeof(Produto) * nProdutos);
    if (!prods_copy) return INSERIR_CLIENTE_MEMORIA;
    for (i = 0; i < nProdutos; i++) prods_copy[i] = produtos[i];

    cliente = criar_cliente(id, nome ? nome : "", nProdutos, sm->instanteAtual, destino, prods_copy);
    if (!cliente) return INSERIR_CLIENTE_MEMORIA;
    if (!fila_inserir(&sm->caixas[destino].fila, cliente)) {
        destruir_cliente(cliente);
        return INSERIR_CLIENTE_MEMORIA;
    }
    if (!hash_inserir(hash, cliente, destino)) {
        fila_remover_por_id(&sm->caixas[destino].fila, id);
        destruir_cliente(cliente);
        return INSERIR_CLIENTE_MEMORIA;
    }
    snprintf(detalhes, sizeof(detalhes), "%s entrou na caixa %d com %d produtos", id, destino + 1, nProdutos);
    log_acao(sm->logFile, "NOVO_CLIENTE", detalhes);
    verificar_politica_caixas(sm, hash);
    return destino;
}

int mover_cliente_caixa(Supermercado *sm, HashClientes *hash, const char *idOriginal, int novaCaixa) {
    char id[MAX_ID];
    NoHash *registo;
    Cliente *cliente;
    int caixaOrigem;
    char detalhes[128];

    strncpy(id, idOriginal, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    trim(id);
    registo = hash_pesquisar(hash, id);
    if (!registo) return 0;
    if (novaCaixa < 0 || novaCaixa >= sm->cfg.nCaixas) return 0;
    if (sm->caixas[novaCaixa].estado != CAIXA_ABERTA) return 0;
    if (registo->idCaixa == novaCaixa) return 2;
    caixaOrigem = registo->idCaixa;

    cliente = fila_remover_por_id(&sm->caixas[caixaOrigem].fila, id);
    if (!cliente) return 0;
    cliente->caixaAtual = novaCaixa;
    if (!fila_inserir(&sm->caixas[novaCaixa].fila, cliente)) {
        cliente->caixaAtual = caixaOrigem;
        fila_inserir(&sm->caixas[caixaOrigem].fila, cliente);
        return 0;
    }
    hash_atualizar_caixa(hash, id, novaCaixa);
    snprintf(detalhes, sizeof(detalhes), "%s mudou da caixa %d para a caixa %d", id, caixaOrigem + 1, novaCaixa + 1);
    log_acao(sm->logFile, "MUDAR_CAIXA", detalhes);
    verificar_politica_caixas(sm, hash);
    return 1;
}

static void concluir_atendimento(Supermercado *sm, HashClientes *hash, Caixa *caixa) {
    Cliente *cliente = caixa->emAtendimento;
    char detalhes[160];
    if (!cliente) return;
    cliente->atendido = true;
    cliente->tempoEsperaTotal = cliente->instanteInicioAtendimento - cliente->instanteEntradaFila;
    caixa->totalClientesAtendidos++;
    caixa->totalProdutosVendidos += (cliente->nProdutos - cliente->produtosOferecidos);
    caixa->totalValorVendido += cliente_valor_total(cliente) - cliente->valorOferecido;
    caixa_adicionar_historico(caixa, cliente);
    sm->totalClientesAtendidos++;
    sm->totalProdutosVendidos += (cliente->nProdutos - cliente->produtosOferecidos);
    sm->somaTemposEspera += cliente->tempoEsperaTotal;
    hash_remover(hash, cliente->id);
    snprintf(detalhes, sizeof(detalhes), "%s atendido na caixa %d | espera=%d | oferecidos=%d | valor_oferecido=%.2f",
             cliente->id, caixa->id + 1, cliente->tempoEsperaTotal, cliente->produtosOferecidos, cliente->valorOferecido);
    log_acao(sm->logFile, "CLIENTE_ATENDIDO", detalhes);
    caixa->emAtendimento = NULL;
    caixa->tempoRestanteAtendimento = 0;
    if (caixa->estado == CAIXA_A_FECHAR && fila_vazia(&caixa->fila)) caixa->estado = CAIXA_FECHADA;
}

void avancar_simulacao(Supermercado *sm, HashClientes *hash, int passos) {
    int p;
    for (p = 0; p < passos; ++p) {
        int i;
        sm->instanteAtual++;

        for (i = 0; i < sm->cfg.nCaixas; ++i) {
            Caixa *caixa = &sm->caixas[i];
            if (caixa->estado == CAIXA_ABERTA || caixa->estado == CAIXA_A_FECHAR) {
                iniciar_atendimento(caixa, sm->instanteAtual);
                if (caixa->emAtendimento) {
                    caixa->tempoRestanteAtendimento--;
                    if (caixa->tempoRestanteAtendimento <= 0) concluir_atendimento(sm, hash, caixa);
                }
            }
        }

        for (i = 0; i < sm->cfg.nCaixas; ++i) {
            NoFila *no = sm->caixas[i].fila.cabeca;
            while (no) {
                Cliente *cliente = no->cliente;
                cliente->tempoEsperaTotal = sm->instanteAtual - cliente->instanteEntradaFila;
                if (cliente->tempoEsperaTotal > sm->cfg.maxEspera && !cliente->oferecimentoFeito) {
                    float valor = oferecer_um_produto(cliente);
                    if (valor > 0.0f) {
                        char detalhes[160];
                        cliente->oferecimentoFeito = true;
                        sm->totalProdutosOferecidos++;
                        sm->totalValorOferecido += valor;
                        snprintf(detalhes, sizeof(detalhes), "%s ultrapassou MAX_ESPERA na caixa %d | produto_oferecido=%.2f",
                                 cliente->id, i + 1, valor);
                        log_acao(sm->logFile, "PRODUTO_OFERECIDO", detalhes);
                    }
                }
                no = no->seguinte;
            }
        }

        verificar_politica_caixas(sm, hash);
    }
}

void verificar_politica_caixas(Supermercado *sm, HashClientes *hash) {
    int abertas = contar_caixas_abertas(sm);
    int totalFilas = total_clientes_em_filas(sm);
    double media;

    if (abertas <= 0) return;
    media = (double)totalFilas / abertas;

    if (media > sm->cfg.maxFila) {
        int i;
        for (i = 0; i < sm->cfg.nCaixas; ++i) {
            if (sm->caixas[i].estado == CAIXA_FECHADA) {
                sm->caixas[i].estado = CAIXA_ABERTA;
                log_acao(sm->logFile, "ABRIR_CAIXA_AUTO", sm->caixas[i].operador);
                return;
            }
        }
    }

    if (media < sm->cfg.minFila && contar_caixas_realmente_abertas(sm) > 1) {
        int idx = encontrar_caixa_menos_pessoas_aberta(sm, -1);
        if (idx != -1 && sm->caixas[idx].fila.tamanho == 0 && sm->caixas[idx].emAtendimento == NULL) {
            sm->caixas[idx].estado = CAIXA_FECHADA;
            log_acao(sm->logFile, "FECHAR_CAIXA_AUTO", sm->caixas[idx].operador);
        } else if (idx != -1) {
            sm->caixas[idx].estado = CAIXA_A_FECHAR;
            log_acao(sm->logFile, "FECHAR_CAIXA_SUAVE_AUTO", sm->caixas[idx].operador);
        }
    }

    (void)hash;
}

int abrir_caixa_manual(Supermercado *sm, int idCaixa) {
    char detalhes[64];
    if (idCaixa < 0 || idCaixa >= sm->cfg.nCaixas) return 0;
    sm->caixas[idCaixa].estado = CAIXA_ABERTA;
    snprintf(detalhes, sizeof(detalhes), "Caixa %d aberta manualmente", idCaixa + 1);
    log_acao(sm->logFile, "ABRIR_CAIXA_MANUAL", detalhes);
    return 1;
}

int fechar_caixa_suave_manual(Supermercado *sm, int idCaixa) {
    char detalhes[64];
    if (idCaixa < 0 || idCaixa >= sm->cfg.nCaixas) return 0;
    if (sm->caixas[idCaixa].estado == CAIXA_FECHADA) return 0;
    sm->caixas[idCaixa].estado = CAIXA_A_FECHAR;
    snprintf(detalhes, sizeof(detalhes), "Caixa %d marcada para fecho suave", idCaixa + 1);
    log_acao(sm->logFile, "FECHAR_CAIXA_SUAVE_MANUAL", detalhes);
    return 1;
}

int fechar_caixa_imediata_manual(Supermercado *sm, HashClientes *hash, int idCaixa) {
    char detalhes[64];
    if (idCaixa < 0 || idCaixa >= sm->cfg.nCaixas) return 0;
    if (contar_caixas_abertas(sm) <= 1) return 0;
    if (sm->caixas[idCaixa].emAtendimento) {
        Cliente *cliente = sm->caixas[idCaixa].emAtendimento;
        int destino = encontrar_caixa_menos_pessoas_aberta(sm, idCaixa);
        cliente->estavaEmAtendimento = false;
        cliente->instanteInicioAtendimento = -1;
        sm->caixas[idCaixa].emAtendimento = NULL;
        sm->caixas[idCaixa].tempoRestanteAtendimento = 0;
        if (destino >= 0 && fila_inserir(&sm->caixas[destino].fila, cliente)) {
            cliente->caixaAtual = destino;
            hash_atualizar_caixa(hash, cliente->id, destino);
        } else {
            sm->caixas[idCaixa].emAtendimento = cliente;
            sm->caixas[idCaixa].tempoRestanteAtendimento = cliente_tempo_atendimento(cliente);
            return 0;
        }
    }
    redistribuir_fila(sm, hash, idCaixa);
    sm->caixas[idCaixa].estado = CAIXA_FECHADA;
    snprintf(detalhes, sizeof(detalhes), "Caixa %d fechada de imediato com redistribuicao", idCaixa + 1);
    log_acao(sm->logFile, "FECHAR_CAIXA_IMEDIATA_MANUAL", detalhes);
    return 1;
}

void pesquisar_cliente(Supermercado *sm, HashClientes *hash, const char *idOriginal) {
    char id[MAX_ID];
    NoHash *no;
    int pos;
    strncpy(id, idOriginal, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    trim(id);
    no = hash_pesquisar(hash, id);
    if (!no) {
        printf("\nCliente %s nao esta em espera em nenhuma caixa.\n", id);
        return;
    }
    pos = fila_posicao_cliente(&sm->caixas[no->idCaixa].fila, id);
    if (pos > 0) {
        printf("\nCliente %s encontrado na caixa %d, posicao %d na fila.\n", id, no->idCaixa + 1, pos);
    } else if (sm->caixas[no->idCaixa].emAtendimento && strcmp(sm->caixas[no->idCaixa].emAtendimento->id, id) == 0) {
        printf("\nCliente %s esta em atendimento na caixa %d.\n", id, no->idCaixa + 1);
    } else {
        printf("\nCliente %s encontrado na caixa %d.\n", id, no->idCaixa + 1);
    }
    mostrar_cliente(no->cliente);
}

size_t memoria_utilizada(const Supermercado *sm, const HashClientes *hash) {
    size_t total = sizeof(Supermercado);
    int i;
    total += sizeof(Caixa) * sm->cfg.nCaixas;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        NoFila *no = sm->caixas[i].fila.cabeca;
        total += sizeof(Cliente *) * sm->caixas[i].capHistorico;
        while (no) {
            total += sizeof(NoFila);
            total += sizeof(Cliente);
            total += sizeof(Produto) * no->cliente->nProdutos;
            no = no->seguinte;
        }
        if (sm->caixas[i].emAtendimento) {
            total += sizeof(Cliente);
            total += sizeof(Produto) * sm->caixas[i].emAtendimento->nProdutos;
        }
        {
            int j;
            for (j = 0; j < sm->caixas[i].nHistorico; ++j) {
                total += sizeof(Cliente);
                total += sizeof(Produto) * sm->caixas[i].historicoAtendidos[j]->nProdutos;
            }
        }
    }
    total += sizeof(HashClientes);
    total += hash_memoria(hash);
    return total;
}

size_t memoria_desperdicada(const Supermercado *sm, const HashClientes *hash) {
    size_t total = 0;
    int i;
    for (i = 0; i < sm->cfg.nCaixas; ++i) total += caixa_memoria_desperdicada(&sm->caixas[i]);
    total += hash_memoria_desperdicada(hash);
    return total;
}

int guardar_estatisticas_csv(const char *filename, const Supermercado *sm) {
    FILE *f = fopen(filename, "w");
    double mediaEspera = (sm->totalClientesAtendidos > 0) ? ((double)sm->somaTemposEspera / sm->totalClientesAtendidos) : 0.0;
    int i;
    if (!f) {
        printf("Nao foi possivel criar %s\n", filename);
        return 0;
    }
    fprintf(f, "instante_final,total_clientes_atendidos,total_produtos_vendidos,tempo_medio_espera,total_produtos_oferecidos,total_valor_oferecido\n");
    fprintf(f, "%d,%d,%d,%.2f,%d,%.2f\n", sm->instanteAtual, sm->totalClientesAtendidos, sm->totalProdutosVendidos, mediaEspera, sm->totalProdutosOferecidos, sm->totalValorOferecido);
    fprintf(f, "\ncaixa,operador,clientes_atendidos,produtos_vendidos,valor_vendido\n");
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        fprintf(f, "%d,%s,%d,%d,%.2f\n", i + 1, sm->caixas[i].operador, sm->caixas[i].totalClientesAtendidos, sm->caixas[i].totalProdutosVendidos, sm->caixas[i].totalValorVendido);
    }
    fclose(f);
    return 1;
}

int guardar_historico_caixas_csv(const char *filename, const Supermercado *sm) {
    FILE *f = fopen(filename, "w");
    int i, j;
    if (!f) {
        printf("Nao foi possivel criar %s\n", filename);
        return 0;
    }
    fprintf(f, "caixa,operador,cliente,produtos,tempo_espera,produtos_oferecidos,valor_oferecido\n");
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        for (j = 0; j < sm->caixas[i].nHistorico; ++j) {
            Cliente *c = sm->caixas[i].historicoAtendidos[j];
            fprintf(f, "%d,%s,%s,%d,%d,%d,%.2f\n", i + 1, sm->caixas[i].operador, c->id, c->nProdutos, c->tempoEsperaTotal, c->produtosOferecidos, c->valorOferecido);
        }
    }
    fclose(f);
    return 1;
}

void mostrar_estatisticas_finais(const Supermercado *sm) {
    int i;
    int idxMaisClientes = -1, idxMaisProdutos = -1, idxMenosClientes = -1;
    int maxClientes = -1, maxProdutos = -1, minClientes = 0;
    double mediaEspera = (sm->totalClientesAtendidos > 0) ? ((double)sm->somaTemposEspera / sm->totalClientesAtendidos) : 0.0;

    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        const Caixa *c = &sm->caixas[i];
        if (idxMaisClientes == -1 || c->totalClientesAtendidos > maxClientes) {
            maxClientes = c->totalClientesAtendidos;
            idxMaisClientes = i;
        }
        if (idxMaisProdutos == -1 || c->totalProdutosVendidos > maxProdutos) {
            maxProdutos = c->totalProdutosVendidos;
            idxMaisProdutos = i;
        }
        if (idxMenosClientes == -1 || c->totalClientesAtendidos < minClientes) {
            minClientes = c->totalClientesAtendidos;
            idxMenosClientes = i;
        }
    }

    printf("\n================ ESTATISTICAS FINAIS ================\n");
    printf("Instante final: %d\n", sm->instanteAtual);
    printf("Total de clientes atendidos: %d\n", sm->totalClientesAtendidos);
    printf("Total de produtos vendidos: %d\n", sm->totalProdutosVendidos);
    printf("Tempo medio de espera: %.2f\n", mediaEspera);
    printf("Produtos oferecidos: %d\n", sm->totalProdutosOferecidos);
    printf("Valor oferecido: %.2f\n", sm->totalValorOferecido);
    if (idxMaisClientes != -1) printf("Caixa que atendeu mais pessoas: Caixa %d (%s) com %d clientes\n", idxMaisClientes + 1, sm->caixas[idxMaisClientes].operador, sm->caixas[idxMaisClientes].totalClientesAtendidos);
    if (idxMaisProdutos != -1) printf("Caixa que vendeu mais produtos: Caixa %d (%s) com %d produtos\n", idxMaisProdutos + 1, sm->caixas[idxMaisProdutos].operador, sm->caixas[idxMaisProdutos].totalProdutosVendidos);
    if (idxMenosClientes != -1) printf("Operador que atendeu menos pessoas: %s com %d clientes\n", sm->caixas[idxMenosClientes].operador, sm->caixas[idxMenosClientes].totalClientesAtendidos);

    for (i = 0; i < sm->cfg.nCaixas; ++i) caixa_listar_historico(&sm->caixas[i]);
}
