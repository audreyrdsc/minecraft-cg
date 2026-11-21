#include <assert.h>
#include <stdio.h>
#include "../include/chunks.h"
#include "../include/world.h"
#include "../include/worldgen.h"
#include "../include/blocks.h"

int main(void)
{
    int alturaA, biomaA, cavernaA, minerioA;
    int alturaB, biomaB, cavernaB, minerioB;
    int diferentes = 0, lagos = 0, cerejeiras = 0;
    int x, z;
    static unsigned char pixels[MAPA_RESOLUCAO * MAPA_RESOLUCAO * 3];

    /* O mapa deriva dos blocos reais e ignora objetos pequenos acima do solo. */
    colocaBloco(GRAMA, 2, 2, 3);
    colocaBloco(TOCHA, 2, 3, 3);
    assert(blocoTopoMapa(2, 3, &alturaA) == GRAMA && alturaA == 2);
    colocaBloco(AGUA, 3, 2, 3);
    colocaBloco(FOLHA_CEREJEIRA, 4, 5, 3);
    geraPixelsMapa(pixels);
    assert(pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 2 * MAPA_ESCALA) * 3 + 1] > pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 2 * MAPA_ESCALA) * 3]);
    assert(pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 3 * MAPA_ESCALA) * 3 + 2] > pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 3 * MAPA_ESCALA) * 3]);
    assert(pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 4 * MAPA_ESCALA) * 3] > pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 4 * MAPA_ESCALA) * 3 + 1]);
    colocaBloco(VIDRO, 2, 4, 3);
    assert(blocoTopoMapa(2, 3, &alturaA) == VIDRO && alturaA == 4);
    geraPixelsMapa(pixels);
    assert(pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 2 * MAPA_ESCALA) * 3 + 2] > pixels[(3 * MAPA_ESCALA * MAPA_RESOLUCAO + 2 * MAPA_ESCALA) * 3]);
    assert(blocoTopoMapa(-1, 3, &alturaA) == VAZIO && alturaA == -1);
    /* As copas estilizadas mantem a arvore real e podem revelar o chao abaixo. */
    colocaBloco(GRAMA, 10, 0, 10);
    colocaBloco(TRONCO_CEREJEIRA, 10, 1, 10);
    colocaBloco(FOLHA_CEREJEIRA, 10, 2, 10);
    colocaBloco(GRAMA, 12, 0, 10);
    colocaBloco(FOLHA_CEREJEIRA, 12, 2, 10);
    geraPixelsMapa(pixels);
    {
        int verdes = 0, rosados = 0, sx, sz;
        for (sz = 0; sz < MAPA_ESCALA; sz++)
            for (sx = 0; sx < MAPA_ESCALA; sx++) {
                size_t a = ((size_t)(10 * MAPA_ESCALA + sz) * MAPA_RESOLUCAO +
                            10 * MAPA_ESCALA + sx) * 3u;
                size_t b = ((size_t)(10 * MAPA_ESCALA + sz) * MAPA_RESOLUCAO +
                            12 * MAPA_ESCALA + sx) * 3u;
                if (pixels[a] > pixels[a + 1]) rosados++;
                if (pixels[b + 1] > pixels[b]) verdes++;
            }
        assert(rosados > 0 && verdes > 0);
        assert(blocoNoMundo(10, 1, 10) == TRONCO_CEREJEIRA);
    }

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
    for (x = 64; x < LADO; x++)
        for (z = 0; z < LADO; z++) {
            if (alturaProcedural(x, z) == 0) lagos++;
            if (biomaProcedural(x, z) == 4) cerejeiras++;
        }
    assert(lagos > 0 && cerejeiras > 0);
    assert(LADO == 128 && NUM_CHUNKS_X == 8 && NUM_CHUNKS_Z == 8);
    assert(biomaProcedural(26, 7) == 4);
    assert(biomaProcedural(30, 42) == 0 && alturaProcedural(30, 42) == 3);
    /* Uma secao de agua a cada coluna demonstra continuidade do rio. */
    for (x = 16; x < LADO; x++) {
        int agua = 0;
        for (z = 22; z <= 35; z++) if (alturaProcedural(x, z) == 0) agua++;
        assert(agua >= 5);
    }

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
    assert(dentroDoMundo(127, ALTURA_MUNDO - 1, 127));
    assert(!dentroDoMundo(128, 0, 0));

    puts("worldgen_chunks_test: OK");
    return 0;
}
