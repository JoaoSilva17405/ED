#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_NOME 64
#define MAX_ID 32
#define MAX_OPERADOR 64
#define HASH_SIZE 101
#define HIST_GROWTH 16
#define MAX_LINE 512

typedef enum { false = 0, true = 1 } bool;

typedef enum {
    CAIXA_FECHADA = 0,
    CAIXA_ABERTA = 1,
    CAIXA_A_FECHAR = 2
} EstadoCaixa;

#endif
