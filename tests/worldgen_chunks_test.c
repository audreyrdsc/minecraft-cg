#include <assert.h>
#include <stdio.h>
#include "../include/chunks.h"
#include "../include/world.h"
#include "../include/worldgen.h"

int main(void)
{
    int alturaA, biomaA, cavernaA, minerioA;
    int alturaB, biomaB, cavernaB, minerioB;
    int diferentes = 0;
    int x, z;

    defineSementeMundo(12345u);
    alturaA = alturaProcedural(23, 41);
    biomaA = biomaProcedural(23, 41);
    cavernaA = cavernaProcedural(23, 4, 41);
    minerioA = minerioProcedural(23, 3, 41);

    defineSementeMundo(12345u);
    alturaB = alturaProcedural(23, 41);
    biomaB = biomaProcedural(23, 41);
    cavernaB = cavernaProcedural(23, 4, 41);
    minerioB = minerioProcedural(23, 3, 41);
    assert(alturaA == alturaB && biomaA == biomaB &&
           cavernaA == cavernaB && minerioA == minerioB);

    defineSementeMundo(98765u);
    for (x = 0; x < 8; x++)
        for (z = 0; z < 8; z++)
            if (alturaProcedural(x * 3, z * 3) != alturaA)
                diferentes++;
    assert(diferentes > 0);

    inicializaChunks();
    assert(obtemChunk(0, 0) != 0);
    assert(obtemChunk(NUM_CHUNKS_X - 1, NUM_CHUNKS_Z - 1) != 0);
    assert(chunkEstaSujo(0, 0));
    limpaMarcaChunk(0, 0);
    assert(!chunkEstaSujo(0, 0));
    marcaChunkSujo(CHUNK_LADO + 1, CHUNK_LADO + 1);
    assert(chunkEstaSujo(1, 1));
    assert(chunkVisivel(0, 0, 4.0f, 4.0f, 24.0f));
    limpaMarcaChunk(0, 0);
    invalidaCacheFaces(0, 0, 0);
    assert(chunkEstaSujo(0, 0));
    limpaMarcaChunk(0, 0);
    marcaChunkSujo(-1, 0);
    marcaChunkSujo(0, -1);
    assert(!chunkEstaSujo(0, 0));
    assert(obtemFacesCache(-1, 0, 0) == 0);
    assert(dentroDoMundo(63, ALTURA_MUNDO - 1, 63));
    assert(!dentroDoMundo(64, 0, 0));

    puts("worldgen_chunks_test: OK");
    return 0;
}
