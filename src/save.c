#include <stdio.h>
#include <string.h>
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
    cabecalho.versao = 5;
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
        !salvaEstadoEntidades(arquivo)) {
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

    if (!arquivo) return 0;
    if (fread(&cabecalho, sizeof(cabecalho), 1, arquivo) != 1 ||
        memcmp(cabecalho.assinatura, "MCGSV01", 7) != 0 ||
        cabecalho.versao != 5 ||
        fread(mundo, sizeof(mundo), 1, arquivo) != 1 ||
        fread(&inventario, sizeof(inventario), 1, arquivo) != 1 ||
        !carregaEstadoEntidades(arquivo)) {
        fclose(arquivo);
        return 0;
    }
    fclose(arquivo);

    defineSementeMundo(cabecalho.seed);
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
    itemSelecionado = cabecalho.itemSelecionado;
    defineChuva(cabecalho.chuva);
    invalidaCacheFacesTodos();
    marcaTodosChunksSujo();
    sincronizaHotbar();
    reconstroiLuzes();
    return 1;
}
