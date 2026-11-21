/* Captura visual da montaria, da pose de Steve e da etiqueta 3D. */
#define main minecraft_main
#include "../mine.c"
#undef main
#include "../src/entities.c"

int main(int argc, char **argv)
{
    unsigned char *pixels;
    FILE *arquivo;
    int x, y, z;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Teste visual da montaria");
    if (glewInit() != GLEW_OK) return 1;
    larguraJanela = 800; alturaJanela = 600;
    glViewport(0, 0, 800, 600);
    aplicaProjecao();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.53f, 0.75f, 0.93f, 1);
    carregaAtlas();
    inicializaChunks();
    memset(mundo, 0, sizeof(mundo));
    for (x = 0; x < LADO; x++)
        for (z = 0; z < LADO; z++) mundo[x][0][z] = GRAMA;
    jogadorX = 40.5f; jogadorY = 1.0f; jogadorZ = 40.5f;
    tempoMundo = 0.5f;
    iniciaAnimais();
    jogadorX = animais[ANIMAL_CAVALO].x + 1.0f;
    jogadorZ = animais[ANIMAL_CAVALO].z;
    if (!alternaMontaria()) return 2;
    modoCamera = TERCEIRA_PESSOA;
    yaw = 0; pitch = 0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    posicionaCamera();
    desenhaOpacos();
    desenhaAnimais();
    desenhaJogador();
    desenhaNomeSteve();
    glFinish();
    pixels = (unsigned char *)malloc(800u * 600u * 3u);
    if (!pixels) return 3;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    arquivo = fopen("rider-check.ppm", "wb");
    if (!arquivo) { free(pixels); return 4; }
    fprintf(arquivo, "P6\n800 600\n255\n");
    for (y = 599; y >= 0; y--) fwrite(pixels + y * 800 * 3, 1, 800 * 3, arquivo);
    free(pixels);
    return fclose(arquivo) == 0 ? 0 : 5;
}
