#ifndef UTILS_H
#define UTILS_H

#include "common.h"

void trim(char *str);
int ler_int(const char *prompt);
void ler_string(const char *prompt, char *buffer, size_t size);
int str_casecmp_local(const char *a, const char *b);

#endif
