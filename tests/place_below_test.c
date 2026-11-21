/* Testa o clique real, sem exigir janela OpenGL. */
#define main minecraft_main
#include "../mine.c"
#undef main
#include <assert.h>

int main(void)
{
    int antes;
    inicializaChunks();
    memset(mundo, 0, sizeof(mundo));
    mundo[8][2][10] = GRAMA;
    jogadorX = 8.5f; jogadorY = 3.0f; jogadorZ = 10.5f;
    assert(blocoApoioJogador() == GRAMA);
    jogadorX = 9.05f; /* centro fora do bloco, mas caixa ainda apoiada */
    assert(blocoApoioJogador() == GRAMA);
    jogadorX = 8.5f;
    pitch = -80.0f;
    iniciaInventario();
    itemSelecionado = 0;
    antes = quantidadesItens[0];
    interageComBloco(GLUT_RIGHT_BUTTON);
    assert(mundo[8][3][10] == GRAMA && jogadorY == 4.0f);
    assert(quantidadesItens[0] == antes - 1 && jogadorNoChao());
    interageComBloco(GLUT_RIGHT_BUTTON);
    assert(mundo[8][4][10] == GRAMA && jogadorY == 5.0f);
    assert(quantidadesItens[0] == antes - 2);
    mundo[8][7][10] = PEDRA; /* impede erguer a cabeca */
    interageComBloco(GLUT_RIGHT_BUTTON);
    assert(mundo[8][5][10] == VAZIO && jogadorY == 5.0f);
    assert(quantidadesItens[0] == antes - 2);
    puts("place_below_test: OK");
    return 0;
}
