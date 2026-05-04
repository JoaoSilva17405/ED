#ifndef STORE_H
#define STORE_H

#include "common.h"
#include "config.h"
#include "register.h"
#include "hash.h"
#include "logging.h"
#include "employee.h"
#include "catalog.h"
#include "client_registry.h"

#define CONFIG_FILE "data/Configuracao.txt"
#define DATA_FILE "data/Dados.txt"
#define FUNCIONARIOS_FILE "data/funcionarios.txt"
#define CLIENTES_FILE     "Clientes.txt"
#define PRODUTOS_FILE     "Produtos.txt"
#define LOG_FILE "output/historico_utilizador.csv"
#define ESTATISTICAS_FILE "output/estatisticas.csv"
#define HISTORICO_CAIXAS_FILE "output/historico_caixas.csv"

#define INSERIR_CLIENTE_DUPLICADO -1
#define INSERIR_CLIENTE_INVALIDO -2
#define INSERIR_CLIENTE_SEM_CAIXA -3
#define INSERIR_CLIENTE_MEMORIA -4

typedef struct {
    Configuracao cfg;
    Caixa *caixas;
    CatalogoProdutos *catalogo;
    int instanteAtual;
    int totalClientesAtendidos;
    int totalProdutosVendidos;
    int totalProdutosOferecidos;
    float totalValorOferecido;
    long somaTemposEspera;
    FILE *logFile;
    int instanteUltimoFecho;
    int instanteUltimaAberturaManual;
} Supermercado;

int supermercado_init(Supermercado *sm, const Configuracao *cfg, FILE *logFile, ListaFuncionarios *funcionarios, CatalogoProdutos *catalogo);
void supermercado_destruir(Supermercado *sm, HashClientes *hash);
int carregar_dados_iniciais(const char *filename, Supermercado *sm, HashClientes *hash);
void mostrar_estado_supermercado(const Supermercado *sm);
void mostrar_caixa(const Caixa *caixa);
int inserir_novo_cliente(Supermercado *sm, HashClientes *hash, const char *id, const char *nome, Produto *produtos, int nProdutos);
int mover_cliente_caixa(Supermercado *sm, HashClientes *hash, const char *id, int novaCaixa);
void avancar_simulacao(Supermercado *sm, HashClientes *hash, int passos);
void verificar_politica_caixas(Supermercado *sm, HashClientes *hash);
int abrir_caixa_manual(Supermercado *sm, int idCaixa);
int fechar_caixa_suave_manual(Supermercado *sm, int idCaixa);
int fechar_caixa_imediata_manual(Supermercado *sm, HashClientes *hash, int idCaixa);
void pesquisar_cliente(Supermercado *sm, HashClientes *hash, const char *id);
size_t memoria_utilizada(const Supermercado *sm, const HashClientes *hash);
size_t memoria_desperdicada(const Supermercado *sm, const HashClientes *hash);
int guardar_snapshot(const char *filename, const Supermercado *sm, const RegistoClientes *registo);
int guardar_estatisticas_csv(const char *filename, const Supermercado *sm);
int guardar_historico_caixas_csv(const char *filename, const Supermercado *sm);
void mostrar_estatisticas_finais(const Supermercado *sm);

#endif
