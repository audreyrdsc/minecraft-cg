#ifndef MINECRAFT_CG_AUDIO_H
#define MINECRAFT_CG_AUDIO_H

void inicializaAudio(void);
void audioQuebra(void);
void audioQuebraBloco(int tipo);
void audioColeta(void);
void audioDano(void);
void audioAtaque(void);
void audioZumbiAtingido(void);
void audioZumbiMorreu(void);
void audioComer(void);
void audioAtualizaPassos(float dt, int andando);
void audioAtualizaAgua(int dentroDaAgua);
void audioAtualizaNoite(int noite);
void audioSpawnZumbi(void);

#endif
