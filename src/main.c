#include "config.h"
#include "store.h"
#include "hash.h"
#include "utils.h"
#include "client_registry.h"

static void menu_principal(void) {
    printf("\n================ COMPRA AQUI LDA. ================\n");
    printf("1. Mostrar configuracao\n");
    printf("2. Mostrar estado das caixas\n");
    printf("3. Avancar simulacao\n");
    printf("4. Inserir novo cliente\n");
    printf("5. Mudar cliente de caixa\n");
    printf("6. Abrir caixa manualmente\n");
    printf("7. Fechar caixa suavemente\n");
    printf("8. Fechar caixa imediatamente e redistribuir\n");
    printf("9. Pesquisar cliente\n");
    printf("10. Mostrar memoria utilizada\n");
    printf("11. Mostrar memoria desperdicada\n");
    printf("12. Mostrar estatisticas finais\n");
    printf("13. Gravar estatisticas CSV\n");
    printf("0. Sair\n");
}

int main(void) {
    Configuracao cfg;
    Supermercado sm;
    HashClientes hash;
    ListaFuncionarios *funcionarios;
    RegistoClientes *registo;
    FILE *logFile;
    int opcao;

    srand((unsigned int)time(NULL));

    if (!carregar_configuracao(CONFIG_FILE, &cfg)) {
        printf("Erro ao abrir %s\n", CONFIG_FILE);
        printf("Verifique se a pasta data existe no diretorio de execucao.\n");
        return 1;
    }

    funcionarios = funcionarios_carregar(FUNCIONARIOS_FILE);
    if (!funcionarios) {
        printf("Aviso: nao foi possivel carregar %s — operadores genericos serao usados.\n", FUNCIONARIOS_FILE);
    }

    registo = registo_carregar(CLIENTES_FILE);
    if (!registo) {
        printf("Aviso: nao foi possivel carregar %s — IDs manuais serao usados.\n", CLIENTES_FILE);
    } else {
        printf("Registo de clientes carregado: %d clientes.\n", registo->tamanho);
    }

    hash_init(&hash);
    logFile = abrir_log_acoes(LOG_FILE);
    if (!logFile) {
        printf("Aviso: nao foi possivel criar %s\n", LOG_FILE);
    }
    if (!supermercado_init(&sm, &cfg, logFile, funcionarios)) {
        printf("Erro: nao foi possivel inicializar as estruturas do supermercado.\n");
        funcionarios_destruir(funcionarios);
        fechar_log(logFile);
        return 1;
    }

    if (!carregar_dados_iniciais(DATA_FILE, &sm, &hash)) {
        printf("Erro ao abrir %s\n", DATA_FILE);
        printf("Verifique se a pasta data existe no diretorio de execucao.\n");
        supermercado_destruir(&sm, &hash);
        fechar_log(logFile);
        return 1;
    }

    do {
        menu_principal();
        opcao = ler_int("Opcao: ");
        switch (opcao) {
            case 1:
                log_acao(logFile, "MENU", "Mostrar configuracao");
                mostrar_configuracao(&cfg);
                break;
            case 2:
                log_acao(logFile, "MENU", "Mostrar estado das caixas");
                mostrar_estado_supermercado(&sm);
                break;
            case 3: {
                int passos = ler_int("Quantos instantes pretende avancar? ");
                if (passos <= 0) {
                    printf("Numero de instantes invalido: indique um valor maior que zero.\n");
                    break;
                }
                log_acao(logFile, "MENU", "Avancar simulacao");
                avancar_simulacao(&sm, &hash, passos);
                break;
            }
            case 4: {
                char id[MAX_ID];
                char nome[MAX_NOME];
                NoHash *existente;
                log_acao(logFile, "MENU", "Inserir novo cliente");
                ler_string("ID do cliente: ", id, sizeof(id));
                existente = hash_pesquisar(&hash, id);
                if (existente) {
                    char resp[4];
                    printf("Cliente %s ja esta na caixa %d.\n", id, existente->idCaixa + 1);
                    printf("Pretende mover para outra caixa? (s/n): ");
                    if (fgets(resp, sizeof(resp), stdin) && (resp[0] == 's' || resp[0] == 'S')) {
                        int nova_caixa = ler_int("Nova caixa (1..N): ") - 1;
                        int r = mover_cliente_caixa(&sm, &hash, id, nova_caixa);
                        if (r == 1) printf("Cliente movido com sucesso.\n");
                        else if (r == 2) printf("Cliente ja estava nessa caixa.\n");
                        else printf("Nao foi possivel mover o cliente.\n");
                    }
                } else {
                    int nProdutos;
                    int resultado;
                    nome[0] = '\0';
                    if (registo) {
                        EntradaCliente *entrada = registo_pesquisar_id(registo, id);
                        if (entrada) strncpy(nome, entrada->nome, sizeof(nome) - 1);
                    }
                    nProdutos = ler_int("Numero de produtos: ");
                    resultado = inserir_novo_cliente(&sm, &hash, id, nome, nProdutos);
                    if (resultado == INSERIR_CLIENTE_INVALIDO) printf("Dados invalidos: indique um ID nao vazio e numero de produtos superior a zero.\n");
                    else if (resultado == INSERIR_CLIENTE_SEM_CAIXA) printf("Nao existe nenhuma caixa disponivel para receber o cliente.\n");
                    else if (resultado == INSERIR_CLIENTE_MEMORIA) printf("Nao foi possivel reservar memoria para inserir o cliente.\n");
                    else if (resultado >= 0) printf("Cliente colocado na caixa %d.\n", resultado + 1);
                    else printf("Nao foi possivel inserir cliente.\n");
                }
                break;
            }
            case 5: {
                char id[MAX_ID];
                int caixa;
                int r;
                log_acao(logFile, "MENU", "Mudar cliente de caixa");
                ler_string("ID do cliente a mover: ", id, sizeof(id));
                caixa = ler_int("Nova caixa (1..N): ") - 1;
                r = mover_cliente_caixa(&sm, &hash, id, caixa);
                if (r == 1) printf("Cliente movido com sucesso.\n");
                else if (r == 2) printf("Cliente ja estava nessa caixa.\n");
                else printf("Nao foi possivel mover o cliente.\n");
                break;
            }
            case 6: {
                int caixa = ler_int("Caixa a abrir (1..N): ") - 1;
                log_acao(logFile, "MENU", "Abrir caixa manualmente");
                printf(abrir_caixa_manual(&sm, caixa) ? "Caixa aberta.\n" : "Nao foi possivel abrir a caixa.\n");
                break;
            }
            case 7: {
                int caixa = ler_int("Caixa a fechar suavemente (1..N): ") - 1;
                log_acao(logFile, "MENU", "Fechar caixa suavemente");
                printf(fechar_caixa_suave_manual(&sm, caixa) ? "Caixa marcada para fecho suave.\n" : "Nao foi possivel marcar a caixa.\n");
                break;
            }
            case 8: {
                int caixa = ler_int("Caixa a fechar imediatamente (1..N): ") - 1;
                log_acao(logFile, "MENU", "Fechar caixa imediatamente e redistribuir");
                printf(fechar_caixa_imediata_manual(&sm, &hash, caixa) ? "Caixa fechada e fila redistribuida.\n" : "Nao foi possivel fechar a caixa.\n");
                break;
            }
            case 9: {
                char id[MAX_ID];
                log_acao(logFile, "MENU", "Pesquisar cliente");
                ler_string("ID do cliente a pesquisar: ", id, sizeof(id));
                pesquisar_cliente(&sm, &hash, id);
                break;
            }
            case 10:
                log_acao(logFile, "MENU", "Mostrar memoria utilizada");
                printf("Memoria utilizada: %zu bytes\n", memoria_utilizada(&sm, &hash));
                break;
            case 11:
                log_acao(logFile, "MENU", "Mostrar memoria desperdicada");
                printf("Memoria desperdicada: %zu bytes\n", memoria_desperdicada(&sm, &hash));
                break;
            case 12:
                log_acao(logFile, "MENU", "Mostrar estatisticas finais");
                mostrar_estatisticas_finais(&sm);
                break;
            case 13:
                log_acao(logFile, "MENU", "Gravar estatisticas CSV");
                if (guardar_estatisticas_csv(ESTATISTICAS_FILE, &sm) &&
                    guardar_historico_caixas_csv(HISTORICO_CAIXAS_FILE, &sm)) {
                    printf("Ficheiros CSV gerados em output/.\n");
                } else {
                    printf("Nao foi possivel gerar os ficheiros CSV em output/.\n");
                }
                break;
            case 0:
                log_acao(logFile, "MENU", "Sair");
                break;
            default:
                printf("Opcao invalida.\n");
        }
    } while (opcao != 0);

    guardar_estatisticas_csv(ESTATISTICAS_FILE, &sm);
    guardar_historico_caixas_csv(HISTORICO_CAIXAS_FILE, &sm);
    mostrar_estatisticas_finais(&sm);
    supermercado_destruir(&sm, &hash);
    funcionarios_destruir(funcionarios);
    registo_destruir(registo);
    fechar_log(logFile);
    return 0;
}
