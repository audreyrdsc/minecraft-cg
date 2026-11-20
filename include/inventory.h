#ifndef MINECRAFT_CG_INVENTORY_H
#define MINECRAFT_CG_INVENTORY_H

#ifndef MAX_ITEM_STACK
#define MAX_ITEM_STACK 64
#endif
#ifndef INVENTORY_SLOTS
#define INVENTORY_SLOTS 36
#endif
#ifndef NUM_ITENS
#define NUM_ITENS 13
#endif

#include "game_types.h"

extern const int tiposItens[NUM_ITENS];
extern int quantidadesItens[NUM_ITENS];
extern Inventory inventario;

int indiceDoItem(int tipo);
void sincronizaHotbar(void);
int adicionaAoInventario(int tipo, int quantidade);
int removeDoInventario(int tipo, int quantidade);
int gastaDurabilidade(int tipo);
int durabilidadeDoItem(int tipo);

#endif
