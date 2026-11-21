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
void audioAtualizaPassos(float dt, int andando, int blocoApoio);
void audioAtualizaAgua(int dentroDaAgua);
void audioAtualizaNoite(float tempoMundo, float duracaoDia);
void audioSpawnZumbi(void);
void audioAlertaCachorro(void);
void audioAtualizaLatidos(void);
void audioMontaria(void);
void audioCavaloProximo(void);
void audioPorcoProximo(void);
void audioAtualizaGalope(int andando);
void audioAtualizaFogoZumbi(int queimando);

#endif
