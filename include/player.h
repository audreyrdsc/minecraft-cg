#ifndef MINECRAFT_CG_PLAYER_H
#define MINECRAFT_CG_PLAYER_H

extern float jogadorX, jogadorY, jogadorZ;
extern float velocidadeVertical;
extern float vidaJogador, fomeJogador, saturacaoJogador;
extern float distanciaQueda;

int blocoSolido(int x, int y, int z);
int posicaoLivre(float x, float y, float z);
void moveJogador(float dx, float dy, float dz);
int jogadorNoChao(void);
int blocoApoioJogador(void);
void reapareceJogador(void);

#endif
