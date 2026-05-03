#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "common.h"

typedef struct {
    int id;
    char nome[MAX_NOME];
} Funcionario;

typedef struct {
    Funcionario *lista;
    int tamanho;
    int capacidade;
} ListaFuncionarios;

ListaFuncionarios *funcionarios_carregar(const char *filename);
Funcionario *funcionarios_obter_aleatorio(const ListaFuncionarios *lista);
void funcionarios_destruir(ListaFuncionarios *lista);

#endif
