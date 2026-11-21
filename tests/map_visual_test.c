/* Teste de interface com contexto OpenGL real; captura opcional para inspecao. */
#define main minecraft_main
#include "../mine.c"
#undef main
#include <assert.h>

int main(int argc, char **argv)
{
    float x, y, lado, antes;
    unsigned char *pixels;
    FILE *arquivo;
    int linha;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(960, 720);
    glutInitWindowPosition(-2000, -2000);
    glutCreateWindow("Teste do mapa");
    assert(glewInit() == GLEW_OK);
    larguraJanela = 960; alturaJanela = 720;
    glViewport(0, 0, larguraJanela, alturaJanela);
    carregaAtlas();
    carregaTexturasUI();
    {
        size_t total = (size_t)texturaPergaminho.largura * texturaPergaminho.altura * 4u;
        unsigned char *pergaminho = malloc(total);
        size_t fundo = ((size_t)0 * texturaPergaminho.largura + 100) * 4u;
        size_t centro = ((size_t)250 * texturaPergaminho.largura + 250) * 4u;
        size_t linhaExterna = (size_t)20 * texturaPergaminho.largura * 4u;
        size_t contorno = ((size_t)20 * texturaPergaminho.largura + 1) * 4u;
        assert(pergaminho && texturaPergaminho.largura > 250 && texturaPergaminho.altura > 250);
        glBindTexture(GL_TEXTURE_2D, texturaPergaminho.id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pergaminho);
        assert(pergaminho[fundo + 3] == 0 && pergaminho[centro + 3] == 255);
        assert(pergaminho[linhaExterna + 3] == 0 && pergaminho[contorno + 3] == 255);
        free(pergaminho);
        glBindTexture(GL_TEXTURE_2D, atlas);
    }
    inicializaChunks();
    construirMundo();
    iniciaAnimais();
    /* Coluna distante: o mapa deve usar o atlas, mesmo fora dos chunks visiveis. */
    colocaBloco(GRAMA, LADO - 3, ALTURA_MUNDO - 1, LADO - 8);
    estadoJogo = ESTADO_JOGANDO;
    capturaCursor();
    teclado(9, 0, 0);
    assert(!cursorCapturado);
    retanguloIconeMapa(&x, &y, &lado);
    cliqueMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)(x + lado / 2),
                alturaJanela - (int)(y + lado / 2));
    assert(estadoJogo == ESTADO_MAPA && !cursorCapturado && texturaTerrenoMapa);
    assert(tamanhoTexturaMapa == MAPA_TAMANHO_DETALHADO || tamanhoTexturaMapa == MAPA_RESOLUCAO);
    if (tamanhoTexturaMapa == MAPA_TAMANHO_DETALHADO) {
        unsigned char *imagem = malloc((size_t)tamanhoTexturaMapa * tamanhoTexturaMapa * 3u);
        unsigned char atlasOriginal[64 * 64 * 4];
        size_t i = ((size_t)((LADO - 8) * 16 + 4) * tamanhoTexturaMapa + (LADO - 3) * 16 + 3) * 3u;
        int a = (4 * 64 + 3) * 4;
        assert(imagem);
        glBindTexture(GL_TEXTURE_2D, texturaTerrenoMapa);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, imagem);
        glBindTexture(GL_TEXTURE_2D, atlas);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasOriginal);
        assert(imagem[i] == atlasOriginal[a] && imagem[i + 1] == atlasOriginal[a + 1] &&
               imagem[i + 2] == atlasOriginal[a + 2]);
        free(imagem);
    }
    {
        float mx, my, ml;
        int itemAntes = itemSelecionado;
        areaTerrenoMapa(&mx, &my, &ml);
        rodaMouse(0, 1, (int)(mx + ml * 0.5f), alturaJanela - (int)(my + ml * 0.5f));
        assert(zoomMapa > 1.0f && itemSelecionado == itemAntes);
        cliqueMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)(mx + ml * 0.5f), alturaJanela - (int)(my + ml * 0.5f));
        movimentoMouse((int)(mx + ml * 0.4f), alturaJanela - (int)(my + ml * 0.5f));
        assert(inicioMapaX > 0.0f);
        cliqueMouse(GLUT_LEFT_BUTTON, GLUT_UP, 0, 0);
        assert(!arrastandoMapa);
        zoomMapa = 1.0f; inicioMapaX = inicioMapaZ = 0.0f;
    }
    antes = tempoMundo;
    atualiza();
    assert(tempoMundo == antes);
    glClearColor(0.18f, 0.23f, 0.26f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    iniciaInterface();
    desenhaInterface();
    terminaInterface();
    glFinish();
    pixels = malloc((size_t)larguraJanela * alturaJanela * 3u);
    assert(pixels);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, larguraJanela, alturaJanela, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    arquivo = fopen("map_visual_verify.ppm", "wb");
    assert(arquivo);
    fprintf(arquivo, "P6\n%d %d\n255\n", larguraJanela, alturaJanela);
    for (linha = alturaJanela - 1; linha >= 0; linha--)
        assert(fwrite(pixels + (size_t)linha * larguraJanela * 3u,
                      (size_t)larguraJanela * 3u, 1, arquivo) == 1);
    assert(fclose(arquivo) == 0);
    free(pixels);
    teclado(27, 0, 0);
    assert(estadoJogo == ESTADO_JOGANDO && cursorCapturado);
    colocaBloco(AGUA, 2, ALTURA_MUNDO - 1, 3);
    larguraJanela = 320; alturaJanela = 240;
    retanguloIconeMapa(&x, &y, &lado);
    assert(x >= 0 && y >= 0 && x + lado <= larguraJanela && y + lado <= alturaJanela);
    teclado(9, 0, 0);
    cliqueMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)(x + lado / 2),
                alturaJanela - (int)(y + lado / 2));
    assert(estadoJogo == ESTADO_MAPA);
    {
        unsigned char *mapa = malloc((size_t)tamanhoTexturaMapa * tamanhoTexturaMapa * 3u);
        int escala = tamanhoTexturaMapa / LADO;
        assert(mapa);
        glBindTexture(GL_TEXTURE_2D, texturaTerrenoMapa);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, mapa);
        assert(mapa[((size_t)3 * escala * tamanhoTexturaMapa + 2 * escala) * 3 + 2] >
               mapa[((size_t)3 * escala * tamanhoTexturaMapa + 2 * escala) * 3]);
        free(mapa);
        glBindTexture(GL_TEXTURE_2D, atlas);
    }
    glViewport(0, 0, larguraJanela, alturaJanela);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    iniciaInterface(); desenhaInterface(); terminaInterface(); glFinish();
    pixels = malloc((size_t)larguraJanela * alturaJanela * 3u);
    assert(pixels);
    glReadPixels(0, 0, larguraJanela, alturaJanela, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    arquivo = fopen("map_visual_small_verify.ppm", "wb"); assert(arquivo);
    fprintf(arquivo, "P6\n%d %d\n255\n", larguraJanela, alturaJanela);
    for (linha = alturaJanela - 1; linha >= 0; linha--)
        assert(fwrite(pixels + (size_t)linha * larguraJanela * 3u,
                      (size_t)larguraJanela * 3u, 1, arquivo) == 1);
    assert(fclose(arquivo) == 0);
    free(pixels);
    retanguloPainelMapa(&x, &y, &lado);
    cliqueMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)(x + lado - 10.0f),
                alturaJanela - (int)(y + lado - 10.0f));
    assert(estadoJogo == ESTADO_JOGANDO && cursorCapturado);
    {
        float hx, hz;
        int projecaoAnterior = projecaoOrtogonal;
        teclado('p', 0, 0);
        assert(projecaoOrtogonal != projecaoAnterior && estadoJogo == ESTADO_JOGANDO);
        assert(posicaoAnimalMapa(ANIMAL_CAVALO, &hx, &hz));
        jogadorX = hx + 1.0f; jogadorY = 4.01f; jogadorZ = hz;
        teclado('m', 0, 0);
        assert(jogadorMontado() && estadoJogo == ESTADO_JOGANDO);
    }
    puts("map_visual_test: OK");
    return 0;
}
