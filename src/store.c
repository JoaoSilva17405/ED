#include "store.h"
#include "utils.h"
#include "client_registry.h"

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

static int total_filas_apenas_abertas(const Supermercado *sm) {
    int i, total = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        if (sm->caixas[i].estado == CAIXA_ABERTA)
            total += sm->caixas[i].fila.tamanho;
    }
    return total;
}

static int encontrar_caixa_menos_fila_aberta(const Supermercado *sm) {
    int i, idx = -1, min = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) {
        const Caixa *c = &sm->caixas[i];
        if (c->estado != CAIXA_ABERTA) continue;
        if (idx == -1 || c->fila.tamanho < min) {
            idx = i;
            min = c->fila.tamanho;
        }
    }
    return idx;
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
    sm->clientesEmLoja = NULL;
    sm->nClientesEmLoja = 0;
    sm->capClientesEmLoja = 0;
    sm->instanteAtual = 0;
    sm->totalClientesAtendidos = 0;
    sm->totalProdutosVendidos = 0;
    sm->totalProdutosOferecidos = 0;
    sm->totalValorOferecido = 0.0f;
    sm->somaTemposEspera = 0;
    sm->logFile = logFile;
    sm->instanteUltimoFecho = -1;
    sm->instanteUltimaAberturaManual = -4;
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
    for (i = 0; i < sm->nClientesEmLoja; ++i) destruir_cliente(sm->clientesEmLoja[i]);
    free(sm->clientesEmLoja);
    sm->clientesEmLoja = NULL;
    sm->nClientesEmLoja = 0;
    sm->capClientesEmLoja = 0;
    for (i = 0; i < sm->cfg.nCaixas; ++i) caixa_destruir(&sm->caixas[i], true);
    free(sm->caixas);
    sm->caixas = NULL;
    if (hash) hash_destruir(hash);
}

/* ---- snapshot helpers ---- */

static int tokenizar_linha(char *str, char **tokens, int max) {
    int n = 0;
    char *p = str;
    while (*p && n < max) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        tokens[n++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return n;
}

static int parse_produto_linha(char *linha, Produto *prod) {
    char buf[MAX_LINE];
    char *tokens[64];
    int n, k;
    char nome_buf[MAX_NOME];

    strncpy(buf, linha, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    n = tokenizar_linha(buf, tokens, 64);
    /* need: PRODUTO id nome(>=1) preco stock tempo oferecido → min 7 */
    if (n < 7) return 0;

    prod->id            = atoi(tokens[1]);
    prod->preco         = (float)atof(tokens[n - 4]);
    prod->tempoCompra   = (int)atof(tokens[n - 3]);
    prod->tempoPassagem = atoi(tokens[n - 2]);
    prod->oferecido     = (strcmp(tokens[n - 1], "true") == 0);

    nome_buf[0] = '\0';
    for (k = 2; k < n - 4; k++) {
        if (k > 2) strncat(nome_buf, " ", MAX_NOME - 1 - strlen(nome_buf));
        strncat(nome_buf, tokens[k], MAX_NOME - 1 - strlen(nome_buf));
    }
    strncpy(prod->nome, nome_buf, MAX_NOME - 1);
    prod->nome[MAX_NOME - 1] = '\0';
    return 1;
}

static int parse_id_nome_ate(const char *p, const char *marcador,
                              char *id_out, char *nome_out) {
    const char *mar_pos;
    const char *id_end;
    size_t id_len, nome_len;

    while (*p == ' ') p++;
    mar_pos = strstr(p, marcador);
    if (!mar_pos) return 0;

    id_end = strchr(p, ' ');
    if (!id_end || id_end >= mar_pos) return 0;
    id_len = (size_t)(id_end - p);
    if (id_len >= MAX_ID) id_len = MAX_ID - 1;
    strncpy(id_out, p, id_len);
    id_out[id_len] = '\0';

    p = id_end + 1;
    nome_len = (size_t)(mar_pos - p);
    if (nome_len >= MAX_NOME) nome_len = MAX_NOME - 1;
    strncpy(nome_out, p, nome_len);
    nome_out[nome_len] = '\0';
    while (nome_len > 0 && (nome_out[nome_len - 1] == ' ' || nome_out[nome_len - 1] == '\t')) {
        nome_out[--nome_len] = '\0';
    }
    return 1;
}

static void guardar_produtos_snapshot(FILE *f, const Cliente *c, int indent) {
    int p;
    for (p = 0; p < c->nProdutos; p++) {
        fprintf(f, "%*sPRODUTO %d %s %.2f %d %d %s\n",
                indent, "",
                c->produtos[p].id, c->produtos[p].nome,
                c->produtos[p].preco, c->produtos[p].tempoCompra,
                c->produtos[p].tempoPassagem,
                c->produtos[p].oferecido ? "true" : "false");
    }
}

int guardar_snapshot(const char *filename, const Supermercado *sm, const RegistoClientes *registo) {
    FILE *f;
    int i;

    if (!filename || !sm) return 0;
    f = fopen(filename, "w");
    if (!f) return 0;

    fprintf(f, "INSTANTE %d\n", sm->instanteAtual);
    fprintf(f, "ULTIMO_FECHO %d\n", sm->instanteUltimoFecho);
    fprintf(f, "ULTIMA_ABERTURA_MANUAL %d\n", sm->instanteUltimaAberturaManual);
    fprintf(f, "EM_LOJA %d\n", sm->nClientesEmLoja);
    for (i = 0; i < sm->nClientesEmLoja; i++) {
        const Cliente *c = sm->clientesEmLoja[i];
        fprintf(f, "  LOJA_CLIENTE %s %s entrada_loja=%d tempo_compra=%d\n",
                c->id, c->nome[0] ? c->nome : "Desconhecido",
                c->instanteEntradaLoja, c->tempoCompraTotal);
        guardar_produtos_snapshot(f, c, 4);
    }
    fprintf(f, "CAIXAS %d\n", sm->cfg.nCaixas);

    for (i = 0; i < sm->cfg.nCaixas; i++) {
        const Caixa *cx = &sm->caixas[i];
        const char *estado_str = (cx->estado == CAIXA_ABERTA)    ? "ABERTA" :
                                  (cx->estado == CAIXA_A_FECHAR) ? "A_FECHAR" : "FECHADA";
        const NoFila *no;

        fprintf(f, "CAIXA %d %s %s\n", i + 1, estado_str, cx->operador);

        if (cx->emAtendimento) {
            const Cliente *c = cx->emAtendimento;
            const char *nome_c = c->nome[0] ? c->nome : "Desconhecido";
            if (registo) {
                const EntradaCliente *e = registo_pesquisar_id(registo, c->id);
                if (e) nome_c = e->nome;
            }
            fprintf(f, "  EM_ATENDIMENTO %s %s tempo_restante=%d entrada=%d inicio=%d\n",
                    c->id, nome_c, cx->tempoRestanteAtendimento,
                    c->instanteEntradaFila, c->instanteInicioAtendimento);
            guardar_produtos_snapshot(f, c, 4);
        }

        fprintf(f, "  FILA %d\n", cx->fila.tamanho);
        no = cx->fila.cabeca;
        while (no) {
            const Cliente *c = no->cliente;
            const char *nome_c = c->nome[0] ? c->nome : "Desconhecido";
            if (registo) {
                const EntradaCliente *e = registo_pesquisar_id(registo, c->id);
                if (e) nome_c = e->nome;
            }
            fprintf(f, "    CLIENTE %s %s entrada=%d\n", c->id, nome_c, c->instanteEntradaFila);
            guardar_produtos_snapshot(f, c, 6);
            no = no->seguinte;
        }
    }

    fclose(f);
    return 1;
}

/* ---- place a shopping-phase client from snapshot into clientesEmLoja ---- */

static void finalizar_cliente_loja(
    Supermercado *sm, HashClientes *hash,
    const char *pid, const char *pnome,
    int pEntradaLoja, int pTempoCompra,
    Produto *prods, int nProds)
{
    Produto *prods_copy;
    Cliente *cliente;
    int j;
    Cliente **nova_lista;

    if (pid[0] == '\0' || nProds == 0) return;

    prods_copy = (Produto *)malloc(sizeof(Produto) * nProds);
    if (!prods_copy) return;
    for (j = 0; j < nProds; j++) prods_copy[j] = prods[j];

    cliente = criar_cliente(pid, pnome, nProds, pEntradaLoja, -1, prods_copy);
    if (!cliente) return;
    cliente->instanteEntradaLoja = pEntradaLoja;
    cliente->tempoCompraTotal    = pTempoCompra;
    cliente->comprasTerminadas   = false;
    cliente->instanteEntradaFila = -1;

    if (sm->nClientesEmLoja == sm->capClientesEmLoja) {
        int nova_cap = sm->capClientesEmLoja + 16;
        nova_lista = (Cliente **)realloc(sm->clientesEmLoja, sizeof(Cliente *) * nova_cap);
        if (!nova_lista) { destruir_cliente(cliente); return; }
        sm->clientesEmLoja    = nova_lista;
        sm->capClientesEmLoja = nova_cap;
    }
    sm->clientesEmLoja[sm->nClientesEmLoja++] = cliente;
    if (hash) hash_inserir(hash, cliente, -1);
}

/* ---- finalize a pending client being assembled from snapshot lines ---- */

static void finalizar_cliente_pendente(
    Supermercado *sm, HashClientes *hash,
    const char *pid, const char *pnome,
    int pEntrada, int pInicio,
    bool pEmAtendimento, int pTempoRestante,
    Produto *prods, int nProds, int caixaAtual)
{
    Produto *prods_copy;
    Cliente *cliente;
    int j;

    if (pid[0] == '\0' || nProds == 0) return;
    if (caixaAtual < 0 || caixaAtual >= sm->cfg.nCaixas) return;

    prods_copy = (Produto *)malloc(sizeof(Produto) * nProds);
    if (!prods_copy) return;
    for (j = 0; j < nProds; j++) prods_copy[j] = prods[j];

    cliente = criar_cliente(pid, pnome, nProds, pEntrada, caixaAtual, prods_copy);
    if (!cliente) return;
    cliente->instanteInicioAtendimento = pInicio;
    cliente->comprasTerminadas = true;

    if (pEmAtendimento) {
        cliente->estavaEmAtendimento = true;
        sm->caixas[caixaAtual].emAtendimento = cliente;
        sm->caixas[caixaAtual].tempoRestanteAtendimento = pTempoRestante;
    } else {
        if (!fila_inserir(&sm->caixas[caixaAtual].fila, cliente)) {
            destruir_cliente(cliente);
            return;
        }
        if (hash) hash_inserir(hash, cliente, caixaAtual);
    }
}

/* ---- new format parser ---- */

static int carregar_formato_novo(FILE *f, Supermercado *sm, HashClientes *hash, char *primeira_linha) {
    char linha[MAX_LINE];
    int caixaAtual = -1;
    char pendingId[MAX_ID]   = {0};
    char pendingNome[MAX_NOME] = {0};
    int pendingEntrada = 0, pendingInicio = -1, pendingTempoRestante = 0;
    bool pendingEmAtendimento = false;
    bool pendingEmLoja = false;
    int pendingEntradaLoja = 0, pendingTempoCompraLoja = 0;
    Produto prods[512];
    int nProds = 0;

    /* parse first line (already read before detecting format) */
    sscanf(primeira_linha, "INSTANTE %d", &sm->instanteAtual);

    while (fgets(linha, sizeof(linha), f)) {
        char *p = linha;
        trim(linha);
        if (linha[0] == '\0') continue;

        /* strip leading spaces for keyword detection */
        while (*p == ' ') p++;

        if (strncmp(p, "PRODUTO", 7) == 0) {
            if (nProds < 512) {
                parse_produto_linha(p, &prods[nProds]);
                nProds++;
            }
            continue;
        }

        /* any non-PRODUTO line finalizes the pending client */
        if (pendingId[0] != '\0') {
            if (pendingEmLoja) {
                finalizar_cliente_loja(sm, hash,
                    pendingId, pendingNome,
                    pendingEntradaLoja, pendingTempoCompraLoja,
                    prods, nProds);
            } else {
                finalizar_cliente_pendente(sm, hash,
                    pendingId, pendingNome, pendingEntrada, pendingInicio,
                    pendingEmAtendimento, pendingTempoRestante,
                    prods, nProds, caixaAtual);
            }
            pendingId[0] = '\0';
            nProds = 0;
            pendingEmAtendimento = false;
            pendingEmLoja = false;
        }

        if (strncmp(p, "EM_LOJA", 7) == 0) {
            continue; /* count informational, ignore */
        } else if (strncmp(p, "LOJA_CLIENTE", 12) == 0) {
            const char *rest = p + 12;
            char id[MAX_ID], nome[MAX_NOME];
            int el = 0, tc = 0;
            const char *el_pos;
            while (*rest == ' ') rest++;
            if (parse_id_nome_ate(rest, "entrada_loja=", id, nome)) {
                el_pos = strstr(rest, "entrada_loja=");
                sscanf(el_pos, "entrada_loja=%d tempo_compra=%d", &el, &tc);
                strncpy(pendingId, id, MAX_ID - 1);
                strncpy(pendingNome, nome, MAX_NOME - 1);
                pendingEntradaLoja     = el;
                pendingTempoCompraLoja = tc;
                pendingEmLoja = true;
                nProds = 0;
            }
            continue;
        } else if (strncmp(p, "CAIXAS", 6) == 0) {
            continue;
        } else if (strncmp(p, "CAIXA", 5) == 0) {
            int num;
            char estado_str[16], operador[MAX_OPERADOR];
            if (sscanf(p, "CAIXA %d %15s %63[^\n]", &num, estado_str, operador) >= 2) {
                caixaAtual = num - 1;
                if (caixaAtual >= 0 && caixaAtual < sm->cfg.nCaixas) {
                    if (strcmp(estado_str, "ABERTA") == 0)
                        sm->caixas[caixaAtual].estado = CAIXA_ABERTA;
                    else if (strcmp(estado_str, "A_FECHAR") == 0)
                        sm->caixas[caixaAtual].estado = CAIXA_A_FECHAR;
                    else
                        sm->caixas[caixaAtual].estado = CAIXA_FECHADA;
                }
            }
        } else if (strncmp(p, "EM_ATENDIMENTO", 14) == 0) {
            const char *rest = p + 14;
            char id[MAX_ID], nome[MAX_NOME];
            int tr = 0, ent = 0, ini = -1;
            const char *tr_pos;
            while (*rest == ' ') rest++;
            if (parse_id_nome_ate(rest, "tempo_restante=", id, nome)) {
                tr_pos = strstr(rest, "tempo_restante=");
                sscanf(tr_pos, "tempo_restante=%d entrada=%d inicio=%d", &tr, &ent, &ini);
                strncpy(pendingId, id, MAX_ID - 1);
                strncpy(pendingNome, nome, MAX_NOME - 1);
                pendingEntrada = ent;
                pendingInicio  = ini;
                pendingTempoRestante = tr;
                pendingEmAtendimento = true;
                nProds = 0;
            }
        } else if (strncmp(p, "CLIENTE", 7) == 0) {
            const char *rest = p + 7;
            char id[MAX_ID], nome[MAX_NOME];
            int ent = 0;
            const char *ent_pos;
            while (*rest == ' ') rest++;
            if (parse_id_nome_ate(rest, "entrada=", id, nome)) {
                ent_pos = strstr(rest, "entrada=");
                sscanf(ent_pos, "entrada=%d", &ent);
                strncpy(pendingId, id, MAX_ID - 1);
                strncpy(pendingNome, nome, MAX_NOME - 1);
                pendingEntrada = ent;
                pendingInicio  = -1;
                pendingTempoRestante = 0;
                pendingEmAtendimento = false;
                nProds = 0;
            }
        } else if (strncmp(p, "ULTIMO_FECHO", 12) == 0) {
            sscanf(p, "ULTIMO_FECHO %d", &sm->instanteUltimoFecho);
        } else if (strncmp(p, "ULTIMA_ABERTURA_MANUAL", 22) == 0) {
            sscanf(p, "ULTIMA_ABERTURA_MANUAL %d", &sm->instanteUltimaAberturaManual);
        } else if (strncmp(p, "INSTANTE", 8) == 0) {
            sscanf(p, "INSTANTE %d", &sm->instanteAtual);
        }
        /* FILA N: just informational, ignore count */
    }

    /* finalize last pending client */
    if (pendingId[0] != '\0') {
        if (pendingEmLoja) {
            finalizar_cliente_loja(sm, hash,
                pendingId, pendingNome,
                pendingEntradaLoja, pendingTempoCompraLoja,
                prods, nProds);
        } else {
            finalizar_cliente_pendente(sm, hash,
                pendingId, pendingNome, pendingEntrada, pendingInicio,
                pendingEmAtendimento, pendingTempoRestante,
                prods, nProds, caixaAtual);
        }
    }
    return 1;
}

/* ---- old format fallback ---- */

static int carregar_formato_antigo(const char *filename, Supermercado *sm, HashClientes *hash) {
    FILE *f = fopen(filename, "r");
    char linha[MAX_LINE];
    int caixasNoFicheiro = -1, caixasDeclaradas = 0;
    int caixaAtual = -1, clientesEsperados = -1, clientesLidos = 0;

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
            caixasNoFicheiro = atoi(linha); continue;
        }
        if (strncmp(linha, "Caixa", 5) == 0) {
            int numero, ativa;
            if (sscanf(linha, "Caixa%d : %d", &numero, &ativa) == 2 ||
                sscanf(linha, "Caixa%d:%d",   &numero, &ativa) == 2) {
                caixaAtual = numero - 1;
                clientesEsperados = -1; clientesLidos = 0; caixasDeclaradas++;
                if (caixaAtual >= 0 && caixaAtual < sm->cfg.nCaixas)
                    sm->caixas[caixaAtual].estado = ativa ? CAIXA_ABERTA : CAIXA_FECHADA;
            }
            continue;
        }
        if (caixaAtual >= 0 && clientesEsperados == -1 && isdigit((unsigned char)linha[0])) {
            clientesEsperados = atoi(linha); continue;
        }
        if (caixaAtual >= 0 && clientesEsperados >= 0 && clientesLidos < clientesEsperados) {
            char id[MAX_ID]; int nProdutos;
            if (sscanf(linha, "%31[^:]: %d", id, &nProdutos) == 2 ||
                sscanf(linha, "%31[^:]:%d",  id, &nProdutos) == 2) {
                Produto *prods;
                Cliente *cliente;
                trim(id);
                prods = sm->catalogo
                    ? catalog_obter_produtos_aleatorios(sm->catalogo, nProdutos, &sm->cfg)
                    : gerar_produtos_aleatorios(nProdutos, &sm->cfg);
                cliente = criar_cliente(id, "", nProdutos, sm->instanteAtual, caixaAtual, prods);
                if (cliente) {
                    if (fila_inserir(&sm->caixas[caixaAtual].fila, cliente) &&
                        hash_inserir(hash, cliente, caixaAtual)) {
                        clientesLidos++;
                    } else {
                        fila_remover_por_id(&sm->caixas[caixaAtual].fila, cliente->id);
                        destruir_cliente(cliente);
                    }
                }
            }
        }
    }
    fclose(f);
    if (caixasNoFicheiro >= 0 && caixasDeclaradas != caixasNoFicheiro)
        printf("Aviso: o ficheiro declarava %d caixas, mas foram lidas %d.\n",
               caixasNoFicheiro, caixasDeclaradas);
    return 1;
}

int carregar_dados_iniciais(const char *filename, Supermercado *sm, HashClientes *hash) {
    FILE *f = fopen(filename, "r");
    char primeira[MAX_LINE];

    if (!f) return 0;
    if (!fgets(primeira, sizeof(primeira), f)) { fclose(f); return 1; }
    trim(primeira);

    if (strncmp(primeira, "INSTANTE", 8) == 0) {
        int r = carregar_formato_novo(f, sm, hash, primeira);
        fclose(f);
        log_acao(sm->logFile, "CARREGAR_DADOS", filename);
        return r;
    }
    fclose(f);
    log_acao(sm->logFile, "CARREGAR_DADOS", filename);
    return carregar_formato_antigo(filename, sm, hash);
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
    if (sm->nClientesEmLoja > 0) {
        printf("\n--- Clientes a fazer compras (%d) ---\n", sm->nClientesEmLoja);
        for (i = 0; i < sm->nClientesEmLoja; ++i) {
            const Cliente *c = sm->clientesEmLoja[i];
            int restante = (c->instanteEntradaLoja + c->tempoCompraTotal) - sm->instanteAtual;
            if (restante < 0) restante = 0;
            printf("  %s | compra=%ds | restante=%ds\n", c->id, c->tempoCompraTotal, restante);
        }
    }
    for (i = 0; i < sm->cfg.nCaixas; ++i) mostrar_caixa(&sm->caixas[i]);
}

int inserir_novo_cliente(Supermercado *sm, HashClientes *hash, const char *idOriginal, const char *nome, Produto *produtos, int nProdutos) {
    char detalhes[160];
    char id[MAX_ID];
    Cliente *cliente;
    Produto *prods_copy;
    int i;
    Cliente **nova_lista;

    strncpy(id, idOriginal, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    trim(id);
    if (id[0] == '\0' || !produtos || nProdutos <= 0) return INSERIR_CLIENTE_INVALIDO;

    if (hash_pesquisar(hash, id)) return INSERIR_CLIENTE_DUPLICADO;

    prods_copy = (Produto *)malloc(sizeof(Produto) * nProdutos);
    if (!prods_copy) return INSERIR_CLIENTE_MEMORIA;
    for (i = 0; i < nProdutos; i++) prods_copy[i] = produtos[i];

    cliente = criar_cliente(id, nome ? nome : "", nProdutos, sm->instanteAtual, -1, prods_copy);
    if (!cliente) { free(prods_copy); return INSERIR_CLIENTE_MEMORIA; }

    cliente->instanteEntradaLoja = sm->instanteAtual;
    cliente->tempoCompraTotal    = tempo_compra_total_produtos(prods_copy, nProdutos);
    cliente->comprasTerminadas   = false;
    cliente->instanteEntradaFila = -1;

    if (sm->nClientesEmLoja == sm->capClientesEmLoja) {
        int nova_cap = sm->capClientesEmLoja + 16;
        nova_lista = (Cliente **)realloc(sm->clientesEmLoja, sizeof(Cliente *) * nova_cap);
        if (!nova_lista) { destruir_cliente(cliente); return INSERIR_CLIENTE_MEMORIA; }
        sm->clientesEmLoja    = nova_lista;
        sm->capClientesEmLoja = nova_cap;
    }
    sm->clientesEmLoja[sm->nClientesEmLoja++] = cliente;

    if (!hash_inserir(hash, cliente, -1)) {
        sm->nClientesEmLoja--;
        destruir_cliente(cliente);
        return INSERIR_CLIENTE_MEMORIA;
    }

    snprintf(detalhes, sizeof(detalhes), "%s entrou no supermercado com %d produtos (compra: %ds)",
             id, nProdutos, cliente->tempoCompraTotal);
    log_acao(sm->logFile, "ENTRADA_LOJA", detalhes);
    return INSERIR_CLIENTE_EM_COMPRAS;
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
    if (registo->idCaixa < 0) return 0; /* ainda a fazer compras */
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
        int i, j;
        sm->instanteAtual++;

        /* fase de compras: mover clientes que terminaram para a fila */
        j = 0;
        while (j < sm->nClientesEmLoja) {
            Cliente *c = sm->clientesEmLoja[j];
            if (sm->instanteAtual >= c->instanteEntradaLoja + c->tempoCompraTotal) {
                int destino;
                char detalhes[128];
                c->comprasTerminadas   = true;
                c->instanteEntradaFila = sm->instanteAtual;
                destino = encontrar_caixa_para_novo_cliente(sm);
                if (destino >= 0) {
                    c->caixaAtual = destino;
                    hash_atualizar_caixa(hash, c->id, destino);
                    fila_inserir(&sm->caixas[destino].fila, c);
                    snprintf(detalhes, sizeof(detalhes),
                             "%s terminou compras e entrou na fila da caixa %d",
                             c->id, destino + 1);
                    log_acao(sm->logFile, "ENTRAR_FILA", detalhes);
                    verificar_politica_caixas(sm, hash);
                }
                /* swap-with-last para remover em O(1) */
                sm->clientesEmLoja[j] = sm->clientesEmLoja[--sm->nClientesEmLoja];
            } else {
                j++;
            }
        }

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
    int abertas = contar_caixas_realmente_abertas(sm);
    int totalFilas = total_filas_apenas_abertas(sm);
    double media;
    char detalhes[128];

    (void)hash;
    if (abertas <= 0) return;
    media = (double)totalFilas / abertas;

    if (media > sm->cfg.maxFila && sm->instanteAtual != sm->instanteUltimoFecho) {
        int i;
        for (i = 0; i < sm->cfg.nCaixas; ++i) {
            if (sm->caixas[i].estado == CAIXA_FECHADA) {
                sm->caixas[i].estado = CAIXA_ABERTA;
                snprintf(detalhes, sizeof(detalhes),
                         "Caixa %d aberta (media=%.2f > MAX_FILA=%d)",
                         i + 1, media, sm->cfg.maxFila);
                log_acao(sm->logFile, "ABRIR_CAIXA_AUTO", detalhes);
                return;
            }
        }
        return;
    }

    if (media < sm->cfg.minFila
        && abertas > 1
        && sm->instanteAtual > sm->instanteUltimaAberturaManual + 3) {
        int idx = encontrar_caixa_menos_fila_aberta(sm);
        if (idx != -1) {
            sm->caixas[idx].estado = CAIXA_A_FECHAR;
            sm->instanteUltimoFecho = sm->instanteAtual;
            snprintf(detalhes, sizeof(detalhes),
                     "Caixa %d a fechar (media=%.2f < MIN_FILA=%d)",
                     idx + 1, media, sm->cfg.minFila);
            log_acao(sm->logFile, "FECHAR_CAIXA_SUAVE_AUTO", detalhes);
        }
    }
}

int abrir_caixa_manual(Supermercado *sm, int idCaixa) {
    char detalhes[64];
    if (idCaixa < 0 || idCaixa >= sm->cfg.nCaixas) return 0;
    sm->caixas[idCaixa].estado = CAIXA_ABERTA;
    sm->instanteUltimaAberturaManual = sm->instanteAtual;
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
    if (no->idCaixa < 0) {
        printf("\nCliente %s esta a fazer compras no supermercado.\n", id);
        mostrar_cliente(no->cliente);
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
