#include "../include/crafting.h"

void atualizaReceita(void)
{
    int i, areia = 0, tronco = 0, folhas = 0, pedra = 0, outros = 0;

    resultadoCrafting.tipo = VAZIO;
    resultadoCrafting.quantidade = 0;
    for (i = 0; i < 4; i++) {
        if (gradeCrafting[i].tipo == AREIA) areia += gradeCrafting[i].quantidade;
        else if (gradeCrafting[i].tipo == TRONCO) tronco += gradeCrafting[i].quantidade;
        else if (gradeCrafting[i].tipo == FOLHA) folhas += gradeCrafting[i].quantidade;
        else if (gradeCrafting[i].tipo == PEDRA) pedra += gradeCrafting[i].quantidade;
        else if (gradeCrafting[i].quantidade > 0) outros += gradeCrafting[i].quantidade;
    }

    if (outros == 0 && areia == 2 && tronco == 0 && folhas == 0) {
        resultadoCrafting.tipo = VIDRO;
        resultadoCrafting.quantidade = 1;
    } else if (outros == 0 && areia == 0 && tronco == 1 && folhas == 1) {
        resultadoCrafting.tipo = TOCHA;
        resultadoCrafting.quantidade = 4;
    } else if (outros == 0 && areia == 0 && tronco == 1 && folhas == 0 &&
               pedra == 2) {
        resultadoCrafting.tipo = PICARETA;
        resultadoCrafting.quantidade = 1;
    }
}

void limpaCrafting(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        gradeCrafting[i].tipo = VAZIO;
        gradeCrafting[i].quantidade = 0;
    }
    resultadoCrafting.tipo = VAZIO;
    resultadoCrafting.quantidade = 0;
}

void consomeReceita(void)
{
    int i, areia = 0, tronco = 0, folhas = 0, pedra = 0;
    for (i = 0; i < 4; i++) {
        if (gradeCrafting[i].tipo == AREIA) areia += gradeCrafting[i].quantidade;
        if (gradeCrafting[i].tipo == TRONCO) tronco += gradeCrafting[i].quantidade;
        if (gradeCrafting[i].tipo == FOLHA) folhas += gradeCrafting[i].quantidade;
        if (gradeCrafting[i].tipo == PEDRA) pedra += gradeCrafting[i].quantidade;
    }

    if (areia == 2 && tronco == 0 && folhas == 0) {
        for (i = 0; i < 4; i++)
            if (gradeCrafting[i].tipo == AREIA && gradeCrafting[i].quantidade > 0) {
                gradeCrafting[i].quantidade--;
                areia--;
                if (gradeCrafting[i].quantidade == 0) gradeCrafting[i].tipo = VAZIO;
                if (areia == 0) break;
            }
    } else if (areia == 0 && tronco == 1 && folhas == 1) {
        for (i = 0; i < 4; i++)
            if (gradeCrafting[i].tipo == TRONCO) {
                gradeCrafting[i].tipo = VAZIO;
                gradeCrafting[i].quantidade = 0;
                break;
            }
        for (i = 0; i < 4; i++)
            if (gradeCrafting[i].tipo == FOLHA) {
                gradeCrafting[i].tipo = VAZIO;
                gradeCrafting[i].quantidade = 0;
                break;
            }
    } else if (areia == 0 && tronco == 1 && folhas == 0 && pedra == 2) {
        for (i = 0; i < 4; i++)
            if (gradeCrafting[i].tipo == TRONCO) {
                gradeCrafting[i].tipo = VAZIO;
                gradeCrafting[i].quantidade = 0;
                break;
            }
        for (i = 0; i < 4; i++)
            if (gradeCrafting[i].tipo == PEDRA && gradeCrafting[i].quantidade > 0) {
                int removido = pedra < gradeCrafting[i].quantidade
                              ? pedra : gradeCrafting[i].quantidade;
                gradeCrafting[i].quantidade -= removido;
                pedra -= removido;
                if (gradeCrafting[i].quantidade == 0)
                    gradeCrafting[i].tipo = VAZIO;
                if (pedra == 0) break;
            }
    }
    atualizaReceita();
}
