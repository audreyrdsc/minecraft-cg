#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../include/blocks.h"
#include "../include/inventory.h"

const int tiposItens[NUM_ITENS] = {
    GRAMA, TERRA, PEDRA, TRONCO, FOLHA, AREIA, TOCHA, NEVE,
    VIDRO, MACA, PICARETA, ESPADA, CRISTAL
};
int quantidadesItens[NUM_ITENS];
Inventory inventario;

int main(void)
{
    int i;
    memset(&inventario, 0, sizeof(inventario));
    memset(quantidadesItens, 0, sizeof(quantidadesItens));

    assert(adicionaAoInventario(MACA, 70) == 70);
    assert(inventario.slots[0].quantidade == 64);
    assert(inventario.slots[1].quantidade == 6);

    assert(adicionaAoInventario(PICARETA, 1) == 1);
    assert(inventario.slots[2].quantidade == 1);
    assert(durabilidadeDoItem(PICARETA) == 100);
    for (i = 0; i < 100; i++) assert(gastaDurabilidade(PICARETA));
    assert(durabilidadeDoItem(PICARETA) == 0);
    assert(inventario.slots[2].tipo == VAZIO);

    assert(adicionaAoInventario(CRISTAL, 1) == 1);
    assert(indiceDoItem(CRISTAL) == 12);
    assert(inventario.slots[2].tipo == CRISTAL);
    /* A barra é indexada por tipo, não pelo slot físico do inventário. */
    assert(quantidadesItens[12] == 1);

    puts("inventory_test: OK");
    return 0;
}
