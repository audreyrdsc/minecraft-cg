/* Testa a logica real dos zumbis sem precisar abrir uma janela OpenGL. */
#include "../src/entities.c"
#include "../include/world.h"
#include <string.h>

int main(void)
{
    int x, i;
    float posicaoInicial;
    memset(mundo, 0, sizeof(mundo));
    memset(zumbis, 0, sizeof(zumbis));
    memset(fogos, 0, sizeof(fogos));
    for (x = 0; x < LADO; x++) mundo[x][0][10] = 1;
    tempoMundo = 0.25f; /* 6h: inicio do dia */
    jogadorX = 40.5f;
    jogadorY = 1.0f;
    jogadorZ = 10.5f;
    zumbis[0].ativo = 1;
    zumbis[0].natural = 1;
    zumbis[0].tipo = 0;
    zumbis[0].x = 10.5f;
    zumbis[0].y = 1.0f;
    zumbis[0].z = 10.5f;
    zumbis[0].vida = 12.0f;
    posicaoInicial = zumbis[0].x;

    atualizaEntidades(1.0f);
    if (!zumbis[0].ativo || !fogos[0].ativo ||
        zumbis[0].x <= posicaoInicial || zumbis[0].faseCaminhada <= 0.0f ||
        fogos[0].restante != 9.0f) return 1;
    for (i = 0; i < 8; i++) atualizaEntidades(1.0f);
    if (!zumbis[0].ativo || !fogos[0].ativo || fogos[0].restante != 1.0f)
        return 2;
    atualizaEntidades(1.0f);
    if (zumbis[0].ativo || fogos[0].ativo) return 3;
    printf("Zumbi andou em chamas e desapareceu apos 10 segundos.\n");
    return 0;
}
