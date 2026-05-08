#ifndef SIMULATION_H
#define SIMULATION_H

#include "config.h"

typedef enum { SIM_PAUSADA, SIM_A_CORRER } EstadoSimulacao;

const char* sim_nome_fluxo(int intervaloChegada);
void        sim_mostrar_status_bar(EstadoSimulacao estado, int instante, int intervaloChegada);
int         sim_alterar_fluxo(Configuracao *cfg, int novoIntervalo);

#endif
