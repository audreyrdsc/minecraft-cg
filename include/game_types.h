#ifndef MINECRAFT_CG_GAME_TYPES_H
#define MINECRAFT_CG_GAME_TYPES_H

/* Tipos compartilhados entre os futuros módulos do jogo. Este arquivo não
   contém estado global nem lógica: apenas define o contrato dos dados. */

#ifndef INVENTORY_SLOTS
#define INVENTORY_SLOTS 36
#endif

typedef struct {
    int tipo;
    int quantidade;
    int durabilidade;
} ItemStack;

typedef struct {
    ItemStack slots[INVENTORY_SLOTS];
} Inventory;

typedef struct {
    int ativo;
    int tipo;
    float x, y, z;
    float velocidadeY;
} DropItem;

typedef struct {
    int encontrado;
    int x, y, z;
    int adjX, adjY, adjZ;
    int tipo;
} HitBloco;

typedef struct {
    float x, y, z;
    float intensidade;
    float alcance;
} LuzPonto;

#endif
