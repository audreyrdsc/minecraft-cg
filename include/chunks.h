#ifndef MINECRAFT_CG_CHUNKS_H
#define MINECRAFT_CG_CHUNKS_H

#include "config.h"

typedef struct {
    int x, z;
    int carregado;
    int sujo;
} ChunkInfo;

void inicializaChunks(void);
void atualizaChunks(float jogadorX, float jogadorZ, float distanciaCarga);
void invalidaCacheFaces(int x, int y, int z);
void invalidaCacheFacesTodos(void);
int cacheFacesValido(int x, int y, int z);
int obtemFacesCache(int x, int y, int z);
void gravaFacesCache(int x, int y, int z, int faces);
void marcaChunkSujo(int x, int z);
void marcaTodosChunksSujo(void);
int chunkEstaSujo(int chunkX, int chunkZ);
void limpaMarcaChunk(int chunkX, int chunkZ);
const ChunkInfo *obtemChunk(int chunkX, int chunkZ);
int chunkVisivel(int chunkX, int chunkZ, float olhoX, float olhoZ,
                 float distancia);

#endif
