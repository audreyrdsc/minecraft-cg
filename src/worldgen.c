#include <math.h>
#include "../include/worldgen.h"

unsigned int sementeMundo = 20261119u;

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
