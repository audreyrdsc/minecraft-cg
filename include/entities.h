#ifndef MINECRAFT_CG_ENTITIES_H
#define MINECRAFT_CG_ENTITIES_H

#include "game_types.h"
#include "config.h"
#include <stdio.h>

typedef enum { ANIMAL_PORCO, ANIMAL_CACHORRO, ANIMAL_CAVALO, NUM_ANIMAIS } TipoAnimal;
int posicaoAnimalMapa(int tipo, float *x, float *z);

void criaDrop(int tipo, float x, float y, float z);
void atualizaDrops(float dt);
void desenhaDrops(void);
void iniciaEntidades(void);
void atualizaEntidades(float dt);
void desenhaEntidades(void);
int atacaInimigo(void);
int salvaEstadoEntidades(FILE *arquivo);
int carregaEstadoEntidades(FILE *arquivo);
void atualizaAnimais(float dt);
void desenhaAnimais(void);
int alternaMontaria(void);
int jogadorMontado(void);
int montariaNaAgua(void);
void moveMontaria(float dx, float dz, float dt, int saltar);
void cancelaMontaria(void);
int salvaEstadoAnimais(FILE *arquivo);
int carregaEstadoAnimais(FILE *arquivo);
void iniciaAnimais(void);
void preservaChunksEntidades(unsigned char alterado[NUM_CHUNKS_X][NUM_CHUNKS_Z]);

#endif
