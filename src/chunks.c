#include "../include/chunks.h"
#include <math.h>

static ChunkInfo chunks[NUM_CHUNKS_X][NUM_CHUNKS_Z];
static unsigned char facesValidas[LADO][ALTURA_MUNDO][LADO];
static unsigned char facesCache[LADO][ALTURA_MUNDO][LADO];

static void invalidaFace(int x, int y, int z)
{
    if (x >= 0 && x < LADO && y >= 0 && y < ALTURA_MUNDO &&
        z >= 0 && z < LADO)
        facesValidas[x][y][z] = 0;
}

void inicializaChunks(void)
{
    int x, z;
    for (x = 0; x < NUM_CHUNKS_X; x++)
        for (z = 0; z < NUM_CHUNKS_Z; z++) {
            chunks[x][z].x = x;
            chunks[x][z].z = z;
            chunks[x][z].carregado = 1;
            chunks[x][z].sujo = 1;
        }
    invalidaCacheFacesTodos();
}

void invalidaCacheFaces(int x, int y, int z)
{
    invalidaFace(x, y, z);
    invalidaFace(x + 1, y, z);
    invalidaFace(x - 1, y, z);
    invalidaFace(x, y + 1, z);
    invalidaFace(x, y - 1, z);
    invalidaFace(x, y, z + 1);
    invalidaFace(x, y, z - 1);
    marcaChunkSujo(x, z);
    marcaChunkSujo(x + 1, z);
    marcaChunkSujo(x - 1, z);
    marcaChunkSujo(x, z + 1);
    marcaChunkSujo(x, z - 1);
}

void invalidaCacheFacesTodos(void)
{
    int x, y, z;
    for (x = 0; x < LADO; x++)
        for (y = 0; y < ALTURA_MUNDO; y++)
            for (z = 0; z < LADO; z++)
                facesValidas[x][y][z] = 0;
}

int cacheFacesValido(int x, int y, int z)
{
    if (x < 0 || x >= LADO || y < 0 || y >= ALTURA_MUNDO ||
        z < 0 || z >= LADO)
        return 0;
    return facesValidas[x][y][z] != 0;
}

int obtemFacesCache(int x, int y, int z)
{
    if (x < 0 || x >= LADO || y < 0 || y >= ALTURA_MUNDO ||
        z < 0 || z >= LADO)
        return 0;
    return facesCache[x][y][z];
}

void gravaFacesCache(int x, int y, int z, int faces)
{
    if (x < 0 || x >= LADO || y < 0 || y >= ALTURA_MUNDO ||
        z < 0 || z >= LADO)
        return;
    facesCache[x][y][z] = (unsigned char)faces;
    facesValidas[x][y][z] = 1;
}

void atualizaChunks(float jogadorX, float jogadorZ, float distanciaCarga)
{
    int x, z;
    for (x = 0; x < NUM_CHUNKS_X; x++)
        for (z = 0; z < NUM_CHUNKS_Z; z++) {
            float centroX = x * CHUNK_LADO + CHUNK_LADO * 0.5f;
            float centroZ = z * CHUNK_LADO + CHUNK_LADO * 0.5f;
            float dx = centroX - jogadorX;
            float dz = centroZ - jogadorZ;
            int carregado = dx * dx + dz * dz <=
                            distanciaCarga * distanciaCarga;
            if (x == (int)floorf(jogadorX / CHUNK_LADO) &&
                z == (int)floorf(jogadorZ / CHUNK_LADO))
                carregado = 1;
            if (chunks[x][z].carregado != carregado) {
                chunks[x][z].carregado = carregado;
                chunks[x][z].sujo = 1;
            }
        }
}

void marcaChunkSujo(int x, int z)
{
    int chunkX, chunkZ;

    /* A divisão inteira de C trunca em direção a zero; sem esta
       validação, x == -1 acabaria apontando para o chunk 0. */
    if (x < 0 || x >= LADO || z < 0 || z >= LADO)
        return;
    chunkX = x / CHUNK_LADO;
    chunkZ = z / CHUNK_LADO;
    if (chunkX >= 0 && chunkX < NUM_CHUNKS_X &&
        chunkZ >= 0 && chunkZ < NUM_CHUNKS_Z)
        chunks[chunkX][chunkZ].sujo = 1;
}

void marcaTodosChunksSujo(void)
{
    int x, z;
    for (x = 0; x < NUM_CHUNKS_X; x++)
        for (z = 0; z < NUM_CHUNKS_Z; z++)
            chunks[x][z].sujo = 1;
}

int chunkEstaSujo(int chunkX, int chunkZ)
{
    if (chunkX < 0 || chunkX >= NUM_CHUNKS_X ||
        chunkZ < 0 || chunkZ >= NUM_CHUNKS_Z)
        return 0;
    return chunks[chunkX][chunkZ].sujo;
}

void limpaMarcaChunk(int chunkX, int chunkZ)
{
    if (chunkX >= 0 && chunkX < NUM_CHUNKS_X &&
        chunkZ >= 0 && chunkZ < NUM_CHUNKS_Z)
        chunks[chunkX][chunkZ].sujo = 0;
}

const ChunkInfo *obtemChunk(int chunkX, int chunkZ)
{
    if (chunkX < 0 || chunkX >= NUM_CHUNKS_X ||
        chunkZ < 0 || chunkZ >= NUM_CHUNKS_Z)
        return 0;
    return &chunks[chunkX][chunkZ];
}

int chunkVisivel(int chunkX, int chunkZ, float olhoX, float olhoZ,
                 float distancia)
{
    float minimoX, maximoX, minimoZ, maximoZ;
    float dx = 0.0f, dz = 0.0f;

    if (chunkX < 0 || chunkX >= NUM_CHUNKS_X ||
        chunkZ < 0 || chunkZ >= NUM_CHUNKS_Z)
        return 0;
    if (!chunks[chunkX][chunkZ].carregado)
        return 0;

    minimoX = (float)(chunkX * CHUNK_LADO);
    maximoX = minimoX + CHUNK_LADO;
    minimoZ = (float)(chunkZ * CHUNK_LADO);
    maximoZ = minimoZ + CHUNK_LADO;

    if (olhoX < minimoX) dx = minimoX - olhoX;
    else if (olhoX > maximoX) dx = olhoX - maximoX;
    if (olhoZ < minimoZ) dz = minimoZ - olhoZ;
    else if (olhoZ > maximoZ) dz = olhoZ - maximoZ;

    return dx * dx + dz * dz <= distancia * distancia;
}
