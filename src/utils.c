#include "utils.h"

void trim(char *str) {
    size_t len;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
    len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

int ler_int(const char *prompt) {
    char linha[64];
    int valor;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(linha, sizeof(linha), stdin)) return 0;
        if (sscanf(linha, "%d", &valor) == 1) return valor;
        printf("Valor invalido.\n");
    }
}

void ler_string(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return;
    }
    trim(buffer);
}

int str_casecmp_local(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
