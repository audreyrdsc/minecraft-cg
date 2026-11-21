#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "../include/save.h"
#include "../include/world.h"
#include "../include/inventory.h"
#include "../include/worldgen.h"
#include "../include/effects.h"
#include "../include/entities.h"
#include "../include/chunks.h"

#include "../include/config.h"

extern float jogadorX, jogadorY, jogadorZ;
extern float yaw, pitch;
extern float vidaJogador, fomeJogador, saturacaoJogador;
extern float tempoRegeneracaoJogador;
extern float tempoMundo;
extern int itemSelecionado;
extern void reconstroiLuzes(void);
extern void construirMundo(void);

typedef struct {
    char assinatura[8];
    unsigned int versao;
    unsigned int seed;
    float jogadorX, jogadorY, jogadorZ;
    float yaw, pitch;
    float vida, fome, saturacao, tempo;
    float tempoRegeneracao;
    int itemSelecionado;
    int chuva;
} CabecalhoSave;

/* Nao substitui uma copia anterior do save legado. */
static int preservaSaveAntigo(FILE *original, const char *caminho, unsigned int versao)
{
    char nome[512];
    unsigned char buffer[4096];
    size_t lidos;
    FILE *backup;
    int ok = 1;
    int fd;
    if (strlen(caminho) + 16 >= sizeof(nome)) return 0;
    snprintf(nome, sizeof(nome), "%s.v%u.bak", caminho, versao);
    backup = fopen(nome, "rb");
    if (backup) { fclose(backup); return 1; }
#ifdef _WIN32
    fd = _open(nome, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd < 0) return 0;
    backup = _fdopen(fd, "wb");
    if (!backup) { _close(fd); remove(nome); return 0; }
#else
    fd = open(nome, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) return 0;
    backup = fdopen(fd, "wb");
    if (!backup) { close(fd); remove(nome); return 0; }
#endif
    rewind(original);
    while ((lidos = fread(buffer, 1, sizeof(buffer), original)) > 0)
        if (fwrite(buffer, 1, lidos, backup) != lidos) { ok = 0; break; }
    if (ferror(original)) ok = 0;
    if (fclose(backup) != 0) ok = 0;
    if (!ok) remove(nome); /* apenas arquivo criado por esta chamada */
    return ok;
}

int salvaJogo(const char *caminho)
{
    char caminhoTemporario[512];
    char caminhoBackup[512];
    FILE *arquivo;
    int backupCriado = 0;
    CabecalhoSave cabecalho;

    if (!caminho || strlen(caminho) >= sizeof(caminhoTemporario) - 5)
        return 0;
    snprintf(caminhoTemporario, sizeof(caminhoTemporario), "%s.tmp", caminho);
    snprintf(caminhoBackup, sizeof(caminhoBackup), "%s.bak", caminho);
    arquivo = fopen(caminhoTemporario, "wb");
    if (!arquivo) return 0;
    memset(&cabecalho, 0, sizeof(cabecalho));
    memcpy(cabecalho.assinatura, "MCGSV01", 7);
    cabecalho.versao = 7;
    cabecalho.seed = sementeMundo;
    cabecalho.jogadorX = jogadorX;
    cabecalho.jogadorY = jogadorY;
    cabecalho.jogadorZ = jogadorZ;
    cabecalho.yaw = yaw;
    cabecalho.pitch = pitch;
    cabecalho.vida = vidaJogador;
    cabecalho.fome = fomeJogador;
    cabecalho.saturacao = saturacaoJogador;
    cabecalho.tempo = tempoMundo;
    cabecalho.itemSelecionado = itemSelecionado;
    cabecalho.chuva = chuvaEstaAtiva();
    cabecalho.tempoRegeneracao = tempoRegeneracaoJogador;

    if (fwrite(&cabecalho, sizeof(cabecalho), 1, arquivo) != 1 ||
        fwrite(mundo, sizeof(mundo), 1, arquivo) != 1 ||
        fwrite(&inventario, sizeof(inventario), 1, arquivo) != 1 ||
        !salvaEstadoEntidades(arquivo) ||
        !salvaEstadoAnimais(arquivo)) {
        fclose(arquivo);
        remove(caminhoTemporario);
        return 0;
    }
    if (fclose(arquivo) != 0) {
        remove(caminhoTemporario);
        return 0;
    }
    remove(caminhoBackup);
    if (rename(caminho, caminhoBackup) == 0)
        backupCriado = 1;
    if (rename(caminhoTemporario, caminho) != 0) {
        remove(caminhoTemporario);
        if (backupCriado) rename(caminhoBackup, caminho);
        return 0;
    }
    if (backupCriado) remove(caminhoBackup);
    return 1;
}

int carregaJogo(const char *caminho)
{
    FILE *arquivo = fopen(caminho, "rb");
    CabecalhoSave cabecalho;
    Inventory novoInventario;
    unsigned char *blocos;
    size_t tamanhoBlocos;
    int x, y, z;

    if (!arquivo) return 0;
    if (fread(&cabecalho, sizeof(cabecalho), 1, arquivo) != 1 ||
        memcmp(cabecalho.assinatura, "MCGSV01", 7) != 0 ||
        (cabecalho.versao != 5 && cabecalho.versao != 6 && cabecalho.versao != 7)) {
        fclose(arquivo);
        return 0;
    }
    tamanhoBlocos = cabecalho.versao == 5 ? 64u * ALTURA_MUNDO * 64u : sizeof(mundo);
    blocos = (unsigned char *)malloc(tamanhoBlocos);
    if (!blocos || fread(blocos, tamanhoBlocos, 1, arquivo) != 1 ||
        fread(&novoInventario, sizeof(novoInventario), 1, arquivo) != 1 ||
        !carregaEstadoEntidades(arquivo) ||
        (cabecalho.versao >= 6 && !carregaEstadoAnimais(arquivo))) {
        free(blocos);
        fclose(arquivo);
        return 0;
    }
    if (cabecalho.versao < 7 && !preservaSaveAntigo(arquivo, caminho, cabecalho.versao)) {
        free(blocos);
        fclose(arquivo);
        return 0;
    }
    fclose(arquivo);

    defineSementeMundo(cabecalho.seed);
    if (cabecalho.versao == 5) {
        construirMundo();
        for (x = 0; x < 64; x++)
            for (y = 0; y < ALTURA_MUNDO; y++)
                memcpy(&mundo[x][y][0], blocos + (x * ALTURA_MUNDO + y) * 64, 64);
    } else if (cabecalho.versao == 6) {
        unsigned char alterado[NUM_CHUNKS_X][NUM_CHUNKS_Z] = {{0}};
        /* Reconstrucao da geracao anterior: qualquer chunk que nao coincida
           integralmente com ela e preservado, inclusive espacos escavados. */
        defineGeracaoLegada(1);
        construirMundo();
        defineGeracaoLegada(0);
        for (x = 0; x < LADO; x++)
            for (y = 0; y < ALTURA_MUNDO; y++)
                for (z = 0; z < LADO; z++)
                    if (mundo[x][y][z] != blocos[(x * ALTURA_MUNDO + y) * LADO + z])
                        alterado[x / CHUNK_LADO][z / CHUNK_LADO] = 1;
        /* Nao alterar o terreno imediatamente ocupado pelo jogador salvo. */
        if (isfinite(cabecalho.jogadorX) && isfinite(cabecalho.jogadorZ) &&
            cabecalho.jogadorX >= 0 && cabecalho.jogadorX < LADO &&
            cabecalho.jogadorZ >= 0 && cabecalho.jogadorZ < LADO) {
            int ox, oz;
            for (ox = -1; ox <= 1; ox++)
                for (oz = -1; oz <= 1; oz++) {
                    int px = (int)floorf(cabecalho.jogadorX + ox);
                    int pz = (int)floorf(cabecalho.jogadorZ + oz);
                    if (px >= 0 && px < LADO && pz >= 0 && pz < LADO)
                        alterado[px / CHUNK_LADO][pz / CHUNK_LADO] = 1;
                }
        }
        preservaChunksEntidades(alterado);
        construirMundo();
        for (x = 0; x < LADO; x++)
            for (y = 0; y < ALTURA_MUNDO; y++)
                for (z = 0; z < LADO; z++)
                    if (alterado[x / CHUNK_LADO][z / CHUNK_LADO])
                        mundo[x][y][z] = blocos[(x * ALTURA_MUNDO + y) * LADO + z];
    } else {
        memcpy(mundo, blocos, sizeof(mundo));
    }
    free(blocos);
    inventario = novoInventario;
    jogadorX = cabecalho.jogadorX;
    jogadorY = cabecalho.jogadorY;
    jogadorZ = cabecalho.jogadorZ;
    yaw = cabecalho.yaw;
    pitch = cabecalho.pitch;
    vidaJogador = cabecalho.vida;
    fomeJogador = cabecalho.fome;
    saturacaoJogador = cabecalho.saturacao;
    tempoRegeneracaoJogador = cabecalho.tempoRegeneracao;
    tempoMundo = cabecalho.tempo;
    if (cabecalho.versao == 5) iniciaAnimais();
    itemSelecionado = cabecalho.itemSelecionado;
    defineChuva(cabecalho.chuva);
    invalidaCacheFacesTodos();
    marcaTodosChunksSujo();
    sincronizaHotbar();
    reconstroiLuzes();
    return 1;
}
