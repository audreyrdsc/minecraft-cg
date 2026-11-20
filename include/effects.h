#ifndef MINECRAFT_CG_EFFECTS_H
#define MINECRAFT_CG_EFFECTS_H

void iniciaEfeitos(void);
void atualizaEfeitos(float dt);
void desenhaEfeitos(void);
void alternaChuva(void);
int chuvaEstaAtiva(void);
void defineChuva(int ativa);
void emiteParticulasBloco(float x, float y, float z, int tipo);

#endif
