#ifndef MINECRAFT_CG_WORLDGEN_H
#define MINECRAFT_CG_WORLDGEN_H

extern unsigned int sementeMundo;

void defineSementeMundo(unsigned int semente);
/* Exclusivo da comparacao segura com mundos gerados pela versao 6. */
void defineGeracaoLegada(int ativa);
int alturaProcedural(int x, int z);
int biomaProcedural(int x, int z);
int cavernaProcedural(int x, int y, int z);
int minerioProcedural(int x, int y, int z);

#endif
