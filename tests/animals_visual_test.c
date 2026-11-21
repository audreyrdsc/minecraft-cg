/* Capturas manuais dos modelos em um terreno plano, no OpenGL real. */
#include "../src/entities.c"
#include <assert.h>
extern void carregaAtlas(void);
extern void desenhaOpacos(void);
extern void inicializaChunks(void);
extern void construirMundo(void);

static int capturaAnimal(const char *nome, const Animal *a)
{
    unsigned char *pixels = (unsigned char *)malloc(800u * 600u * 3u);
    FILE *arquivo;
    int y;
    if (!pixels) return 0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(a->x + 2.0f, a->y + 2.0f, a->z - 3.0f,
              a->x, a->y + 0.9f, a->z, 0, 1, 0);
    desenhaOpacos();
    desenhaAnimais();
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    arquivo = fopen(nome, "wb");
    if (!arquivo) { free(pixels); return 0; }
    fprintf(arquivo, "P6\n800 600\n255\n");
    for (y = 599; y >= 0; y--) fwrite(pixels + y * 800 * 3, 1, 800 * 3, arquivo);
    free(pixels);
    return fclose(arquivo) == 0;
}

static int capturaCerejeira(void)
{
    int x, y, z, achou = 0;
    unsigned char *pixels;
    FILE *arquivo;
    construirMundo();
    for (x = 65; x < LADO - 10 && !achou; x++)
        for (z = 10; z < LADO - 10 && !achou; z++)
            for (y = 1; y < ALTURA_MUNDO - 6; y++)
                if (mundo[x][y][z] == TRONCO_CEREJEIRA) { achou = 1; goto encontrou; }
encontrou:
    if (!achou) return 0;
    jogadorX = x + 8.0f; jogadorY = y + 18.0f; jogadorZ = z + 8.0f;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(x + 8.0f, y + 18.0f, z + 8.0f,
              x + 0.5f, y + 2.0f, z + 0.5f, 0, 1, 0);
    desenhaOpacos();
    glFinish();
    pixels = (unsigned char *)malloc(800u * 600u * 3u);
    if (!pixels) return 0;
    glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    arquivo = fopen("animal-check-cerejeira.ppm", "wb");
    if (!arquivo) { free(pixels); return 0; }
    fprintf(arquivo, "P6\n800 600\n255\n");
    for (y = 599; y >= 0; y--) fwrite(pixels + y * 800 * 3, 1, 800 * 3, arquivo);
    free(pixels);
    return fclose(arquivo) == 0;
}

int main(int argc, char **argv)
{
    int x, z;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Teste visual dos animais");
    if (glewInit() != GLEW_OK) return 1;
    glViewport(0, 0, 800, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55, 800.0 / 600.0, 0.1, 200);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.53f, 0.75f, 0.93f, 1);
    carregaAtlas();
    inicializaChunks();
    memset(mundo, 0, sizeof(mundo));
    for (x = 0; x < LADO; x++)
        for (z = 0; z < LADO; z++) mundo[x][0][z] = GRAMA;
    jogadorX = 40.5f; jogadorY = 1.0f; jogadorZ = 40.5f;
    tempoMundo = 0.50f;
    iniciaAnimais();
    assert(capturaAnimal("animal-check-porco.ppm", &animais[ANIMAL_PORCO]));
    assert(capturaAnimal("animal-check-cachorro.ppm", &animais[ANIMAL_CACHORRO]));
    assert(capturaAnimal("animal-check-cavalo.ppm", &animais[ANIMAL_CAVALO]));
    assert(capturaCerejeira());
    puts("Capturas dos animais: OK");
    return 0;
}
