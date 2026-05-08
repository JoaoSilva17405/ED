#include "logging.h"

FILE *abrir_log_acoes(const char *filename) {
    FILE *test = fopen(filename, "r");
    if (test) {
        fclose(test);
        return fopen(filename, "a");
    }
    {
        FILE *f = fopen(filename, "w");
        if (!f) return NULL;
        fprintf(f, "timestamp,acao,detalhes\n");
        fflush(f);
        return f;
    }
}

void log_acao(FILE *logFile, const char *acao, const char *detalhes) {
    time_t agora;
    char buffer[64];
    struct tm *tmInfo;
    if (!logFile) return;
    agora = time(NULL);
    tmInfo = localtime(&agora);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tmInfo);
    fprintf(logFile, "%s,%s,%s\n", buffer, acao, detalhes ? detalhes : "");
    fflush(logFile);
}

void fechar_log(FILE *logFile) {
    if (logFile) fclose(logFile);
}
