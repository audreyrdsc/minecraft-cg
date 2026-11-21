/* Verifica o icone compartilhado, a extrusao e o estado OpenGL. */
#define main minecraft_main
#include "../mine.c"
#undef main
#include <assert.h>

static void pixel(int x, int y, unsigned char cor[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, cor);
}

int main(int argc, char **argv)
{
    unsigned char frente[4], vazio[4], lado[4];
    size_t brilho = ((size_t)32 * MACA_TEX_LADO + 21) * 4u;
    GLint texturaAntes, texturaDepois;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(256, 256);
    glutInitWindowPosition(-2000, -2000);
    glutCreateWindow("Teste da maca 3D");
    assert(glewInit() == GLEW_OK);
    carregaAtlas();
    assert(texturaMaca3D && glIsTexture(texturaMaca3D));
    assert(pixelsMaca[3] == 0 && pixelsMaca[brilho + 3] == 255);
    assert(pixelsMaca[brilho] > pixelsMaca[brilho + 1]);
    tempoMundo = 0.5f;
    glViewport(0, 0, 256, 256);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, 1, 0, 1, -10, 10);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_FOG); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texturaAntes);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    desenhaMacaNaCopa();
    glFinish();
    pixel(21 * 256 / 58, 32 * 256 / 58, frente);
    pixel(4, 4, vazio);
    assert(frente[0] > frente[1] && frente[0] > 30);
    assert(vazio[0] == 0 && vazio[1] == 0 && vazio[2] == 0);
    assert(glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_CULL_FACE));
    assert(!glIsEnabled(GL_ALPHA_TEST));
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texturaDepois);
    assert(texturaAntes == texturaDepois && glGetError() == GL_NO_ERROR);

    /* Perfil lateral: a extrusao nao desaparece ao girar 90 graus. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity(); glTranslatef(0.0f, 0.0f, 0.5f);
    glRotatef(90, 0, 1, 0); glTranslatef(-0.5f, 0, 0);
    desenhaMacaNaCopa(); glFinish();
    pixel(128, 32 * 256 / 58, lado);
    assert(lado[0] > lado[1] && lado[0] > 10);

    inicializaChunks(); construirMundo();
    preparaListasChunks();
    {
        int x, y, z, encontrou = 0;
        for (x = 0; x < LADO && !encontrou; x++)
            for (z = 0; z < LADO && !encontrou; z++)
                for (y = 0; y < ALTURA_MUNDO && !encontrou; y++)
                    if (mundo[x][y][z] == FOLHA && facesVisiveis(x,y,z) &&
                        ((unsigned)x * 31u + (unsigned)y * 17u + (unsigned)z * 13u + sementeMundo) % 19u == 0u) {
                        int cx = x / CHUNK_LADO, cz = z / CHUNK_LADO;
                        constroiListaChunk(cx, cz);
                        assert(!chunkEstaSujo(cx, cz));
                        glCallList(listasChunks[cx][cz][PASSADA_RECORTE]);
                        colocaBloco(VAZIO, x, y, z);
                        assert(chunkEstaSujo(cx, cz));
                        constroiListaChunk(cx, cz);
                        assert(!chunkEstaSujo(cx, cz));
                        encontrou = 1;
                    }
        assert(encontrou && glGetError() == GL_NO_ERROR);
    }
    puts("apple_model_test: OK");
    return 0;
}
