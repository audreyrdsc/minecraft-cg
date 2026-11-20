#include <assert.h>
#include <stdio.h>
#include "../include/crafting.h"

ItemStack gradeCrafting[4];
ItemStack resultadoCrafting;

int main(void)
{
    gradeCrafting[0] = (ItemStack){ PEDRA, 1, 0 };
    gradeCrafting[1] = (ItemStack){ PEDRA, 1, 0 };
    gradeCrafting[2] = (ItemStack){ TRONCO, 1, 0 };
    gradeCrafting[3] = (ItemStack){ VAZIO, 0, 0 };

    atualizaReceita();
    assert(resultadoCrafting.tipo == PICARETA);
    assert(resultadoCrafting.quantidade == 1);

    consomeReceita();
    assert(gradeCrafting[0].quantidade == 0);
    assert(gradeCrafting[1].quantidade == 0);
    assert(gradeCrafting[2].quantidade == 0);
    assert(resultadoCrafting.tipo == VAZIO);

    puts("crafting_test: OK");
    return 0;
}
