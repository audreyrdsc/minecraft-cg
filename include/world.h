#ifndef MINECRAFT_CG_WORLD_H
#define MINECRAFT_CG_WORLD_H

#include "game_types.h"
#include "chunks.h"

#include "config.h"

extern unsigned char mundo[LADO][ALTURA_MUNDO][LADO];

int dentroDoMundo(int x, int y, int z);
void colocaBloco(int tipo, int x, int y, int z);
int blocoNoMundo(int x, int y, int z);

#endif
