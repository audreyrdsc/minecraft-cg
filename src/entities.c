#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/entities.h"
#include "../include/inventory.h"
#include "../include/worldgen.h"
#include "../include/world.h"
#include "../include/player.h"
#include "../include/blocks.h"
#include "../include/audio.h"

#ifndef MAX_DROPS
#define MAX_DROPS 128
#endif
#ifndef GRAVIDADE
#define GRAVIDADE 18.0f
#endif
#ifndef TODAS_FACES
#define TODAS_FACES 63
#endif
#include "../include/config.h"
#ifndef PELE
#define PELE 10
#endif
#ifndef CAMISA
#define CAMISA 12
#endif
#ifndef FOLHA
#define FOLHA 5
#endif
#ifndef MACA
#define MACA 19
#endif
#ifndef ESPADA
#define ESPADA 23
#endif
#ifndef CALCA
#define CALCA 13
#endif
#ifndef CRISTAL
#define CRISTAL 24
#endif
#ifndef RAIO_COLETA
#define RAIO_COLETA 1.55f
#endif
#ifndef NUM_ITENS
#define NUM_ITENS 13
#endif
#ifndef PI
#define PI 3.14159265f
#endif

extern int indiceDoItem(int tipo);
extern int blocoSolido(int x, int y, int z);
extern int alturaDoTerreno(int x, int z);
extern int adicionaAoInventario(int tipo, int quantidade);
extern float yaw;
extern float jogadorX, jogadorY, jogadorZ, tempoMundo;
extern float vidaJogador;
extern int itemSelecionado;
extern const int tiposItens[NUM_ITENS];
extern void posicaoDoOlho(float *x, float *y, float *z);
extern void direcaoDoRaio(float *dx, float *dy, float *dz);
extern void desenhaBloco(int tipo, float tom, int faces);
extern void audioDano(void);
extern void audioAtaque(void);
extern void audioZumbiAtingido(void);
extern void audioZumbiMorreu(void);
extern void audioSpawnZumbi(void);
extern void audioColeta(void);
extern int gastaDurabilidade(int tipo);

#define MAX_ZUMBIS 4
typedef struct {
    int ativo;
    int tipo;
    float x, y, z;
    float vida;
    float tempoAtaque;
    float tempoDano;
    float faseCaminhada;
    int natural;
} Zumbi;

typedef struct {
    int ativo;
    float restante;
    float fase;
} FogoZumbi;

static Zumbi zumbis[MAX_ZUMBIS];
static FogoZumbi fogos[MAX_ZUMBIS];
static float tempoSpawn = 0.0f;
static int sequenciaSpawn = 0;
static float tempoAtaqueJogador = 0.0f;
static unsigned int inimigosProximosDoCachorro = 0;

#define NUM_ANIMAIS 3
enum { ANIMAL_PORCO, ANIMAL_CACHORRO, ANIMAL_CAVALO };
typedef struct {
    int ativo, tipo;
    float x, y, z, angulo, fase, espera, velocidadeY;
    unsigned int aleatorio;
} Animal;
static Animal animais[NUM_ANIMAIS];
static int cavaloMontado = 0;
static int jogadorProximoCavalo = 0;
static int jogadorProximoPorco = 0;

static GLuint shaderFogo = 0;
static GLint shaderFogoTempo = -1;
static GLint shaderFogoFase = -1;

static GLuint compilaShaderFogo(GLenum tipo, const char *fonte)
{
    GLuint shader = glCreateShader(tipo);
    GLint compilou = GL_FALSE;

    if (!shader) return 0;
    glShaderSource(shader, 1, &fonte, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compilou);
    if (!compilou) {
        char log[1024];
        GLsizei tamanho = 0;
        glGetShaderInfoLog(shader, sizeof(log), &tamanho, log);
        log[(size_t)(tamanho < (GLsizei)sizeof(log) ? tamanho :
             (GLsizei)sizeof(log) - 1)] = '\0';
        printf("Shader do fogo: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static void inicializaShaderFogo(void)
{
    static const char *vertexShader =
        "#version 120\n"
        "uniform float u_tempo;\n"
        "uniform float u_fase;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "  vec4 p = gl_Vertex;\n"
        "  float pulso = sin(u_tempo * 7.0 + u_fase + p.y * 8.0);\n"
        "  p.x += pulso * 0.035 * (p.y + 0.2);\n"
        "  p.z += cos(u_tempo * 5.0 + u_fase * 1.7 + p.y * 6.0) * 0.025 * (p.y + 0.2);\n"
        "  gl_Position = gl_ModelViewProjectionMatrix * p;\n"
        "  v_uv = gl_MultiTexCoord0.st;\n"
        "}\n";
    static const char *fragmentShader =
        "#version 120\n"
        "uniform float u_tempo;\n"
        "uniform float u_fase;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  float deslocamento = sin(u_tempo * 8.0 + u_fase + uv.y * 12.0) * 0.08;\n"
        "  float largura = 0.30 + 0.12 * sin(u_tempo * 6.0 + u_fase * 2.0);\n"
        "  float chama = 1.0 - smoothstep(0.0, largura, abs(uv.x - 0.5 + deslocamento * (1.0 - uv.y)));\n"
        "  chama *= smoothstep(0.02, 0.16, uv.y);\n"
        "  chama *= 1.0 - smoothstep(0.72, 1.0, uv.y);\n"
        "  float detalhe = 0.5 + 0.5 * sin(uv.x * 24.0 + uv.y * 18.0 + u_tempo * 10.0 + u_fase);\n"
        "  chama *= 0.72 + detalhe * 0.28;\n"
        "  if (chama < 0.08) discard;\n"
        "  vec3 base = mix(vec3(1.0, 0.08, 0.01), vec3(1.0, 0.58, 0.02), smoothstep(0.10, 0.58, uv.y));\n"
        "  base = mix(base, vec3(1.0, 0.96, 0.42), smoothstep(0.48, 0.88, uv.y));\n"
        "  gl_FragColor = vec4(base, chama * 0.86);\n"
        "}\n";
    GLuint vs, fs, programa;
    GLint ligado = GL_FALSE;

    if (shaderFogo || !glCreateShader) return;
    vs = compilaShaderFogo(GL_VERTEX_SHADER, vertexShader);
    fs = compilaShaderFogo(GL_FRAGMENT_SHADER, fragmentShader);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }
    programa = glCreateProgram();
    glAttachShader(programa, vs);
    glAttachShader(programa, fs);
    glLinkProgram(programa);
    glGetProgramiv(programa, GL_LINK_STATUS, &ligado);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ligado) {
        char log[1024];
        GLsizei tamanho = 0;
        glGetProgramInfoLog(programa, sizeof(log), &tamanho, log);
        log[(size_t)(tamanho < (GLsizei)sizeof(log) ? tamanho :
             (GLsizei)sizeof(log) - 1)] = '\0';
        printf("Programa do fogo: %s\n", log);
        glDeleteProgram(programa);
        return;
    }
    shaderFogo = programa;
    shaderFogoTempo = glGetUniformLocation(shaderFogo, "u_tempo");
    shaderFogoFase = glGetUniformLocation(shaderFogo, "u_fase");
    printf("Shader do fogo ativo.\n");
}

static void desenhaQuadFogo(float deslocamento, float giro)
{
    glPushMatrix();
    glRotatef(giro, 0.0f, 1.0f, 0.0f);
    glTranslatef(deslocamento, 1.24f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.84f, -1.24f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.84f, -1.24f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.50f,  1.56f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.50f,  1.56f, 0.0f);
    glEnd();
    glPopMatrix();
}

static void desenhaFogoZumbi(const FogoZumbi *fogo)
{
    if (!shaderFogo || !fogo->ativo) return;
    glUseProgram(shaderFogo);
    glUniform1f(shaderFogoTempo, fogo->fase);
    glUniform1f(shaderFogoFase, fogo->fase * 1.37f);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    desenhaQuadFogo(-0.10f, 0.0f);
    desenhaQuadFogo( 0.08f, 90.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glUseProgram(0);
}

static DropItem drops[MAX_DROPS];

/* Mantém a base da entidade apoiada no bloco sólido mais próximo abaixo
   dela. Isso funciona como uma gravidade simples sem permitir que o
   inimigo atravesse o terreno ao descer uma encosta. */
static void ajustaAlturaNoTerreno(Zumbi *zumbi)
{
    int x = (int)floorf(zumbi->x);
    int z = (int)floorf(zumbi->z);
    int y = (int)floorf(zumbi->y - 0.01f);

    if (x < 0 || x >= LADO || z < 0 || z >= LADO) return;
    if (y >= ALTURA_MUNDO) y = ALTURA_MUNDO - 1;
    while (y >= 0 && !blocoSolido(x, y, z)) y--;
    if (y >= 0) zumbi->y = y + 1.0f;
}

void criaDrop(int tipo, float x, float y, float z)
{
    int i;

    if (indiceDoItem(tipo) < 0) return;
    for (i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].ativo) {
            drops[i].ativo = 1;
            drops[i].tipo = tipo;
            drops[i].x = x;
            drops[i].y = y;
            drops[i].z = z;
            drops[i].velocidadeY = 2.0f;
            return;
        }
    }
}

void atualizaDrops(float dt)
{
    int i;

    for (i = 0; i < MAX_DROPS; i++) {
        DropItem *drop = &drops[i];
        int indice;

        if (!drop->ativo) continue;

        drop->velocidadeY -= GRAVIDADE * dt;
        drop->y += drop->velocidadeY * dt;
        if (drop->y < 0.0f ||
            blocoSolido((int)floorf(drop->x), (int)floorf(drop->y),
                        (int)floorf(drop->z))) {
            drop->y = ceilf(drop->y);
            if (drop->y < 0.0f) drop->y = 0.0f;
            drop->velocidadeY = 0.0f;
        }

        indice = indiceDoItem(drop->tipo);
        if (indice >= 0) {
            float dx = drop->x - jogadorX;
            float dy = drop->y - (jogadorY + 0.8f);
            float dz = drop->z - jogadorZ;

            /* O raio anterior de 0,8 bloco fazia drops de blocos
               adjacentes parecerem próximos, mas permanecerem no chão. */
            if (dx * dx + dy * dy + dz * dz < RAIO_COLETA * RAIO_COLETA &&
                adicionaAoInventario(drop->tipo, 1) == 1) {
                drop->ativo = 0;
                audioColeta();
            }
        }
    }
}

void desenhaDrops(void)
{
    int i;

    for (i = 0; i < MAX_DROPS; i++) {
        DropItem *drop = &drops[i];
        if (!drop->ativo) continue;

        glPushMatrix();
        glTranslatef(drop->x - 0.125f, drop->y - 0.125f, drop->z - 0.125f);
        glScalef(0.25f, 0.25f, 0.25f);
        desenhaBloco(drop->tipo, 1.0f, TODAS_FACES);
        glPopMatrix();
    }
}

void iniciaEntidades(void)
{
    int i;
    for (i = 0; i < MAX_ZUMBIS; i++) zumbis[i].ativo = 0;
    for (i = 0; i < MAX_ZUMBIS; i++) {
        fogos[i].ativo = 0;
        fogos[i].restante = 0.0f;
        fogos[i].fase = 0.0f;
    }
    inicializaShaderFogo();

    /* Nenhum inimigo começa ativo: zumbis só são criados por spawn noturno. */
    zumbis[0].ativo = 0;
    zumbis[0].tipo = 0;
    zumbis[0].x = 5.5f;
    zumbis[0].z = 5.5f;
    zumbis[0].y = alturaDoTerreno(5, 5) + 1.0f;
    zumbis[0].vida = 12.0f;
    zumbis[0].tempoAtaque = 2.0f;
    zumbis[0].tempoDano = 0.0f;
    zumbis[0].faseCaminhada = 0.0f;
    zumbis[0].natural = 0;
    tempoSpawn = 0.0f;
    sequenciaSpawn = 0;
    tempoAtaqueJogador = 0.0f;
    iniciaAnimais();
}

static int chaoSeguroAnimal(int x, int z)
{
    int y;
    if (x < 1 || x >= LADO - 1 || z < 1 || z >= LADO - 1) return -1;
    for (y = ALTURA_MUNDO - 3; y >= 0; y--) {
        int tipo = mundo[x][y][z];
        if (tipo == VAZIO) continue;
        if ((tipo == GRAMA || tipo == TERRA || tipo == AREIA ||
             tipo == PEDRA || tipo == NEVE) && mundo[x][y + 1][z] == VAZIO &&
            mundo[x][y + 2][z] == VAZIO) return y + 1;
        return -1;
    }
    return -1;
}

static int animalLivre(float x, float y, float z, float altura);

void iniciaAnimais(void)
{
    int i, raio, dx, dz;
    inimigosProximosDoCachorro = 0;
    cavaloMontado = 0;
    jogadorProximoCavalo = 0;
    jogadorProximoPorco = 0;
    memset(animais, 0, sizeof(animais));
    for (i = 0; i < NUM_ANIMAIS; i++) {
        Animal *a = &animais[i];
        a->tipo = i;
        a->aleatorio = sementeMundo ^ (unsigned int)(0x9e3779b9u * (i + 1));
        /* No mundo inicial este ponto e plano; em saves modificados nunca
           enterramos o cavalo em blocos construidos pelo jogador. */
        if (i == ANIMAL_CAVALO && chaoSeguroAnimal(12, 2) == 3 &&
            animalLivre(12.0f, 3.0f, 2.5f, 2.3f)) {
            a->x = 12.0f; a->y = 3.0f; a->z = 2.5f;
            a->ativo = 1;
            continue;
        }
        for (raio = 3 + i * 2; raio < 20 && !a->ativo; raio++)
            for (dx = -raio; dx <= raio && !a->ativo; dx++)
                for (dz = -raio; dz <= raio && !a->ativo; dz++) {
                    int x = (int)floorf(jogadorX) + dx;
                    int z = (int)floorf(jogadorZ) + dz;
                    int chao;
                    if (abs(dx) != raio && abs(dz) != raio) continue;
                    chao = chaoSeguroAnimal(x, z);
                    if (chao < 0 || fabsf((float)chao - jogadorY) > 2.0f) continue;
                    a->x = x + 0.5f; a->y = (float)chao; a->z = z + 0.5f;
                    a->ativo = 1;
                }
    }
}

static int animalLivre(float x, float y, float z, float altura)
{
    int cx, cy, cz;
    float raio = altura > 1.6f ? 0.64f : 0.32f;
    for (cx = (int)floorf(x - raio); cx <= (int)floorf(x + raio); cx++)
        for (cy = (int)floorf(y + 0.01f); cy <= (int)floorf(y + altura - 0.01f); cy++)
            for (cz = (int)floorf(z - raio); cz <= (int)floorf(z + raio); cz++)
                if (blocoSolido(cx, cy, cz)) return 0;
    return 1;
}

static int animalAnda(Animal *a, float dx, float dz)
{
    float nx = a->x + dx, nz = a->z + dz;
    int chao = chaoSeguroAnimal((int)floorf(nx), (int)floorf(nz));
    float altura = a->tipo == ANIMAL_CAVALO ? 2.3f : 1.0f;
    if (chao < 0 || fabsf(chao - a->y) > 1.01f ||
        !animalLivre(nx, (float)chao, nz, altura)) return 0;
    a->x = nx; a->z = nz; a->y = (float)chao;
    return 1;
}

static void atualizaProximidadeCavalo(void)
{
    Animal *cavalo = &animais[ANIMAL_CAVALO];
    float dx, dy, dz;
    int proximo;
    if (!cavalo->ativo) { jogadorProximoCavalo = 0; return; }
    /* Ao desmontar dentro do raio, nao confundir a desmontagem com uma
       nova aproximacao: o jogador deve sair e entrar novamente no raio. */
    if (cavaloMontado) { jogadorProximoCavalo = 1; return; }
    dx = jogadorX - cavalo->x;
    dy = jogadorY - cavalo->y;
    dz = jogadorZ - cavalo->z;
    proximo = dx * dx + dy * dy + dz * dz <= 1.0f;
    if (proximo && !jogadorProximoCavalo) audioCavaloProximo();
    jogadorProximoCavalo = proximo;
}

static void atualizaProximidadePorco(void)
{
    Animal *porco = &animais[ANIMAL_PORCO];
    float dx, dy, dz;
    int proximo;
    if (!porco->ativo) { jogadorProximoPorco = 0; return; }
    dx = jogadorX - porco->x;
    dy = jogadorY - porco->y;
    dz = jogadorZ - porco->z;
    proximo = dx * dx + dy * dy + dz * dz <= 4.0f;
    if (proximo && !jogadorProximoPorco) audioPorcoProximo();
    jogadorProximoPorco = proximo;
}

void atualizaAnimais(float dt)
{
    int i;
    for (i = 0; i < NUM_ANIMAIS; i++) {
        Animal *a = &animais[i];
        float dx, dz, dist, passo;
        if (!a->ativo || (i == ANIMAL_CAVALO && cavaloMontado)) continue;
        if (i == ANIMAL_CACHORRO) {
            dx = jogadorX - a->x; dz = jogadorZ - a->z;
            dist = sqrtf(dx * dx + dz * dz);
            if (dist < 1.8f || dist < 0.001f) continue;
            a->angulo = atan2f(-dx, -dz) * 180.0f / PI;
            passo = (dist > 6.0f ? 2.8f : 1.5f) * dt;
            if (passo > dist - 1.5f) passo = dist - 1.5f;
            if (animalAnda(a, dx / dist * passo, dz / dist * passo) ||
                animalAnda(a, dx / dist * passo, 0) || animalAnda(a, 0, dz / dist * passo)) {
                a->fase += dt * 7.0f;
                a->espera = 0.0f;
            } else {
                int raio, ox, oz;
                a->espera += dt;
                /* Recupera o cachorro se ficar preso muito longe do jogador. */
                if (dist > 8.0f && a->espera > 4.0f) {
                    for (raio = 2; raio <= 5 && a->espera > 0.0f; raio++)
                        for (ox = -raio; ox <= raio && a->espera > 0.0f; ox++)
                            for (oz = -raio; oz <= raio && a->espera > 0.0f; oz++) {
                                int x = (int)floorf(jogadorX) + ox;
                                int z = (int)floorf(jogadorZ) + oz;
                                int y = chaoSeguroAnimal(x, z);
                                if ((abs(ox) == raio || abs(oz) == raio) && y >= 0 &&
                                    animalLivre(x + 0.5f, (float)y, z + 0.5f, 1.0f)) {
                                    a->x = x + 0.5f; a->y = (float)y; a->z = z + 0.5f;
                                    a->espera = 0.0f;
                                }
                            }
                }
            }
        } else {
            a->espera -= dt;
            if (a->espera <= 0.0f) {
                a->aleatorio = a->aleatorio * 1664525u + 1013904223u;
                a->angulo = (float)(a->aleatorio % 360u);
                a->espera = 2.0f + (float)((a->aleatorio >> 9) % 40u) / 10.0f;
            }
            if (a->espera < 1.0f) continue;
            dx = -sinf(a->angulo * PI / 180.0f);
            dz = -cosf(a->angulo * PI / 180.0f);
            passo = (i == ANIMAL_CAVALO ? 0.9f : 0.7f) * dt;
            if (animalAnda(a, dx * passo, dz * passo)) a->fase += dt * 6.0f;
            else a->espera = 0.0f;
        }
    }
    atualizaProximidadeCavalo();
    atualizaProximidadePorco();
}

int jogadorMontado(void) { return cavaloMontado; }
void cancelaMontaria(void) { cavaloMontado = 0; audioAtualizaGalope(0); }

int alternaMontaria(void)
{
    Animal *a = &animais[ANIMAL_CAVALO];
    int i;
    if (!a->ativo) return 0;
    if (!cavaloMontado) {
        float dx = jogadorX - a->x, dz = jogadorZ - a->z;
        if (dx * dx + dz * dz > 3.0f * 3.0f || fabsf(jogadorY - a->y) > 2.0f ||
            !posicaoLivre(a->x, a->y + 1.15f, a->z)) return 0;
        cavaloMontado = 1;
        jogadorProximoCavalo = 1;
        jogadorX = a->x; jogadorY = a->y + 1.15f; jogadorZ = a->z;
        velocidadeVertical = 0.0f;
        audioMontaria();
        return 1;
    }
    for (i = 0; i < 8; i++) {
        float angulo = i * PI / 4.0f;
        float x = a->x + cosf(angulo) * 1.35f;
        float z = a->z + sinf(angulo) * 1.35f;
        int y = chaoSeguroAnimal((int)floorf(x), (int)floorf(z));
        if (y >= 0 && fabsf(y - a->y) <= 1.0f && posicaoLivre(x, (float)y, z)) {
            jogadorX = x; jogadorY = (float)y; jogadorZ = z;
            velocidadeVertical = 0.0f;
            cavaloMontado = 0;
            audioAtualizaGalope(0);
            audioMontaria();
            return 1;
        }
    }
    return 0;
}

void moveMontaria(float dx, float dz, float dt, int saltar)
{
    Animal *a = &animais[ANIMAL_CAVALO];
    int apoiado;
    float proximoY;
    float antesX, antesZ;
    if (!cavaloMontado || !a->ativo) return;
    antesX = a->x; antesZ = a->z;
    a->angulo = -yaw;
    apoiado = !animalLivre(a->x, a->y - 0.06f, a->z, 2.3f);
    if (saltar && apoiado) a->velocidadeY = 6.0f;
    a->velocidadeY -= GRAVIDADE * dt;
    proximoY = a->y + a->velocidadeY * dt;
    if (animalLivre(a->x, proximoY, a->z, 2.3f) &&
        posicaoLivre(a->x, proximoY + 1.15f, a->z)) a->y = proximoY;
    else a->velocidadeY = 0.0f;
    if (animalLivre(a->x + dx, a->y, a->z, 2.3f) &&
        posicaoLivre(a->x + dx, a->y + 1.15f, a->z)) a->x += dx;
    if (animalLivre(a->x, a->y, a->z + dz, 2.3f) &&
        posicaoLivre(a->x, a->y + 1.15f, a->z + dz)) a->z += dz;
    audioAtualizaGalope(fabsf(a->x - antesX) + fabsf(a->z - antesZ) > 0.00001f);
    if (dx * dx + dz * dz > 0.000001f) {
        a->fase += dt * 8.0f;
    }
    jogadorX = a->x; jogadorY = a->y + 1.15f; jogadorZ = a->z;
    velocidadeVertical = 0.0f;
}

int salvaEstadoAnimais(FILE *arquivo)
{
    return fwrite(animais, sizeof(animais), 1, arquivo) == 1 &&
           fwrite(&cavaloMontado, sizeof(cavaloMontado), 1, arquivo) == 1;
}

int carregaEstadoAnimais(FILE *arquivo)
{
    Animal novos[NUM_ANIMAIS];
    int montado;
    if (fread(novos, sizeof(novos), 1, arquivo) != 1 ||
        fread(&montado, sizeof(montado), 1, arquivo) != 1) return 0;
    memcpy(animais, novos, sizeof(animais));
    cavaloMontado = montado && animais[ANIMAL_CAVALO].ativo;
    jogadorProximoCavalo = cavaloMontado ? 1 : 0;
    jogadorProximoPorco = 0;
    return 1;
}

static void preservaPosicao(unsigned char alterado[NUM_CHUNKS_X][NUM_CHUNKS_Z],
                            float x, float z)
{
    int ox, oz;
    if (!isfinite(x) || !isfinite(z) || x < 0 || x >= LADO || z < 0 || z >= LADO) return;
    for (ox = -1; ox <= 1; ox++)
        for (oz = -1; oz <= 1; oz++) {
            int px = (int)floorf(x + ox), pz = (int)floorf(z + oz);
            if (px >= 0 && px < LADO && pz >= 0 && pz < LADO)
                alterado[px / CHUNK_LADO][pz / CHUNK_LADO] = 1;
        }
}

void preservaChunksEntidades(unsigned char alterado[NUM_CHUNKS_X][NUM_CHUNKS_Z])
{
    int i;
    for (i = 0; i < NUM_ANIMAIS; i++)
        if (animais[i].ativo) preservaPosicao(alterado, animais[i].x, animais[i].z);
    for (i = 0; i < MAX_ZUMBIS; i++)
        if (zumbis[i].ativo) preservaPosicao(alterado, zumbis[i].x, zumbis[i].z);
    for (i = 0; i < MAX_DROPS; i++)
        if (drops[i].ativo) preservaPosicao(alterado, drops[i].x, drops[i].z);
}

int salvaEstadoEntidades(FILE *arquivo)
{
    return fwrite(zumbis, sizeof(zumbis), 1, arquivo) == 1 &&
           fwrite(&tempoSpawn, sizeof(tempoSpawn), 1, arquivo) == 1 &&
           fwrite(&sequenciaSpawn, sizeof(sequenciaSpawn), 1, arquivo) == 1 &&
           fwrite(&tempoAtaqueJogador, sizeof(tempoAtaqueJogador), 1, arquivo) == 1 &&
           fwrite(drops, sizeof(drops), 1, arquivo) == 1;
}

int carregaEstadoEntidades(FILE *arquivo)
{
    int i;
    Zumbi novosZumbis[MAX_ZUMBIS];
    DropItem novosDrops[MAX_DROPS];
    float novoTempoSpawn, novoTempoAtaque;
    int novaSequencia;
    int sucesso = fread(novosZumbis, sizeof(novosZumbis), 1, arquivo) == 1 &&
           fread(&novoTempoSpawn, sizeof(novoTempoSpawn), 1, arquivo) == 1 &&
           fread(&novaSequencia, sizeof(novaSequencia), 1, arquivo) == 1 &&
           fread(&novoTempoAtaque, sizeof(novoTempoAtaque), 1, arquivo) == 1 &&
           fread(novosDrops, sizeof(novosDrops), 1, arquivo) == 1;
    if (!sucesso) return 0;
    memcpy(zumbis, novosZumbis, sizeof(zumbis));
    memcpy(drops, novosDrops, sizeof(drops));
    tempoSpawn = novoTempoSpawn;
    sequenciaSpawn = novaSequencia;
    tempoAtaqueJogador = novoTempoAtaque;
    inimigosProximosDoCachorro = 0;
    for (i = 0; i < MAX_ZUMBIS; i++) {
        fogos[i].ativo = 0;
        fogos[i].restante = 0.0f;
        fogos[i].fase = 0.0f;
    }
    return sucesso;
}

static int eNoite(void)
{
    return sinf((tempoMundo - 0.25f) * 2.0f * PI) < 0.0f;
}

static void atualizaAlertaCachorro(void)
{
    unsigned int proximos = 0;
    int i;
    if (animais[ANIMAL_CACHORRO].ativo)
        for (i = 0; i < MAX_ZUMBIS; i++) {
            float dx, dy, dz;
            if (!zumbis[i].ativo) continue;
            dx = zumbis[i].x - jogadorX;
            dy = zumbis[i].y - jogadorY;
            dz = zumbis[i].z - jogadorZ;
            if (dx * dx + dy * dy + dz * dz <= 25.0f) {
                unsigned int bit = 1u << i;
                proximos |= bit;
                if (!(inimigosProximosDoCachorro & bit)) audioAlertaCachorro();
            }
        }
    inimigosProximosDoCachorro = proximos;
}

static void atualizaSomFogoZumbis(void)
{
    int i;
    for (i = 0; i < MAX_ZUMBIS; i++)
        if (zumbis[i].ativo && fogos[i].ativo) {
            audioAtualizaFogoZumbi(1);
            return;
        }
    audioAtualizaFogoZumbi(0);
}

static void tentaSpawnarZumbi(void)
{
    int i, slot = -1;
    int x, z, chao;
    float angulo, distancia;

    for (i = 0; i < MAX_ZUMBIS; i++)
        if (!zumbis[i].ativo) {
            slot = i;
            break;
        }
    if (slot < 0) return;

    angulo = (float)(sequenciaSpawn++ % 8) * (PI / 4.0f);
    distancia = 7.0f + (float)(sequenciaSpawn % 3);
    x = (int)floorf(jogadorX + cosf(angulo) * distancia);
    z = (int)floorf(jogadorZ + sinf(angulo) * distancia);
    if (x < 1 || x >= LADO - 1 || z < 1 || z >= LADO - 1) return;
    chao = alturaDoTerreno(x, z);
    if (chao + 1 >= 15 || blocoSolido(x, chao + 1, z)) return;

    zumbis[slot].ativo = 1;
    zumbis[slot].tipo = biomaProcedural(x, z) == 2 ? 1 : 0;
    zumbis[slot].x = x + 0.5f;
    zumbis[slot].z = z + 0.5f;
    zumbis[slot].y = chao + 1.0f;
    zumbis[slot].vida = zumbis[slot].tipo ? 8.0f : 12.0f;
    zumbis[slot].tempoAtaque = 1.0f;
    zumbis[slot].tempoDano = 0.0f;
    zumbis[slot].faseCaminhada = 0.0f;
    zumbis[slot].natural = 1;
    fogos[slot].ativo = 0;
    fogos[slot].restante = 0.0f;
    fogos[slot].fase = 0.0f;
    audioSpawnZumbi();
}

void atualizaEntidades(float dt)
{
    int i;

    if (tempoAtaqueJogador > 0.0f) tempoAtaqueJogador -= dt;

    for (i = 0; i < MAX_ZUMBIS; i++) {
        Zumbi *zumbi = &zumbis[i];
        float dx, dz, distancia;

        if (!zumbi->ativo) continue;
        if (zumbi->natural && !eNoite()) {
            if (zumbi->tipo == 0 && !fogos[i].ativo) {
                fogos[i].ativo = 1;
                fogos[i].restante = 10.0f;
                fogos[i].fase = 0.0f;
            }
            if (fogos[i].ativo) {
                fogos[i].restante -= dt;
                fogos[i].fase += dt;
                if (fogos[i].restante <= 0.0f) {
                    fogos[i].ativo = 0;
                    zumbi->ativo = 0;
                    continue;
                }
            } else {
                zumbi->ativo = 0;
                continue;
            }
        } else if (fogos[i].ativo) {
            fogos[i].ativo = 0;
            fogos[i].restante = 0.0f;
        }
        if (zumbi->tempoAtaque > 0.0f) zumbi->tempoAtaque -= dt;
        if (zumbi->tempoDano > 0.0f) zumbi->tempoDano -= dt;

        dx = jogadorX - zumbi->x;
        dz = jogadorZ - zumbi->z;
        distancia = sqrtf(dx * dx + dz * dz);
        if (distancia > 1.25f && distancia > 0.001f) {
            float passo = (zumbi->tipo ? 0.85f : 1.25f) * dt;
            float nx = zumbi->x + dx / distancia * passo;
            float nz = zumbi->z + dz / distancia * passo;
            if (blocoSolido((int)floorf(nx), (int)floorf(zumbi->y),
                            (int)floorf(zumbi->z)) == 0)
                zumbi->x = nx;
            if (blocoSolido((int)floorf(zumbi->x), (int)floorf(zumbi->y),
                            (int)floorf(nz)) == 0)
                zumbi->z = nz;
            zumbi->faseCaminhada += dt * 7.0f;
        } else if (zumbi->tempoAtaque <= 0.0f) {
            vidaJogador -= zumbi->tipo ? 1.0f : 2.0f;
            audioDano();
            zumbi->tempoAtaque = 1.2f;
        }
        ajustaAlturaNoTerreno(zumbi);
    }

    if (eNoite()) {
        tempoSpawn += dt;
        if (tempoSpawn >= 8.0f) {
            tempoSpawn = 0.0f;
            tentaSpawnarZumbi();
        }
    } else {
        tempoSpawn = 0.0f;
    }
    atualizaAlertaCachorro();
    atualizaSomFogoZumbis();
}

int atacaInimigo(void)
{
    int i, melhor = -1;
    float menorDistancia = 2.60f * 2.60f;
    float menorProjecao = 4.0f;
    float olhoX, olhoY, olhoZ;
    float raioX, raioY, raioZ;
    const float alcanceMira = 4.0f;

    if (tempoAtaqueJogador > 0.0f) return 0;

    /* Em terceira pessoa, o inimigo pode estar no centro da mira e ainda
       ficar além do raio medido a partir dos pés do jogador. Usa a mesma
       direção da câmera para que G e o clique atinjam o alvo visado. */
    posicaoDoOlho(&olhoX, &olhoY, &olhoZ);
    direcaoDoRaio(&raioX, &raioY, &raioZ);
    for (i = 0; i < MAX_ZUMBIS; i++) {
        Zumbi *zumbi = &zumbis[i];
        float dx, dy, dz, distancia;
        float projecao, distanciaMira;

        if (!zumbi->ativo || zumbi->tempoDano > 0.0f) continue;
        dx = zumbi->x - jogadorX;
        dy = zumbi->y + 0.8f - (jogadorY + 0.9f);
        dz = zumbi->z - jogadorZ;
        distancia = dx * dx + dy * dy + dz * dz;

        dx = zumbi->x - olhoX;
        dy = zumbi->y + 0.8f - olhoY;
        dz = zumbi->z - olhoZ;
        projecao = dx * raioX + dy * raioY + dz * raioZ;
        distanciaMira = dx * dx + dy * dy + dz * dz - projecao * projecao;

        if (projecao > 0.0f && projecao <= alcanceMira &&
            distanciaMira <= 0.75f * 0.75f &&
            projecao < menorProjecao) {
            menorProjecao = projecao;
            melhor = i;
            continue;
        }

        /* Também permite golpear um inimigo encostado, mesmo que a mira
           esteja ligeiramente deslocada durante a animação. */
        dx = zumbi->x - jogadorX;
        dy = zumbi->y + 0.8f - (jogadorY + 0.9f);
        dz = zumbi->z - jogadorZ;
        distancia = dx * dx + dy * dy + dz * dz;
        if (melhor < 0 && distancia <= menorDistancia) {
            menorDistancia = distancia;
            melhor = i;
        }
    }

    if (melhor < 0) return 0;
    tempoAtaqueJogador = 0.35f;
    if (tiposItens[itemSelecionado] == ESPADA && !gastaDurabilidade(ESPADA))
        return 0;

    zumbis[melhor].vida -= tiposItens[itemSelecionado] == ESPADA ? 4.0f : 2.0f;
    audioAtaque();
    audioZumbiAtingido();
    zumbis[melhor].tempoDano = 0.35f;
    if (zumbis[melhor].vida <= 0.0f) {
        audioZumbiMorreu();
        fogos[melhor].ativo = 0;
        zumbis[melhor].ativo = 0;
        criaDrop(zumbis[melhor].tipo ? CRISTAL : MACA,
                 zumbis[melhor].x, zumbis[melhor].y + 0.8f, zumbis[melhor].z);
    }
    return 1;
}

/* Os bracos partem do ombro em direcao ao eixo local -Z. A inclinacao
   permanece entre 68 e 88 graus: sempre para a frente e abaixo do ombro. */
static float anguloBracoZumbi(float fase, int lado)
{
    return 78.0f + (lado ? -10.0f : 10.0f) * sinf(fase);
}

void desenhaEntidades(void)
{
    int i;

    for (i = 0; i < MAX_ZUMBIS; i++) {
        Zumbi *zumbi = &zumbis[i];
        int corpo, cabeca;
        float passoPerna, dxJogador, dzJogador, anguloFrente;
        float vidaMaxima;
        if (!zumbi->ativo) continue;
        vidaMaxima = zumbi->tipo ? 8.0f : 12.0f;

        /* Silhueta de zumbi: pele verde, camisa azul-esverdeada e calça
           azul, com volumes separados para não parecer um bloco único. */
        corpo = CAMISA;
        cabeca = FOLHA;
        passoPerna = sinf(zumbi->faseCaminhada) * 32.0f;
        dxJogador = jogadorX - zumbi->x;
        dzJogador = jogadorZ - zumbi->z;
        /* A frente do modelo é o eixo local -Z. No OpenGL, a rotação em
           Y transforma esse eixo em (-sen(a), 0, -cos(a)). */
        anguloFrente = atan2f(-dxJogador, -dzJogador) * 180.0f / PI;

        /* O modelo usa -Z como frente; gira-o para olhar diretamente
           para o jogador, em vez de manter o rosto preso ao eixo do mapa. */
        glPushMatrix();
        glTranslatef(zumbi->x, zumbi->y, zumbi->z);
        glRotatef(anguloFrente, 0.0f, 1.0f, 0.0f);

        glPushMatrix();
        glTranslatef(-0.30f, 0.68f, -0.30f);
        glScalef(0.60f, 0.82f, 0.60f);
        desenhaBloco(corpo, 1.0f, TODAS_FACES);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-0.28f, 1.50f, -0.28f);
        glScalef(0.56f, 0.56f, 0.56f);
        desenhaBloco(cabeca, 1.0f, TODAS_FACES);
        glPopMatrix();

        /* Pivo no ombro: cubo inicialmente abaixo dele e girado para -Z. */
        glPushMatrix();
        glTranslatef(-0.43f, 1.34f, -0.08f);
        glRotatef(anguloBracoZumbi(zumbi->faseCaminhada, 0), 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.68f, -0.12f);
        glScalef(0.18f, 0.68f, 0.24f);
        desenhaBloco(corpo, 1.0f, TODAS_FACES);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.25f, 1.34f, -0.08f);
        glRotatef(anguloBracoZumbi(zumbi->faseCaminhada, 1), 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.68f, -0.12f);
        glScalef(0.18f, 0.68f, 0.24f);
        desenhaBloco(cabeca, 1.0f, TODAS_FACES);
        glPopMatrix();

        /* Pernas afastadas, com movimento oposto e botas escuras. */
        glPushMatrix();
        glTranslatef(-0.20f, 0.04f, -0.15f);
        glRotatef(-passoPerna, 1.0f, 0.0f, 0.0f);
        glScalef(0.20f, 0.66f, 0.24f);
        desenhaBloco(CALCA, 1.0f, TODAS_FACES);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.02f, 0.04f, -0.15f);
        glRotatef(passoPerna, 1.0f, 0.0f, 0.0f);
        glScalef(0.20f, 0.66f, 0.24f);
        desenhaBloco(CALCA, 1.0f, TODAS_FACES);
        glPopMatrix();

        /* Olhos escuros e boca na face voltada para -z. */
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glColor3f(0.015f, 0.02f, 0.02f);
        glBegin(GL_QUADS);
        glVertex3f(-0.18f, 1.77f, -0.286f);
        glVertex3f(-0.08f, 1.77f, -0.286f);
        glVertex3f(-0.08f, 1.87f, -0.286f);
        glVertex3f(-0.18f, 1.87f, -0.286f);
        glVertex3f(0.08f, 1.77f, -0.286f);
        glVertex3f(0.18f, 1.77f, -0.286f);
        glVertex3f(0.18f, 1.87f, -0.286f);
        glVertex3f(0.08f, 1.87f, -0.286f);
        glEnd();
        glColor3f(0.06f, 0.02f, 0.02f);
        glBegin(GL_QUADS);
        glVertex3f(-0.10f, 1.64f, -0.287f);
        glVertex3f(0.10f, 1.64f, -0.287f);
        glVertex3f(0.10f, 1.69f, -0.287f);
        glVertex3f(-0.10f, 1.69f, -0.287f);
        glEnd();
        glEnable(GL_CULL_FACE);

        /* Barra de vida: fica na frente da cabeça (-Z), fora do volume do
           modelo. O culling é desligado para continuar visível em qualquer
           ângulo da câmera. */
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glColor3f(0.06f, 0.01f, 0.01f);
        glBegin(GL_QUADS);
        glVertex3f(-0.5f, 2.12f, -0.32f);
        glVertex3f(0.5f, 2.12f, -0.32f);
        glVertex3f(0.5f, 2.20f, -0.32f);
        glVertex3f(-0.5f, 2.20f, -0.32f);
        glEnd();
        glColor3f(0.85f, 0.08f, 0.08f);
        if (zumbi->vida < 0.0f) zumbi->vida = 0.0f;
        if (zumbi->vida > vidaMaxima) zumbi->vida = vidaMaxima;
        glBegin(GL_QUADS);
        glVertex3f(-0.5f, 2.12f, -0.325f);
        glVertex3f(-0.5f + zumbi->vida / vidaMaxima, 2.12f, -0.325f);
        glVertex3f(-0.5f + zumbi->vida / vidaMaxima, 2.20f, -0.325f);
        glVertex3f(-0.5f, 2.20f, -0.325f);
        glEnd();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        if (fogos[i].ativo)
            desenhaFogoZumbi(&fogos[i]);
        glPopMatrix();
    }
}

static void caixaAnimal(float x, float y, float z, float largura, float altura,
                        float profundidade, float r, float g, float b)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(largura, altura, profundidade);
    glutSolidCube(1.0);
    glPopMatrix();
}

void desenhaAnimais(void)
{
    int i, perna;
    for (i = 0; i < NUM_ANIMAIS; i++) {
        Animal *a = &animais[i];
        float r, g, b, base, comprimento, largura, alturaPerna;
        if (!a->ativo) continue;
        r = i == ANIMAL_PORCO ? 0.94f : (i == ANIMAL_CACHORRO ? 0.72f : 0.78f);
        g = i == ANIMAL_PORCO ? 0.52f : (i == ANIMAL_CACHORRO ? 0.72f : 0.76f);
        b = i == ANIMAL_PORCO ? 0.55f : (i == ANIMAL_CACHORRO ? 0.70f : 0.72f);
        alturaPerna = i == ANIMAL_CAVALO ? 0.82f : 0.38f;
        base = alturaPerna + (i == ANIMAL_CAVALO ? 0.35f : 0.28f);
        comprimento = i == ANIMAL_CAVALO ? 1.25f : 0.85f;
        largura = i == ANIMAL_CAVALO ? 0.65f : 0.58f;
        glPushMatrix();
        glTranslatef(a->x, a->y, a->z);
        glRotatef(a->angulo, 0, 1, 0);
        glDisable(GL_TEXTURE_2D);
        caixaAnimal(0, base, 0, largura, i == ANIMAL_CAVALO ? 0.72f : 0.50f,
                    comprimento, r, g, b);
        for (perna = 0; perna < 4; perna++) {
            float lateral = (perna & 1) ? 0.20f : -0.20f;
            float frente = (perna & 2) ? 0.37f : -0.37f;
            float balan = sinf(a->fase) * (((perna == 0 || perna == 3) ? 1.0f : -1.0f) * 26.0f);
            glPushMatrix();
            glTranslatef(lateral, alturaPerna, frente * (comprimento / 0.85f));
            glRotatef(balan, 1, 0, 0);
            caixaAnimal(0, -alturaPerna / 2, 0, 0.20f, alturaPerna, 0.22f, r * 0.9f, g * 0.9f, b * 0.9f);
            glPopMatrix();
        }
        if (i == ANIMAL_CAVALO) {
            caixaAnimal(0, 1.47f, -0.48f, 0.44f, 0.88f, 0.45f, r, g, b);
            caixaAnimal(0, 1.85f, -0.74f, 0.48f, 0.42f, 0.62f, r, g, b);
            caixaAnimal(0, 1.70f, -1.04f, 0.39f, 0.25f, 0.25f, 0.43f, 0.41f, 0.39f);
            caixaAnimal(-0.11f, 1.69f, -1.17f, 0.07f, 0.06f, 0.02f, 0.18f, 0.17f, 0.16f);
            caixaAnimal(0.11f, 1.69f, -1.17f, 0.07f, 0.06f, 0.02f, 0.18f, 0.17f, 0.16f);
            caixaAnimal(-0.15f, 2.14f, -0.65f, 0.12f, 0.24f, 0.13f, r, g, b);
            caixaAnimal(0.15f, 2.14f, -0.65f, 0.12f, 0.24f, 0.13f, r, g, b);
            caixaAnimal(0, 1.59f, 0.05f, 0.68f, 0.10f, 0.58f, 0.36f, 0.18f, 0.08f);
            caixaAnimal(0, 1.89f, -0.37f, 0.16f, 0.60f, 0.20f, 0.30f, 0.28f, 0.27f);
            caixaAnimal(0, 1.35f, 0.71f, 0.16f, 0.48f, 0.18f, 0.32f, 0.30f, 0.28f);
            caixaAnimal(-0.25f, 1.90f, -0.88f, 0.04f, 0.09f, 0.09f, 0.02f, 0.02f, 0.02f);
            caixaAnimal(0.25f, 1.90f, -0.88f, 0.04f, 0.09f, 0.09f, 0.02f, 0.02f, 0.02f);
        } else {
            float cabecaY = i == ANIMAL_CACHORRO ? 0.86f : 0.75f;
            caixaAnimal(0, cabecaY, -0.55f, 0.55f, 0.50f, 0.42f, r, g, b);
            caixaAnimal(0, cabecaY - 0.08f, -0.79f, 0.30f, 0.20f, 0.18f,
                        i == ANIMAL_PORCO ? 0.90f : 0.80f, i == ANIMAL_PORCO ? 0.39f : 0.76f,
                        i == ANIMAL_PORCO ? 0.43f : 0.69f);
            caixaAnimal(-0.15f, cabecaY + 0.08f, -0.77f, 0.09f, 0.09f, 0.03f, 0.02f, 0.02f, 0.02f);
            caixaAnimal(0.15f, cabecaY + 0.08f, -0.77f, 0.09f, 0.09f, 0.03f, 0.02f, 0.02f, 0.02f);
            if (i == ANIMAL_PORCO) {
                caixaAnimal(-0.075f, cabecaY - 0.07f, -0.89f, 0.06f, 0.07f, 0.02f, 0.45f, 0.13f, 0.17f);
                caixaAnimal(0.075f, cabecaY - 0.07f, -0.89f, 0.06f, 0.07f, 0.02f, 0.45f, 0.13f, 0.17f);
            } else {
                caixaAnimal(0, cabecaY - 0.02f, -0.89f, 0.13f, 0.09f, 0.04f, 0.04f, 0.04f, 0.04f);
            }
            caixaAnimal(-0.20f, cabecaY + 0.31f, -0.51f, 0.14f, 0.18f, 0.17f, r * 0.75f, g * 0.75f, b * 0.75f);
            caixaAnimal(0.20f, cabecaY + 0.31f, -0.51f, 0.14f, 0.18f, 0.17f, r * 0.75f, g * 0.75f, b * 0.75f);
            if (i == ANIMAL_CACHORRO) {
                caixaAnimal(0, 0.67f, -0.30f, 0.60f, 0.16f, 0.13f, 0.65f, 0.05f, 0.05f);
                caixaAnimal(0, 0.70f, 0.55f, 0.13f, 0.15f, 0.40f, r, g, b);
            }
        }
        glPopMatrix();
    }
}
