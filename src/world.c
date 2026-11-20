#include "../include/world.h"
#include "../include/chunks.h"

#include "../include/config.h"
#ifndef VAZIO
#define VAZIO 0
#endif

unsigned char mundo[LADO][ALTURA_MUNDO][LADO];

int dentroDoMundo(int x, int y, int z)
{
    return x >= 0 && x < LADO &&
           y >= 0 && y < ALTURA_MUNDO &&
           z >= 0 && z < LADO;
}

void colocaBloco(int tipo, int x, int y, int z)
{
    if (dentroDoMundo(x, y, z)) {
        mundo[x][y][z] = (unsigned char)tipo;
        invalidaCacheFaces(x, y, z);
        marcaChunkSujo(x, z);
    }
}

int blocoNoMundo(int x, int y, int z)
{
    if (!dentroDoMundo(x, y, z)) return VAZIO;
    return mundo[x][y][z];
}
