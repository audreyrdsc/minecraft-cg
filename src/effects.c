#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <math.h>
#include "../include/effects.h"
#include "../include/worldgen.h"

#define MAX_CHUVA 96
#define MAX_PARTICULAS 96

extern float jogadorX, jogadorY, jogadorZ;

typedef struct {
    float x, y, z;
    float velocidade;
    int ativo;
} Gota;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float vida;
    int ativo;
} Particula;

static Gota chuva[MAX_CHUVA];
static Particula particulas[MAX_PARTICULAS];
static int chuvaAtiva = 0;
static unsigned int proximoAleatorio = 928371u;

static int estaNevando(void)
{
    return biomaProcedural((int)floorf(jogadorX),
                           (int)floorf(jogadorZ)) == 2;
}

static float aleatorio(void)
{
    proximoAleatorio = proximoAleatorio * 1664525u + 1013904223u;
    return (proximoAleatorio & 0x00ffffffu) / 16777215.0f;
}

void iniciaEfeitos(void)
{
    int i;
    chuvaAtiva = 0;
    for (i = 0; i < MAX_CHUVA; i++) chuva[i].ativo = 0;
    for (i = 0; i < MAX_PARTICULAS; i++) particulas[i].ativo = 0;
}

void alternaChuva(void)
{
    chuvaAtiva = !chuvaAtiva;
}

int chuvaEstaAtiva(void)
{
    return chuvaAtiva;
}

void defineChuva(int ativa)
{
    int i;
    chuvaAtiva = ativa ? 1 : 0;
    if (!chuvaAtiva)
        for (i = 0; i < MAX_CHUVA; i++) chuva[i].ativo = 0;
}

static void atualizaChuva(float dt)
{
    int i;
    if (!chuvaAtiva) return;

    for (i = 0; i < MAX_CHUVA; i++) {
        if (!chuva[i].ativo || chuva[i].y < jogadorY - 2.0f) {
            chuva[i].ativo = 1;
            chuva[i].x = jogadorX + (aleatorio() - 0.5f) * 24.0f;
            chuva[i].z = jogadorZ + (aleatorio() - 0.5f) * 24.0f;
            chuva[i].y = jogadorY + 10.0f + aleatorio() * 6.0f;
            chuva[i].velocidade = estaNevando()
                                ? 3.0f + aleatorio() * 3.0f
                                : 12.0f + aleatorio() * 5.0f;
        }
        chuva[i].y -= chuva[i].velocidade * dt;
    }
}

void emiteParticulasBloco(float x, float y, float z, int tipo)
{
    int i, criadas = 0;
    (void)tipo;

    for (i = 0; i < MAX_PARTICULAS && criadas < 8; i++) {
        if (!particulas[i].ativo) {
            Particula *p = &particulas[i];
            p->ativo = 1;
            p->x = x;
            p->y = y;
            p->z = z;
            p->vx = (aleatorio() - 0.5f) * 3.0f;
            p->vy = 1.5f + aleatorio() * 2.0f;
            p->vz = (aleatorio() - 0.5f) * 3.0f;
            p->vida = 0.35f + aleatorio() * 0.35f;
            criadas++;
        }
    }
}

static void atualizaParticulas(float dt)
{
    int i;
    for (i = 0; i < MAX_PARTICULAS; i++) {
        Particula *p = &particulas[i];
        if (!p->ativo) continue;
        p->vida -= dt;
        p->vy -= 12.0f * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->z += p->vz * dt;
        if (p->vida <= 0.0f) p->ativo = 0;
    }
}

void atualizaEfeitos(float dt)
{
    atualizaChuva(dt);
    atualizaParticulas(dt);
}

void desenhaEfeitos(void)
{
    int i;
    glDisable(GL_TEXTURE_2D);

    if (chuvaAtiva) {
        if (estaNevando()) {
            glPointSize(3.0f);
            glColor3f(0.96f, 0.98f, 1.0f);
            glBegin(GL_POINTS);
            for (i = 0; i < MAX_CHUVA; i++)
                if (chuva[i].ativo)
                    glVertex3f(chuva[i].x, chuva[i].y, chuva[i].z);
            glEnd();
            glPointSize(1.0f);
        } else {
            glColor3f(0.45f, 0.65f, 0.95f);
            glBegin(GL_LINES);
            for (i = 0; i < MAX_CHUVA; i++)
                if (chuva[i].ativo) {
                    glVertex3f(chuva[i].x, chuva[i].y, chuva[i].z);
                    glVertex3f(chuva[i].x, chuva[i].y - 0.45f, chuva[i].z);
                }
            glEnd();
        }
    }

    glPointSize(4.0f);
    glColor3f(0.78f, 0.58f, 0.26f);
    glBegin(GL_POINTS);
    for (i = 0; i < MAX_PARTICULAS; i++)
        if (particulas[i].ativo)
            glVertex3f(particulas[i].x, particulas[i].y, particulas[i].z);
    glEnd();
    glPointSize(1.0f);
}
