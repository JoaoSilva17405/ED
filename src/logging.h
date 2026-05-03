#ifndef LOGGING_H
#define LOGGING_H

#include "common.h"

FILE *abrir_log_acoes(const char *filename);
void log_acao(FILE *logFile, const char *acao, const char *detalhes);
void fechar_log(FILE *logFile);

#endif
