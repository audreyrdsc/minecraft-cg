#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "../include/audio.h"
#include "../include/blocks.h"

static float tempoProximoPasso = 0.0f;
static int estavaNaAgua = 0;
static int musicaNoiteTocando = 0;

static void abreSom(const char *arquivo, const char *alias)
{
#ifdef _WIN32
    wchar_t executavel[MAX_PATH];
    wchar_t relativo[MAX_PATH];
    wchar_t caminho[MAX_PATH];
    wchar_t comando[768];
    DWORD lidos = GetModuleFileNameW(NULL, executavel, MAX_PATH);
    wchar_t *barra;
    MCIERROR erro;

    if (lidos == 0 || lidos >= MAX_PATH) return;
    executavel[lidos] = L'\0';
    barra = wcsrchr(executavel, L'\\');
    if (!barra) return;
    *barra = L'\0';
    mbstowcs(relativo, arquivo, MAX_PATH - 1);
    relativo[MAX_PATH - 1] = L'\0';
    for (wchar_t *p = relativo; *p; p++)
        if (*p == L'/') *p = L'\\';
    swprintf(caminho, MAX_PATH, L"%ls\\%ls", executavel, relativo);
    swprintf(comando, 768, L"open \"%ls\" type waveaudio alias %hs",
             caminho, alias);
    erro = mciSendStringW(comando, NULL, 0, NULL);
    if (erro != 0 && strcmp(alias, "sndNightMusic") == 0) {
        char mensagem[256];
        mciGetErrorStringA(erro, mensagem, sizeof(mensagem));
        printf("Erro ao abrir musica noturna: %s\n", mensagem);
        fflush(stdout);
    }
#else
    (void)arquivo;
    (void)alias;
#endif
}

static void tocaSom(const char *alias)
{
#ifdef _WIN32
    char comando[96];
    sprintf(comando, "seek %s to start", alias);
    mciSendStringA(comando, NULL, 0, NULL);
    sprintf(comando, "play %s from 0", alias);
    mciSendStringA(comando, NULL, 0, NULL);
#else
    (void)alias;
#endif
}

static void tocaArquivoDireto(const wchar_t *relativo)
{
#ifdef _WIN32
    wchar_t caminho[MAX_PATH];
    DWORD lidos = GetModuleFileNameW(NULL, caminho, MAX_PATH);
    wchar_t *barra;

    if (lidos == 0 || lidos >= MAX_PATH) return;
    barra = wcsrchr(caminho, L'\\');
    if (!barra) return;
    *(barra + 1) = L'\0';
    wcsncat(caminho, relativo,
            MAX_PATH - wcslen(caminho) - 1);
    PlaySoundW(caminho, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
#else
    (void)relativo;
#endif
}

void inicializaAudio(void)
{
    tempoProximoPasso = 0.0f;
    estavaNaAgua = 0;
    musicaNoiteTocando = 0;
#ifdef _WIN32
    abreSom("assets/audio/classic_hurt.wav", "sndHurt");
    abreSom("assets/audio/comer.wav", "sndEat");
    abreSom("assets/audio/bloco.wav", "sndBlockCollect");
    abreSom("assets/audio/damage.wav", "sndDamage");
    abreSom("assets/audio/walking.wav", "sndWalk");
    abreSom("assets/audio/glass-break.wav", "sndGlass");
    abreSom("assets/audio/water-splash.wav", "sndWater");
    abreSom("assets/audio/zombie.wav", "sndZombie");
    abreSom("assets/audio/zombie-hit.wav", "sndZombieHit");
    abreSom("assets/audio/zombie-death.wav", "sndZombieDeath");
    abreSom("assets/audio/night.wav", "sndNightMusic");
    mciSendStringA("setaudio sndNightMusic volume to 350", NULL, 0, NULL);
#endif
}

void audioQuebra(void) { tocaSom("sndDamage"); }
void audioQuebraBloco(int tipo)
{
    if (tipo == VIDRO)
        tocaArquivoDireto(L"assets\\audio\\glass-break.wav");
    else               tocaSom("sndDamage");
}
void audioColeta(void) { tocaSom("sndBlockCollect"); }
void audioDano(void)
{
    tocaArquivoDireto(L"assets\\audio\\classic_hurt.wav");
}
void audioAtaque(void) { tocaSom("sndDamage"); }
void audioZumbiAtingido(void) { tocaSom("sndZombieHit"); }
void audioZumbiMorreu(void)
{
    tocaArquivoDireto(L"assets\\audio\\zombie-death.wav");
}
void audioComer(void) { tocaSom("sndEat"); }

void audioAtualizaPassos(float dt, int andando)
{
    if (!andando) {
        tempoProximoPasso = 0.0f;
        return;
    }
    tempoProximoPasso -= dt;
    if (tempoProximoPasso <= 0.0f) {
        tocaSom("sndWalk");
        tempoProximoPasso = 0.38f;
    }
}

void audioAtualizaAgua(int dentroDaAgua)
{
    if (dentroDaAgua && !estavaNaAgua)
        tocaArquivoDireto(L"assets\\audio\\water-splash.wav");
    estavaNaAgua = dentroDaAgua ? 1 : 0;
}

void audioAtualizaNoite(int noite)
{
#ifdef _WIN32
    if (noite && !musicaNoiteTocando) {
        mciSendStringA("seek sndNightMusic to start", NULL, 0, NULL);
        mciSendStringA("play sndNightMusic repeat", NULL, 0, NULL);
        musicaNoiteTocando = 1;
    } else if (!noite && musicaNoiteTocando) {
        mciSendStringA("stop sndNightMusic", NULL, 0, NULL);
        musicaNoiteTocando = 0;
    }
#else
    (void)noite;
#endif
}

void audioSpawnZumbi(void) { tocaSom("sndZombie"); }
