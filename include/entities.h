#ifndef MINECRAFT_CG_ENTITIES_H
#define MINECRAFT_CG_ENTITIES_H

#include "game_types.h"
#include <stdio.h>

void criaDrop(int tipo, float x, float y, float z);
void atualizaDrops(float dt);
void desenhaDrops(void);
void iniciaEntidades(void);
void atualizaEntidades(float dt);
void desenhaEntidades(void);
int atacaInimigo(void);
int salvaEstadoEntidades(FILE *arquivo);
int carregaEstadoEntidades(FILE *arquivo);

#endif
