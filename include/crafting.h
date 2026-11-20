#ifndef MINECRAFT_CG_CRAFTING_H
#define MINECRAFT_CG_CRAFTING_H

#include "game_types.h"

#ifndef VAZIO
#define VAZIO 0
#endif
#ifndef TRONCO
#define TRONCO 4
#endif
#ifndef FOLHA
#define FOLHA 5
#endif
#ifndef AREIA
#define AREIA 6
#endif
#ifndef PEDRA
#define PEDRA 3
#endif
#ifndef VIDRO
#define VIDRO 17
#endif
#ifndef TOCHA
#define TOCHA 18
#endif
#ifndef PICARETA
#define PICARETA 22
#endif

extern ItemStack gradeCrafting[4];
extern ItemStack resultadoCrafting;

void atualizaReceita(void);
void limpaCrafting(void);
void consomeReceita(void);

#endif
