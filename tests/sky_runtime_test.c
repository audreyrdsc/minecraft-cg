/* Diagnostico manual do ceu no contexto OpenGL real.
   Executar a partir da raiz; imagens sky-check-*.ppm sao temporarias. */
#define main minecraft_main
#include "../mine.c"
#undef main

static int capturaCeu(const char *caminho, float tempo)
{
    unsigned char pixels[800 * 600 * 3];
    FILE *arquivo;
    int linha;
    tempoMundo = tempo;
    tempoAnimacaoCeu = tempo * DURACAO_DIA;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    posicionaCamera();
    desenhaCeuShader();
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR) return 0;
    arquivo = fopen(caminho, "wb");
    if (!arquivo) return 0;
    fprintf(arquivo, "P6\n800 600\n255\n");
    for (linha = 599; linha >= 0; linha--)
        fwrite(pixels + linha * 800 * 3, 1, 800 * 3, arquivo);
    return fclose(arquivo) == 0;
}

int main(int argc, char **argv)
{
    int ok;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Diagnostico do ceu");
    if (glewInit() != GLEW_OK) return 1;
    while (glGetError() != GL_NO_ERROR) {}
    larguraJanela = 800;
    alturaJanela = 600;
    glViewport(0, 0, 800, 600);
    aplicaProjecao();
    modoCamera = PRIMEIRA_PESSOA;
    yaw = 0.0f;
    pitch = 35.0f;
    inicializaShaderCeu();
    if (!shaderCeu) return 2;
    ok = capturaCeu("sky-check-dia.ppm", 0.50f) &&
         capturaCeu("sky-check-noite.ppm", 0.00f) &&
         capturaCeu("sky-check-meteoro.ppm", 0.83f) &&
         capturaCeu("sky-check-cometa.ppm", 0.85f);
    printf("Capturas do ceu: %s\n", ok ? "OK" : "FALHA");
    return ok ? 0 : 3;
}
