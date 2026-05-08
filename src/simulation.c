#include "simulation.h"
#include "store.h"
#include <stdio.h>

const char* sim_nome_fluxo(int intervaloChegada) {
    if (intervaloChegada == FLUXO_CALMO)        return "CALMO";
    if (intervaloChegada == FLUXO_NORMAL)       return "NORMAL";
    if (intervaloChegada == FLUXO_HORA_DE_PONTA) return "HORA DE PONTA";
    return "CUSTOM";
}

int sim_alterar_fluxo(Configuracao *cfg, int novoIntervalo) {
    if (novoIntervalo <= 0) return EVT_NENHUM;
    cfg->intervaloChegada = novoIntervalo;
    return EVT_FLUXO_ALTERADO;
}

void sim_mostrar_status_bar(EstadoSimulacao estado, int instante, int intervaloChegada) {
    if (estado == SIM_A_CORRER) {
        printf("[A CORRER | Instante: %d | Fluxo: %s | Prima qualquer tecla para pausar]\n",
               instante, sim_nome_fluxo(intervaloChegada));
    } else {
        printf("[PAUSADA | Instante: %d | Fluxo: %s]\n",
               instante, sim_nome_fluxo(intervaloChegada));
    }
}
