#include "../include/world.h"
#include "../include/chunks.h"
#include "../include/blocks.h"
#include "../include/worldgen.h"
#include <stddef.h>

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

int blocoTopoMapa(int x, int z, int *altura)
{
    int y;
    if (altura) *altura = -1;
    if (x < 0 || x >= LADO || z < 0 || z >= LADO) return VAZIO;
    for (y = ALTURA_MUNDO - 1; y >= 0; y--) {
        int tipo = mundo[x][y][z];
        /* Objetos pequenos e efemeros nao escondem o terreno no mapa. */
        if (tipo == VAZIO || tipo == TOCHA || tipo == NUVEM) continue;
        if (altura) *altura = y;
        return tipo;
    }
    return VAZIO;
}

static unsigned int ruidoMapa(unsigned int x, unsigned int z)
{
    unsigned int h = x * 374761393u ^ z * 668265263u ^ sementeMundo;
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    return h ^ (h >> 16);
}

static unsigned char limitaCorMapa(int cor)
{
    if (cor < 0) return 0;
    if (cor > 255) return 255;
    return (unsigned char)cor;
}

static void corBaseMapa(int tipo, int *r, int *g, int *b)
{
    switch (tipo) {
        case GRAMA: *r = 76; *g = 125; *b = 40; break;
        case TERRA: *r = 125; *g = 89; *b = 57; break;
        case PEDRA: case MINERIO_CARVAO: case MINERIO_FERRO:
            *r = 115; *g = 115; *b = 110; break;
        case AREIA: *r = 208; *g = 193; *b = 137; break;
        case AGUA: *r = 38; *g = 81; *b = 169; break;
        case NEVE: *r = 213; *g = 222; *b = 224; break;
        case FOLHA: *r = 44; *g = 109; *b = 34; break;
        case FOLHA_CEREJEIRA: *r = 200; *g = 145; *b = 171; break;
        case TRONCO: *r = 119; *g = 79; *b = 45; break;
        case TRONCO_CEREJEIRA: *r = 76; *g = 48; *b = 53; break;
        case VIDRO: *r = 150; *g = 184; *b = 190; break;
        case CRISTAL: *r = 42; *g = 194; *b = 208; break;
        default: *r = 199; *g = 178; *b = 139; break;
    }
}

void geraPixelsMapa(unsigned char *pixels)
{
    static unsigned char tipos[LADO * LADO];
    static signed char alturas[LADO * LADO];
    static short centroX[LADO * LADO], centroZ[LADO * LADO];
    static unsigned char fundo[LADO * LADO];
    static signed char alturaFundo[LADO * LADO];
    int x, z, sx, sz;
    if (!pixels) return;

    for (z = 0; z < LADO; z++) for (x = 0; x < LADO; x++) {
        int y, indice = z * LADO + x;
        tipos[indice] = (unsigned char)blocoTopoMapa(x, z, &y);
        alturas[indice] = (signed char)y;
        centroX[indice] = centroZ[indice] = -1;
        fundo[indice] = VAZIO;
        alturaFundo[indice] = -1;
        for (y = ALTURA_MUNDO - 1; y >= 0; y--) {
            int bloco = mundo[x][y][z];
            if (bloco == VAZIO || bloco == TOCHA || bloco == NUVEM ||
                bloco == FOLHA || bloco == FOLHA_CEREJEIRA ||
                bloco == TRONCO || bloco == TRONCO_CEREJEIRA) continue;
            fundo[indice] = (unsigned char)bloco;
            alturaFundo[indice] = (signed char)y;
            break;
        }
    }
    /* Localiza troncos reais sob as copas para revelar arvores individuais,
       mesmo quando copas adjacentes se tocam. Nao cria arvores no mapa. */
    for (z = 0; z < LADO; z++) for (x = 0; x < LADO; x++) {
        int indice = z * LADO + x, tipo = tipos[indice], melhor = 1000, dx, dz;
        int tronco = tipo == FOLHA ? TRONCO : TRONCO_CEREJEIRA;
        if (tipo != FOLHA && tipo != FOLHA_CEREJEIRA) continue;
        for (dz = -3; dz <= 3; dz++) for (dx = -3; dx <= 3; dx++) {
            int tx = x + dx, tz = z + dz, yy, dist = dx * dx + dz * dz;
            if (tx < 0 || tx >= LADO || tz < 0 || tz >= LADO || dist >= melhor) continue;
            for (yy = alturas[indice]; yy >= 0 && yy >= alturas[indice] - 8; yy--)
                if (mundo[tx][yy][tz] == tronco) {
                    melhor = dist; centroX[indice] = (short)tx; centroZ[indice] = (short)tz;
                    break;
                }
        }
    }
    for (z = 0; z < LADO; z++) for (x = 0; x < LADO; x++) {
        int indice = z * LADO + x, y = alturas[indice], tipo = tipos[indice];
        int r, g, b, fundoR, fundoG, fundoB, relevo = 0;
        unsigned int h = (unsigned int)x * 374761393u ^ (unsigned int)z * 668265263u;
        corBaseMapa(tipo, &r, &g, &b);
        corBaseMapa(fundo[indice], &fundoR, &fundoG, &fundoB);
        if (z > 0 && y > alturas[indice - LADO]) relevo += 9;
        if (x > 0 && y < alturas[indice - 1]) relevo -= 7;
        for (sz = 0; sz < MAPA_ESCALA; sz++) for (sx = 0; sx < MAPA_ESCALA; sx++) {
            unsigned int hash = ruidoMapa((unsigned int)(x * MAPA_ESCALA + sx),
                                          (unsigned int)(z * MAPA_ESCALA + sz));
            int detalhe = tipo == VAZIO ? 0 : (int)(hash % 13u) - 6 + relevo;
            int corR = r, corG = g, corB = b, tipoDesenhado = tipo, yDesenhado = y;
            int px = x * MAPA_ESCALA + sx, pz = z * MAPA_ESCALA + sz;
            size_t destino = ((size_t)pz * MAPA_RESOLUCAO + px) * 3u;
            if ((tipo == FOLHA || tipo == FOLHA_CEREJEIRA) && centroX[indice] >= 0) {
                int dx = (x - centroX[indice]) * MAPA_ESCALA + sx - MAPA_ESCALA / 2;
                int dz = (z - centroZ[indice]) * MAPA_ESCALA + sz - MAPA_ESCALA / 2;
                unsigned int arvore = ruidoMapa((unsigned int)centroX[indice],
                                               (unsigned int)centroZ[indice]);
                unsigned int manchas = ruidoMapa((unsigned int)(px / 2),
                                                 (unsigned int)(pz / 2));
                int limiar = 55 + (int)(arvore % 41u) - 20 + (int)(manchas % 21u) - 10;
                /* Deslocamento menor que um bloco, apenas na estilização da copa. */
                dx += (int)((arvore >> 13) % 5u) - 2;
                dz += (int)((arvore >> 17) % 5u) - 2;
                int dist = dx * dx + dz * dz;
                /* Copas simbolicas centram-se nos troncos existentes. Entre
                   elas aparece o terreno verdadeiro abaixo das folhas. */
                if (dist > limiar && fundo[indice] != VAZIO) {
                    corR = fundoR; corG = fundoG; corB = fundoB;
                    tipoDesenhado = fundo[indice]; yDesenhado = alturaFundo[indice];
                    detalhe = (int)(hash % 13u) - 6;
                } else {
                    detalhe += (int)((arvore >> 8) % 29u) - 14;
                    detalhe += (int)(manchas % 25u) - 12;
                    detalhe += dist > limiar - 18 ? -22 : 9;
                }
            }
            /* Apenas a vegetacao que realmente aparece no mundo recebe tufos. */
            if (tipoDesenhado == GRAMA && yDesenhado + 1 < ALTURA_MUNDO &&
                mundo[x][yDesenhado + 1][z] == VAZIO &&
                ((h ^ sementeMundo) % 5u) == 0 &&
                ((sx == 1 && sz <= 2) || (sx == 2 && sz >= 1))) detalhe -= 35;
            pixels[destino] = limitaCorMapa(corR + detalhe);
            pixels[destino + 1] = limitaCorMapa(corG + detalhe);
            pixels[destino + 2] = limitaCorMapa(corB + detalhe);
        }
    }
}
