#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logging.h"

static int pass_count = 0, fail_count = 0;

#define CHECK(cond, desc) do { \
    if (cond) { printf("PASS: %s\n", desc); pass_count++; } \
    else       { printf("FAIL: %s (linha %d)\n", desc, __LINE__); fail_count++; } \
} while(0)

#define TMP_LOG17 "output/test17_log.csv"

static int count_line_occurrences(const char *filename, const char *substring) {
    char line[512];
    int count = 0;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, substring)) count++;
    }
    fclose(f);
    return count;
}

static int file_contains(const char *filename, const char *substring) {
    return count_line_occurrences(filename, substring) > 0;
}

static int count_total_lines(const char *filename) {
    char line[512];
    int count = 0;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    while (fgets(line, sizeof(line), f)) count++;
    fclose(f);
    return count;
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 1 — segunda sessao nao duplica o cabecalho
   ══════════════════════════════════════════════════════════════════════════ */

static void test_cabecalho_unico_em_multiplas_sessoes(void) {
    FILE *log1, *log2;

    remove(TMP_LOG17);

    /* sessao 1: ficheiro criado, cabecalho escrito */
    log1 = abrir_log_acoes(TMP_LOG17);
    CHECK(log1 != NULL, "sessao1: abrir_log_acoes retorna FILE valido");
    if (log1) {
        log_acao(log1, "TESTE_S1", "entrada da sessao 1");
        fechar_log(log1);
    }

    /* sessao 2: ficheiro ja existe, deve fazer append sem novo cabecalho */
    log2 = abrir_log_acoes(TMP_LOG17);
    CHECK(log2 != NULL, "sessao2: abrir_log_acoes retorna FILE valido");
    if (log2) {
        log_acao(log2, "TESTE_S2", "entrada da sessao 2");
        fechar_log(log2);
    }

    CHECK(count_line_occurrences(TMP_LOG17, "timestamp,acao,detalhes") == 1,
          "append: cabecalho aparece exatamente 1 vez apos 2 sessoes");
    CHECK(file_contains(TMP_LOG17, "TESTE_S1"),
          "append: entrada da sessao 1 presente no ficheiro final");
    CHECK(file_contains(TMP_LOG17, "TESTE_S2"),
          "append: entrada da sessao 2 presente no ficheiro final");
    /* cabecalho + 2 entradas = 3 linhas */
    CHECK(count_total_lines(TMP_LOG17) == 3,
          "append: ficheiro tem exatamente 3 linhas (cabecalho + 2 entradas)");

    remove(TMP_LOG17);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 2 — ficheiro novo cria cabecalho; ficheiro existente nao cria
   ══════════════════════════════════════════════════════════════════════════ */

static void test_ficheiro_novo_tem_cabecalho(void) {
    FILE *log;
    remove(TMP_LOG17);

    log = abrir_log_acoes(TMP_LOG17);
    CHECK(log != NULL, "ficheiro novo: abrir retorna FILE valido");
    if (log) fechar_log(log);

    CHECK(file_contains(TMP_LOG17, "timestamp,acao,detalhes"),
          "ficheiro novo: cabecalho escrito na criacao");

    remove(TMP_LOG17);
}

static void test_ficheiro_existente_sem_cabecalho_extra(void) {
    FILE *log;
    remove(TMP_LOG17);

    /* criar ficheiro com conteudo pre-existente */
    log = fopen(TMP_LOG17, "w");
    if (!log) { fail_count++; printf("FAIL: nao foi possivel criar ficheiro temporario\n"); return; }
    fprintf(log, "timestamp,acao,detalhes\n");
    fprintf(log, "2025-01-01 00:00:00,SESSAO_ANTIGA,detalhes antigos\n");
    fclose(log);

    /* abrir_log_acoes num ficheiro existente deve fazer append, sem cabecalho */
    log = abrir_log_acoes(TMP_LOG17);
    CHECK(log != NULL, "ficheiro existente: abrir retorna FILE valido");
    if (log) {
        log_acao(log, "NOVO_EVENTO", "evento novo");
        fechar_log(log);
    }

    CHECK(count_line_occurrences(TMP_LOG17, "timestamp,acao,detalhes") == 1,
          "ficheiro existente: cabecalho continua a aparecer apenas 1 vez");
    CHECK(file_contains(TMP_LOG17, "SESSAO_ANTIGA"),
          "ficheiro existente: entrada antiga preservada");
    CHECK(file_contains(TMP_LOG17, "NOVO_EVENTO"),
          "ficheiro existente: entrada nova adicionada por append");

    remove(TMP_LOG17);
}

/* ══════════════════════════════════════════════════════════════════════════
   Cycle 3 — formato de log_acao compativel com INICIO_SESSAO / FIM_SESSAO
   ══════════════════════════════════════════════════════════════════════════ */

static void test_formato_inicio_fim_sessao(void) {
    FILE *log;
    remove(TMP_LOG17);

    log = abrir_log_acoes(TMP_LOG17);
    if (!log) { fail_count++; printf("FAIL: nao foi possivel abrir log\n"); return; }

    log_acao(log, "INICIO_SESSAO", "snapshot=data/Dados.txt instante=0");
    log_acao(log, "FIM_SESSAO", "clientes_atendidos=5 instante_final=300");
    fechar_log(log);

    CHECK(file_contains(TMP_LOG17, "INICIO_SESSAO"),
          "formato: INICIO_SESSAO escrito no ficheiro");
    CHECK(file_contains(TMP_LOG17, "FIM_SESSAO"),
          "formato: FIM_SESSAO escrito no ficheiro");
    CHECK(file_contains(TMP_LOG17, "snapshot=data/Dados.txt"),
          "formato: detalhe snapshot presente na linha INICIO_SESSAO");
    CHECK(file_contains(TMP_LOG17, "clientes_atendidos=5"),
          "formato: detalhe clientes_atendidos presente na linha FIM_SESSAO");

    remove(TMP_LOG17);
}

/* ── runner ─────────────────────────────────────────────────────────────── */
int main(void) {
    printf("=== Issue #17: Persistencia do Log de Utilizador ===\n\n");

    printf("-- Cycle 1: append entre sessoes --\n");
    test_cabecalho_unico_em_multiplas_sessoes();

    printf("\n-- Cycle 2: cabecalho apenas em ficheiro novo --\n");
    test_ficheiro_novo_tem_cabecalho();
    test_ficheiro_existente_sem_cabecalho_extra();

    printf("\n-- Cycle 3: formato INICIO_SESSAO / FIM_SESSAO --\n");
    test_formato_inicio_fim_sessao();

    printf("\n=== Resultados: %d PASS, %d FAIL ===\n", pass_count, fail_count);
    return fail_count > 0 ? 1 : 0;
}
