#ifndef MINECRAFT_CG_WORLDGEN_H
#define MINECRAFT_CG_WORLDGEN_H

extern unsigned int sementeMundo;

void defineSementeMundo(unsigned int semente);
int alturaProcedural(int x, int z);
int biomaProcedural(int x, int z);
int cavernaProcedural(int x, int y, int z);
int minerioProcedural(int x, int y, int z);

#endif
