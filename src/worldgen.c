#include <math.h>
#include "../include/worldgen.h"

unsigned int sementeMundo = 20261119u;
static int geracaoLegada = 0;

void defineGeracaoLegada(int ativa)
{
    geracaoLegada = ativa != 0;
}

void defineSementeMundo(unsigned int semente)
{
    sementeMundo = semente;
}

static unsigned int mistura(unsigned int valor)
{
    valor ^= valor >> 16;
    valor *= 0x7feb352du;
    valor ^= valor >> 15;
    valor *= 0x846ca68bu;
    valor ^= valor >> 16;
    return valor;
}

static float ruidoInteiro(int x, int z)
{
    unsigned int chave = sementeMundo;
    chave ^= (unsigned int)x * 374761393u;
    chave ^= (unsigned int)z * 668265263u;
    return (mistura(chave) & 0x00ffffffu) / 16777215.0f;
}

static float ruidoTresDimensoes(int x, int y, int z)
{
    unsigned int chave = sementeMundo;
    chave ^= (unsigned int)x * 374761393u;
    chave ^= (unsigned int)y * 1103515245u;
    chave ^= (unsigned int)z * 668265263u;
    return (mistura(chave) & 0x00ffffffu) / 16777215.0f;
}

int alturaProcedural(int x, int z)
{
    if (!geracaoLegada && x >= 16) {
        /* Um rio continuo, com margens graduais, atravessa a expansao. */
        float centro = 28.0f + 4.0f * sinf((x - 16) * 0.055f);
        float distancia = fabsf(z - centro);
        if (distancia <= 2.5f) return 0;
        if (distancia <= 3.5f) return 1;
        if (distancia <= 4.5f) return 2;
        if (distancia <= 5.5f) return 3;
        /* Uma planicie identificavel logo depois do rio. */
        if (x >= 20 && x < 56 && z >= 37 && z < 56) return 3;
        if (x >= 24 && x < 48 && z >= 2 && z < 15) return 3;
    }
    if ((x >= 64 || z >= 64) && ruidoInteiro(x / 12, z / 12) < 0.24f)
        return 0; /* lagos de varios blocos na regiao nova */
    if ((x >= 64 || z >= 64) && ruidoInteiro(x / 16 + 19, z / 16 + 23) < 0.58f)
        return 3; /* planicies amplas para explorar e cavalgar */
    float base = 2.0f + ruidoInteiro(x / 4, z / 4) * 3.0f;
    float detalhe = ruidoInteiro(x, z) * 1.5f;
    int altura = (int)floorf(base + detalhe);

    if (altura < 0) altura = 0;
    if (altura > 8) altura = 8;
    return altura;
}

int biomaProcedural(int x, int z)
{
    float valor = ruidoInteiro(x / 8, z / 8);
    if (!geracaoLegada) {
        if (x >= 24 && x < 48 && z >= 2 && z < 15) return 4;
        if (x >= 20 && x < 56 && z >= 37 && z < 56) return 0;
    }
    if ((x >= 64 || z >= 64) && ruidoInteiro(x / 16 + 71, z / 16 + 37) > 0.70f)
        return 4; /* bosque de cerejeiras */
    if (valor < 0.22f) return 1; /* deserto */
    if (valor > 0.78f) return 2; /* tundra */
    if (ruidoInteiro(x / 3, z / 3) > 0.58f) return 3; /* floresta */
    return 0; /* planície */
}

int cavernaProcedural(int x, int y, int z)
{
    float grande = ruidoTresDimensoes(x / 2, y / 2, z / 2);
    float detalhe = ruidoTresDimensoes(x, y, z);
    return grande > 0.68f && detalhe > 0.42f;
}

int minerioProcedural(int x, int y, int z)
{
    float valor = ruidoTresDimensoes(x * 3, y * 5, z * 3);
    if (y <= 3 && valor > 0.79f) return 21; /* ferro */
    if (y <= 5 && valor > 0.68f) return 20; /* carvão */
    return 0;
}
