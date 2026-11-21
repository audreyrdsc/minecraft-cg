#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "../include/audio.h"
#include "../include/blocks.h"

static float tempoProximoPasso = 0.0f;
static int estavaNaAgua = 0;
static int musicaNoiteTocando = 0;
static int musicaNoiteDisponivel = 0;
static int latidosPendentes = 0;
static int latidoDisponivel = 0;
static int latidoTocando = 0;
static int indiceGalope = -1;
static int fogoZumbiTocando = 0;
#ifdef _WIN32
typedef struct {
    char alias[32];
    HWAVEOUT saida;
    WAVEHDR buffer;
    unsigned char *dados;
} EfeitoAudio;
#define MAX_EFEITOS_AUDIO 24
static EfeitoAudio efeitosAudio[MAX_EFEITOS_AUDIO];
static int numeroEfeitosAudio = 0;

static EfeitoAudio *buscaEfeito(const char *alias)
{
    int i;
    for (i = 0; i < numeroEfeitosAudio; i++)
        if (strcmp(efeitosAudio[i].alias, alias) == 0) return &efeitosAudio[i];
    return NULL;
}

static void encerraEfeitosAudio(void)
{
    int i;
    for (i = 0; i < numeroEfeitosAudio; i++) {
        EfeitoAudio *efeito = &efeitosAudio[i];
        waveOutReset(efeito->saida);
        waveOutUnprepareHeader(efeito->saida, &efeito->buffer, sizeof(efeito->buffer));
        waveOutClose(efeito->saida);
        free(efeito->dados);
    }
    numeroEfeitosAudio = 0;
}

/* O ganho e aplicado apenas as amostras da musica, nunca ao dispositivo. */
#define MUSICA_NUM_BUFFERS 4
#define MUSICA_BUFFER_BYTES 8192
#define MUSICA_VOLUME_MAXIMO 0.35f
#define MUSICA_FADE_SEGUNDOS 60.0f
static HWAVEOUT musicaSaida = NULL;
static WAVEHDR musicaBuffers[MUSICA_NUM_BUFFERS];
static short musicaAmostras[MUSICA_NUM_BUFFERS][MUSICA_BUFFER_BYTES / 2];
static unsigned char *musicaDados = NULL;
static DWORD musicaTamanho = 0;
static int musicaBuffersPreparados = 0;
static HANDLE musicaThreadHandle = NULL;
static volatile LONG musicaThreadEncerrar = 0;
static volatile LONG musicaThreadAtiva = 0;
static volatile LONG musicaGanho = 0;

static DWORD WINAPI processaMusicaNoite(LPVOID parametro)
{
    DWORD posicao = 0;
    int enfileirado[MUSICA_NUM_BUFFERS] = {0};
    int estavaAtiva = 0;
    int i;
    (void)parametro;
    while (InterlockedCompareExchange(&musicaThreadEncerrar, 0, 0) == 0) {
        int ativa = InterlockedCompareExchange(&musicaThreadAtiva, 0, 0) != 0;
        if (ativa) {
            estavaAtiva = 1;
            for (i = 0; i < MUSICA_NUM_BUFFERS; i++) {
                WAVEHDR *buffer = &musicaBuffers[i];
                if (!enfileirado[i] || (buffer->dwFlags & WHDR_DONE)) {
                    LONG ganho = InterlockedCompareExchange(&musicaGanho, 0, 0);
                    unsigned j;
                    for (j = 0; j < MUSICA_BUFFER_BYTES / 2; j++) {
                        short amostra;
                        memcpy(&amostra, musicaDados + posicao, sizeof(amostra));
                        musicaAmostras[i][j] = (short)((amostra * ganho) / 32768);
                        posicao += sizeof(amostra);
                        if (posicao >= musicaTamanho) posicao = 0;
                    }
                    if (waveOutWrite(musicaSaida, buffer, sizeof(*buffer)) == MMSYSERR_NOERROR)
                        enfileirado[i] = 1;
                }
            }
        } else if (estavaAtiva) {
            waveOutReset(musicaSaida);
            memset(enfileirado, 0, sizeof(enfileirado));
            posicao = 0;
            estavaAtiva = 0;
        }
        Sleep(10);
    }
    waveOutReset(musicaSaida);
    return 0;
}

static void encerraMusicaNoite(void)
{
    int i;
    if (musicaThreadHandle) {
        InterlockedExchange(&musicaThreadEncerrar, 1);
        WaitForSingleObject(musicaThreadHandle, INFINITE);
        CloseHandle(musicaThreadHandle);
        musicaThreadHandle = NULL;
    }
    if (musicaSaida) {
        waveOutReset(musicaSaida);
        for (i = 0; i < musicaBuffersPreparados; i++)
            waveOutUnprepareHeader(musicaSaida, &musicaBuffers[i], sizeof(musicaBuffers[i]));
        waveOutClose(musicaSaida);
    }
    musicaSaida = NULL;
    musicaBuffersPreparados = 0;
    free(musicaDados);
    musicaDados = NULL;
    musicaNoiteDisponivel = 0;
    musicaNoiteTocando = 0;
}

static int inicializaMusicaNoite(void)
{
    wchar_t caminho[MAX_PATH];
    wchar_t *barra;
    DWORD tamanho = GetModuleFileNameW(NULL, caminho, MAX_PATH);
    HMMIO arquivo;
    MMCKINFO riff = {0}, formato = {0}, dados = {0};
    WAVEFORMATEX wave = {0};
    MMRESULT erro;
    int i;

    if (!tamanho || tamanho >= MAX_PATH) return 0;
    barra = wcsrchr(caminho, L'\\');
    if (!barra) return 0;
    *(barra + 1) = L'\0';
    if (wcslen(caminho) + wcslen(L"assets\\audio\\night.wav") >= MAX_PATH)
        return 0;
    wcscat(caminho, L"assets\\audio\\night.wav");
    arquivo = mmioOpenW(caminho, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (!arquivo) return 0;
    riff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    formato.ckid = mmioFOURCC('f', 'm', 't', ' ');
    dados.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(arquivo, &riff, NULL, MMIO_FINDRIFF) != 0 ||
        mmioDescend(arquivo, &formato, &riff, MMIO_FINDCHUNK) != 0 ||
        formato.cksize < 16 ||
        mmioRead(arquivo, (HPSTR)&wave, 16) != 16 ||
        wave.wFormatTag != WAVE_FORMAT_PCM || wave.wBitsPerSample != 16 ||
        (wave.nChannels != 1 && wave.nChannels != 2) ||
        wave.nBlockAlign != wave.nChannels * 2 ||
        mmioAscend(arquivo, &formato, 0) != 0 ||
        mmioDescend(arquivo, &dados, &riff, MMIO_FINDCHUNK) != 0 ||
        dados.cksize == 0 || dados.cksize > 0x7fffffffUL ||
        dados.cksize % wave.nBlockAlign != 0) {
        mmioClose(arquivo, 0);
        return 0;
    }
    musicaDados = (unsigned char *)malloc(dados.cksize);
    if (!musicaDados || mmioRead(arquivo, (HPSTR)musicaDados, (LONG)dados.cksize) != (LONG)dados.cksize) {
        mmioClose(arquivo, 0);
        encerraMusicaNoite();
        return 0;
    }
    mmioClose(arquivo, 0);
    musicaTamanho = dados.cksize;
    erro = waveOutOpen(&musicaSaida, WAVE_MAPPER, &wave, 0, 0, CALLBACK_NULL);
    if (erro != MMSYSERR_NOERROR) {
        encerraMusicaNoite();
        return 0;
    }
    memset(musicaBuffers, 0, sizeof(musicaBuffers));
    for (i = 0; i < MUSICA_NUM_BUFFERS; i++) {
        musicaBuffers[i].lpData = (LPSTR)musicaAmostras[i];
        musicaBuffers[i].dwBufferLength = MUSICA_BUFFER_BYTES;
        erro = waveOutPrepareHeader(musicaSaida, &musicaBuffers[i], sizeof(musicaBuffers[i]));
        if (erro != MMSYSERR_NOERROR) {
            encerraMusicaNoite();
            return 0;
        }
        musicaBuffersPreparados++;
    }
    musicaThreadEncerrar = 0;
    musicaThreadAtiva = 0;
    musicaGanho = 0;
    musicaThreadHandle = CreateThread(NULL, 0, processaMusicaNoite, NULL, 0, NULL);
    if (!musicaThreadHandle) {
        encerraMusicaNoite();
        return 0;
    }
    atexit(encerraMusicaNoite);
    return 1;
}

static void aplicaVolumeMusica(float volume)
{
    InterlockedExchange(&musicaGanho, (LONG)(volume * 32768.0f + 0.5f));
}
#endif

static int abreSom(const char *arquivo, const char *alias)
{
#ifdef _WIN32
    wchar_t caminho[MAX_PATH], relativo[MAX_PATH];
    DWORD lidos = GetModuleFileNameW(NULL, caminho, MAX_PATH);
    wchar_t *barra;
    HMMIO entrada;
    MMCKINFO riff = {0}, formato = {0}, dados = {0};
    WAVEFORMATEX wave = {0};
    EfeitoAudio *efeito;
    if (!lidos || lidos >= MAX_PATH || numeroEfeitosAudio >= MAX_EFEITOS_AUDIO) return 0;
    barra = wcsrchr(caminho, L'\\');
    if (!barra) return 0;
    *(barra + 1) = L'\0';
    mbstowcs(relativo, arquivo, MAX_PATH - 1);
    relativo[MAX_PATH - 1] = L'\0';
    for (wchar_t *p = relativo; *p; p++)
        if (*p == L'/') *p = L'\\';
    if (wcslen(caminho) + wcslen(relativo) >= MAX_PATH) return 0;
    wcscat(caminho, relativo);
    entrada = mmioOpenW(caminho, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (!entrada) return 0;
    riff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    formato.ckid = mmioFOURCC('f', 'm', 't', ' ');
    dados.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(entrada, &riff, NULL, MMIO_FINDRIFF) != 0 ||
        mmioDescend(entrada, &formato, &riff, MMIO_FINDCHUNK) != 0 ||
        formato.cksize < 16 || mmioRead(entrada, (HPSTR)&wave, 16) != 16 ||
        wave.wFormatTag != WAVE_FORMAT_PCM || wave.wBitsPerSample != 16 ||
        (wave.nChannels != 1 && wave.nChannels != 2) ||
        wave.nBlockAlign != wave.nChannels * 2 ||
        mmioAscend(entrada, &formato, 0) != 0 ||
        mmioDescend(entrada, &dados, &riff, MMIO_FINDCHUNK) != 0 ||
        !dados.cksize || dados.cksize > 0x7fffffffUL ||
        dados.cksize % wave.nBlockAlign != 0) {
        mmioClose(entrada, 0); return 0;
    }
    efeito = &efeitosAudio[numeroEfeitosAudio];
    memset(efeito, 0, sizeof(*efeito));
    efeito->dados = (unsigned char *)malloc(dados.cksize);
    if (!efeito->dados || mmioRead(entrada, (HPSTR)efeito->dados, (LONG)dados.cksize) != (LONG)dados.cksize) {
        mmioClose(entrada, 0); free(efeito->dados); return 0;
    }
    mmioClose(entrada, 0);
    if (waveOutOpen(&efeito->saida, WAVE_MAPPER, &wave, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        free(efeito->dados); return 0;
    }
    efeito->buffer.lpData = (LPSTR)efeito->dados;
    efeito->buffer.dwBufferLength = dados.cksize;
    if (waveOutPrepareHeader(efeito->saida, &efeito->buffer, sizeof(efeito->buffer)) != MMSYSERR_NOERROR) {
        waveOutClose(efeito->saida); free(efeito->dados); return 0;
    }
    snprintf(efeito->alias, sizeof(efeito->alias), "%s", alias);
    numeroEfeitosAudio++;
    return 1;
#else
    (void)arquivo;
    (void)alias;
    return 0;
#endif
}

static void tocaSom(const char *alias)
{
#ifdef _WIN32
    EfeitoAudio *efeito = buscaEfeito(alias);
    if (!efeito) return;
    waveOutReset(efeito->saida);
    waveOutWrite(efeito->saida, &efeito->buffer, sizeof(efeito->buffer));
#else
    (void)alias;
#endif
}

void inicializaAudio(void)
{
    tempoProximoPasso = 0.0f;
    estavaNaAgua = 0;
    musicaNoiteTocando = 0;
    musicaNoiteDisponivel = 0;
    latidosPendentes = 0;
    latidoTocando = 0;
    latidoDisponivel = 0;
    indiceGalope = -1;
    fogoZumbiTocando = 0;
#ifdef _WIN32
    encerraEfeitosAudio();
    atexit(encerraEfeitosAudio);
    abreSom("assets/audio/classic_hurt.wav", "sndHurt");
    abreSom("assets/audio/comer.wav", "sndEat");
    abreSom("assets/audio/bloco.wav", "sndBlockCollect");
    abreSom("assets/audio/damage.wav", "sndDamage");
    abreSom("assets/audio/walking.wav", "sndWalk");
    if (!abreSom("assets/audio/walking-other.wav", "sndWalkOther"))
        printf("Erro ao carregar o som de passos nas outras superficies.\n");
    abreSom("assets/audio/glass-break.wav", "sndGlass");
    abreSom("assets/audio/water-splash.wav", "sndWater");
    abreSom("assets/audio/zombie.wav", "sndZombie");
    abreSom("assets/audio/zombie-hit.wav", "sndZombieHit");
    abreSom("assets/audio/zombie-death.wav", "sndZombieDeath");
    if (!abreSom("assets/audio/horse-armor.wav", "sndHorse"))
        printf("Erro ao carregar o som da montaria.\n");
    if (!abreSom("assets/audio/horse-angry.wav", "sndHorseAngry"))
        printf("Erro ao carregar o som de proximidade do cavalo.\n");
    if (!abreSom("assets/audio/pig-proximo.wav", "sndPig"))
        printf("Erro ao carregar o som de proximidade do porco.\n");
    latidoDisponivel = abreSom("assets/audio/dog-bark-double.wav", "sndDog");
    if (!latidoDisponivel) printf("Erro ao carregar o latido do cachorro.\n");
    abreSom("assets/audio/horse-gallop1.wav", "sndGallop1");
    abreSom("assets/audio/horse-gallop2.wav", "sndGallop2");
    abreSom("assets/audio/horse-gallop3.wav", "sndGallop3");
    abreSom("assets/audio/horse-gallop4.wav", "sndGallop4");
    if (!abreSom("assets/audio/zombie-fire.wav", "sndZombieFire"))
        printf("Erro ao carregar o som do zumbi queimando.\n");
    musicaNoiteDisponivel = inicializaMusicaNoite();
    if (musicaNoiteDisponivel) {
        printf("Musica noturna carregada.\n");
    } else {
        printf("Erro ao inicializar musica noturna em assets/audio/night.wav.\n");
    }
    fflush(stdout);
#endif
}

void audioQuebra(void) { tocaSom("sndDamage"); }
void audioQuebraBloco(int tipo)
{
    if (tipo == VIDRO)
        tocaSom("sndGlass");
    else               tocaSom("sndDamage");
}
void audioColeta(void) { tocaSom("sndBlockCollect"); }
void audioDano(void)
{
    tocaSom("sndHurt");
}
void audioAtaque(void) { tocaSom("sndDamage"); }
void audioZumbiAtingido(void) { tocaSom("sndZombieHit"); }
void audioZumbiMorreu(void)
{
    tocaSom("sndZombieDeath");
}
void audioComer(void) { tocaSom("sndEat"); }

static int usaSomPassoGrama(int tipo)
{
    return tipo == GRAMA || tipo == PEDRA || tipo == TERRA || tipo == AREIA || tipo == NEVE ||
           tipo == TRONCO || tipo == TRONCO_CEREJEIRA;
}

void audioAtualizaPassos(float dt, int andando, int blocoApoio)
{
    if (!andando) {
        tempoProximoPasso = 0.0f;
        return;
    }
    tempoProximoPasso -= dt;
    if (tempoProximoPasso <= 0.0f) {
        int somGrama = usaSomPassoGrama(blocoApoio);
        tocaSom(somGrama ? "sndWalk" : "sndWalkOther");
        /* O novo passo de grama tem cerca de 0,5 s; nao corta seu final. */
        tempoProximoPasso = somGrama ? 0.50f : 0.38f;
    }
}

void audioAtualizaAgua(int dentroDaAgua)
{
    if (dentroDaAgua && !estavaNaAgua)
        tocaSom("sndWater");
    estavaNaAgua = dentroDaAgua ? 1 : 0;
}

void audioAtualizaNoite(float tempoMundo, float duracaoDia)
{
#ifdef _WIN32
    int noite = tempoMundo < 0.25f || tempoMundo >= 0.75f;
    float intensidade = 1.0f;
    if (!musicaNoiteDisponivel) return;
    if (noite && tempoMundo < 0.25f && duracaoDia > 0.0f) {
        intensidade = (0.25f - tempoMundo) * duracaoDia / MUSICA_FADE_SEGUNDOS;
        if (intensidade > 1.0f) intensidade = 1.0f;
        if (intensidade < 0.0f) intensidade = 0.0f;
    }
    aplicaVolumeMusica(noite ? MUSICA_VOLUME_MAXIMO * intensidade : 0.0f);
    InterlockedExchange(&musicaThreadAtiva, noite);
    musicaNoiteTocando = noite;
#else
    (void)tempoMundo;
    (void)duracaoDia;
#endif
}

void audioSpawnZumbi(void) { tocaSom("sndZombie"); }
void audioMontaria(void) { tocaSom("sndHorse"); }
void audioCavaloProximo(void) { tocaSom("sndHorseAngry"); }
void audioPorcoProximo(void) { tocaSom("sndPig"); }

void audioAtualizaFogoZumbi(int queimando)
{
#ifdef _WIN32
    EfeitoAudio *fogo = buscaEfeito("sndZombieFire");
    if (!fogo) return;
    if (!queimando) {
        if (fogoZumbiTocando) waveOutReset(fogo->saida);
        fogoZumbiTocando = 0;
    } else if (!fogoZumbiTocando || (fogo->buffer.dwFlags & WHDR_DONE)) {
        tocaSom("sndZombieFire");
        fogoZumbiTocando = 1;
    }
#else
    (void)queimando;
#endif
}

void audioAtualizaGalope(int andando)
{
#ifdef _WIN32
    static const char *nomes[] = {"sndGallop1", "sndGallop2", "sndGallop3", "sndGallop4"};
    EfeitoAudio *atual = indiceGalope >= 0 ? buscaEfeito(nomes[indiceGalope]) : NULL;
    if (!andando) {
        if (atual) waveOutReset(atual->saida);
        indiceGalope = -1;
        return;
    }
    if (!atual || (atual->buffer.dwFlags & WHDR_DONE)) {
        indiceGalope = (indiceGalope + 1) % 4;
        tocaSom(nomes[indiceGalope]);
    }
#else
    (void)andando;
#endif
}

/* Cada arquivo contem dois latidos consecutivos. Alertas simultaneos entram
   numa fila para que o proximo inimigo nao interrompa a dupla anterior. */
void audioAlertaCachorro(void)
{
    if (latidoDisponivel && latidosPendentes < 32) latidosPendentes++;
    audioAtualizaLatidos();
}

void audioAtualizaLatidos(void)
{
#ifdef _WIN32
    if (latidoTocando) {
        EfeitoAudio *latido = buscaEfeito("sndDog");
        if (!latido || (latido->buffer.dwFlags & WHDR_DONE))
            latidoTocando = 0;
    }
    if (!latidoTocando && latidosPendentes > 0) {
        latidosPendentes--;
        tocaSom("sndDog");
        latidoTocando = 1;
    }
#endif
}
