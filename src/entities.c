#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <math.h>
#include <stdio.h>
#include "../include/entities.h"
#include "../include/inventory.h"
#include "../include/worldgen.h"

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

static Zumbi zumbis[MAX_ZUMBIS];
static float tempoSpawn = 0.0f;
static int sequenciaSpawn = 0;
static float tempoAtaqueJogador = 0.0f;

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
    return fread(zumbis, sizeof(zumbis), 1, arquivo) == 1 &&
           fread(&tempoSpawn, sizeof(tempoSpawn), 1, arquivo) == 1 &&
           fread(&sequenciaSpawn, sizeof(sequenciaSpawn), 1, arquivo) == 1 &&
           fread(&tempoAtaqueJogador, sizeof(tempoAtaqueJogador), 1, arquivo) == 1 &&
           fread(drops, sizeof(drops), 1, arquivo) == 1;
}

static int eNoite(void)
{
    return sinf(tempoMundo * 2.0f * PI) < 0.0f;
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
            zumbi->ativo = 0;
            continue;
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
        zumbis[melhor].ativo = 0;
        criaDrop(zumbis[melhor].tipo ? CRISTAL : MACA,
                 zumbis[melhor].x, zumbis[melhor].y + 0.8f, zumbis[melhor].z);
    }
    return 1;
}

void desenhaEntidades(void)
{
    int i;

    for (i = 0; i < MAX_ZUMBIS; i++) {
        Zumbi *zumbi = &zumbis[i];
        int corpo, cabeca;
        float passoBraco, passoPerna, dxJogador, dzJogador, anguloFrente;
        float vidaMaxima;
        if (!zumbi->ativo) continue;
        vidaMaxima = zumbi->tipo ? 8.0f : 12.0f;

        /* Silhueta de zumbi: pele verde, camisa azul-esverdeada e calça
           azul, com volumes separados para não parecer um bloco único. */
        corpo = CAMISA;
        cabeca = FOLHA;
        passoBraco = sinf(zumbi->faseCaminhada) * 25.0f;
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

        /* Braços alternados, com mãos verdes maiores e balanço visível. */
        glPushMatrix();
        glTranslatef(-0.43f, 1.34f, -0.15f);
        glRotatef(passoBraco, 1.0f, 0.0f, 0.0f);
        glScalef(0.18f, 0.68f, 0.24f);
        desenhaBloco(corpo, 1.0f, TODAS_FACES);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.25f, 1.34f, -0.15f);
        glRotatef(-passoBraco, 1.0f, 0.0f, 0.0f);
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
        glPopMatrix();
    }
}
