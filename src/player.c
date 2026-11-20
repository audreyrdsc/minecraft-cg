#include <math.h>
#include "../include/player.h"
#include "../include/world.h"
#include "../include/blocks.h"

#ifndef LARGURA_JOGADOR
#define LARGURA_JOGADOR 0.6f
#endif
#ifndef ALTURA_JOGADOR
#define ALTURA_JOGADOR 1.8f
#endif
#ifndef PASSO_MAXIMO
#define PASSO_MAXIMO 0.5f
#endif
#ifndef VIDA_MAXIMA
#define VIDA_MAXIMA 20.0f
#endif
#ifndef FOME_MAXIMA
#define FOME_MAXIMA 20.0f
#endif
#ifndef SATURACAO_MAXIMA
#define SATURACAO_MAXIMA 20.0f
#endif

float jogadorX = 8.5f;
float jogadorY = 3.0f;
float jogadorZ = 10.5f;
float velocidadeVertical = 0.0f;
float vidaJogador = VIDA_MAXIMA;
float fomeJogador = FOME_MAXIMA;
float saturacaoJogador = SATURACAO_MAXIMA;
float distanciaQueda = 0.0f;

int blocoSolido(int x, int y, int z)
{
    int tipo;
    if (!dentroDoMundo(x, y, z)) return 1;
    tipo = mundo[x][y][z];
    return tipo != VAZIO && tipo != AGUA;
}

int posicaoLivre(float x, float y, float z)
{
    const float folga = 0.001f;
    float meia = LARGURA_JOGADOR / 2.0f;
    int x0 = (int)floorf(x - meia), x1 = (int)floorf(x + meia - folga);
    int y0 = (int)floorf(y), y1 = (int)floorf(y + ALTURA_JOGADOR - folga);
    int z0 = (int)floorf(z - meia), z1 = (int)floorf(z + meia - folga);
    int cx, cy, cz;

    for (cx = x0; cx <= x1; cx++)
        for (cy = y0; cy <= y1; cy++)
            for (cz = z0; cz <= z1; cz++)
                if (blocoSolido(cx, cy, cz)) return 0;
    return 1;
}

int jogadorNoChao(void)
{
    return posicaoLivre(jogadorX, jogadorY, jogadorZ) &&
           !posicaoLivre(jogadorX, jogadorY - 0.06f, jogadorZ);
}

void reapareceJogador(void)
{
    jogadorX = 8.5f;
    jogadorY = 3.0f;
    jogadorZ = 10.5f;
    velocidadeVertical = 0.0f;
    distanciaQueda = 0.0f;
    vidaJogador = VIDA_MAXIMA;
    fomeJogador = FOME_MAXIMA;
    saturacaoJogador = SATURACAO_MAXIMA;
}

static void movePedaco(float dx, float dy, float dz)
{
    if (posicaoLivre(jogadorX + dx, jogadorY, jogadorZ)) jogadorX += dx;
    if (posicaoLivre(jogadorX, jogadorY, jogadorZ + dz)) jogadorZ += dz;
    if (posicaoLivre(jogadorX, jogadorY + dy, jogadorZ)) jogadorY += dy;
}

void moveJogador(float dx, float dy, float dz)
{
    float comprimento = sqrtf(dx * dx + dy * dy + dz * dz);
    int pedacos = (int)ceilf(comprimento / PASSO_MAXIMO);
    int i;

    if (pedacos < 1) pedacos = 1;
    for (i = 0; i < pedacos; i++)
        movePedaco(dx / pedacos, dy / pedacos, dz / pedacos);
}
