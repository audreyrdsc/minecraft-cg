/*
 * Projeto Minecraft
 *
 * Reúne o que foi visto nas semanas 1 a 5 e na aula de texturas e
 * transparência:
 *   Semana 1: janela, laço principal, tempo de quadro, FPS no título, ESC
 *   Semana 2: primitivas (GL_QUADS) e cor por face
 *   Semana 3: cubo 3D, buffer de profundidade, descarte de faces,
 *             transformações e instâncias com glPushMatrix/glPopMatrix
 *   Semana 4: perspectiva, correção de proporção, câmera com gluLookAt
 *             e navegação pelo teclado
 *   Semana 5: teclado lido por estado, movimento escalado pelo tempo de
 *             quadro, mouse com yaw e pitch e captura do cursor, colisão
 *             por caixa alinhada aos eixos, câmera em primeira e
 *             terceira pessoa e aproximação pela roda do mouse
 *   Texturas e transparência: atlas de texturas lido de um PPM,
 *             coordenadas de textura por face, filtro GL_NEAREST,
 *             GL_MODULATE com o brilho por face, canal alfa
 *             com cor-chave, recorte (teste de alfa) nas folhas,
 *             mistura no vidro e na água, desenho em três passadas e
 *             remoção das faces que um vizinho opaco esconde
 *
 * Cenário: terreno 64 x 64 com uma montanha, uma praia, o mar e uma
 * extensão procedural,
 * uma árvore, uma casinha de vidro e duas nuvens. Tudo é feito com um
 * único cubo de lado 1, desenhado várias vezes em posições diferentes.
 *
 * Texturas: todas as faces vêm de uma única imagem, atlas.ppm. O jogo
 * lê o arquivo da pasta atual, então precisa ser rodado da pasta do
 * projeto.
 *
 * Eixos do mundo:
 *   x  esquerda (0) para a direita (63)
 *   y  altura: 0 é o fundo, cresce para cima
 *   z  fundo do mapa (0) para a frente (63), onde fica o mar
 *
 * Controles:
 *   W / S        andar para frente / para trás
 *   A / D        andar para a esquerda / para a direita
 *   R            correr (segurando, junto com W, A, S, D)
 *   Setas < >    girar o olhar
 *   Espaço ou ^  saltar
 *   F             comer a maçã selecionada
 *   Mouse        direção do olhar (já começa capturado)
 *   Roda ou + -  aproximar / afastar (campo de visão)
 *   Tab          liberar / capturar o cursor; clique também captura
 *   F5 ou V      alternar terceira (início) / primeira pessoa
 *   P            alternar perspectiva / ortográfica
 *   ESC          abrir / fechar o menu (pausa, instruções)
 *   Q            sair (com o menu aberto)
 *
 * Organização do arquivo, de cima para baixo:
 *   constantes, declarações antecipadas, texturas e transparência
 *   (tudo o que essa aula trouxe, num bloco só), mundo, colisão,
 *   câmera, entrada, menu e, no fim, desenho do quadro e main()
 *
 * Compilar e rodar (Linux):
 *   gcc mine.c -o mine -lGLEW -lGL -lGLU -lglut -lm && ./mine
 *
 * Compilar e rodar (macOS, com a GLUT da Apple; sem roda do mouse):
 *   clang mine.c -o mine -framework OpenGL -framework GLUT \
 *         -framework ApplicationServices -lm && ./mine
 */

#ifdef _WIN32
#include <windows.h>
#include "resource.h"
#endif

#ifdef __APPLE__
/* macOS: usa a GLUT e o OpenGL nativos da Apple, sem GLEW; essa GLUT
   não tem a roda do mouse nem o Shift como tecla especial */
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#include <ApplicationServices/ApplicationServices.h>   /* mouse no macOS,
                                                          ver movimentoMouse */
#else
#include <GL/glew.h>            /* carrega as funções do OpenGL */
#include <GL/freeglut.h>        /* janela, teclado, mouse e laço principal;
                                   a versão freeglut traz a roda do mouse */
#include <GL/glu.h>             /* gluPerspective e gluLookAt */
#endif

#include <stdio.h>              /* printf, sprintf */
#include <stdlib.h>             /* exit */
#include <math.h>               /* sinf, cosf, floorf, sqrtf, ceilf */
#include <float.h>              /* FLT_MAX para eixos paralelos no DDA */
#include <string.h>             /* strcmp para opções de inicialização */
#include <ctype.h>              /* tolower */

#define PI 3.14159265f

/* Modos didáticos opcionais para a demonstração da Semana 10.
   Sem nenhuma flag, o projeto mantém o comportamento original. */
#ifndef DEMO_UI_MATRIX_ONLY   //Simula uma interface de usuário mínima, só com a matriz de blocos
#define DEMO_UI_MATRIX_ONLY 0 //Guarda apenas a matriz de projeção
#endif

#ifndef DEMO_UI_FIXED_BAR   //Simula uma interface de usuário mínima, só com a barra fixa de itens
#define DEMO_UI_FIXED_BAR 0 //A barra sai do centro ao redimensionar a janela
#endif

#ifndef DEMO_UI_RAW_MOUSE_Y   //Simula a necessidade de considerar o espelhamento do mouse
#define DEMO_UI_RAW_MOUSE_Y 0 //A GLUT fornece a posição do mouse com origem no canto superior esquerdo
#endif                        //A interface OpenGL foi configurada com origem no canto inferior esquerdo

#ifndef DEMO_UI_PAUSE_CLOCK   //Simula o arremesso do personagem na pausa segurando botão de movimento
#define DEMO_UI_PAUSE_CLOCK 0 //O relógio do jogo continua contando mesmo pausado, precisa reiniciar
#endif

#define DEMO_UI_FIXED_BAR_X 215.0f

/* ------------------------------------------------------------------ */
/* Tipos de bloco                                                     */
/* Cada tipo é só um número com nome. O número não muda a forma do    */
/* bloco (todos são o mesmo cubo); ele só escolhe a aparência: a      */
/* célula do atlas de cada face, em celulaDaFace(), ou a cor sólida,  */
/* em corDaFace(). Para criar um tipo novo: um #define aqui e um case */
/* em uma das duas.                                                   */
/* VAZIO marca uma posição sem bloco na matriz do mundo. Os tipos de  */
/* PELE a OLHO não aparecem no mundo: são as cores do personagem.     */
/* ------------------------------------------------------------------ */

/* Em que passada de desenho cada tipo entra (ver desenhaOpacos):
   opacos primeiro, depois o recorte (folhas, com vazados) e por último
   os semitransparentes (vidro e água) */
/* ------------------------------------------------------------------ */
/* Tamanho do mundo                                                   */
/* ------------------------------------------------------------------ */

#include "include/config.h"
#define NIVEL_AGUA  1   /* colunas com altura até este valor são praia ou
                           mar; acima do chão delas, até esta altura, vai
                           água */
#define NIVEL_NEVE  6   /* colunas com altura a partir deste valor têm
                           neve no topo em vez de grama */
#define ALTURA_NUVEM 14 /* altura (y) das nuvens; bem acima da neve, para
                           não se confundir com ela */

/* ------------------------------------------------------------------ */
/* Atlas de texturas                                                  */
/* Uma única imagem com a aparência de todas as faces, dividida em    */
/* CELULAS_POR_LADO x CELULAS_POR_LADO células de mesmo tamanho. Cada */
/* CEL_ é o índice de uma célula, contado da esquerda para a direita  */
/* e de cima para baixo.                                              */
/* ------------------------------------------------------------------ */

#define ARQUIVO_ATLAS    "atlas.ppm"
#define CELULAS_POR_LADO 4      /* N: o atlas tem N x N células */
#define TAMANHO_CELULA   16     /* pixels de lado de cada célula */

#define CEL_GRAMA_TOPO  0
#define CEL_GRAMA_LADO  1       /* terra com faixa de grama no alto */
#define CEL_TERRA       2
#define CEL_PEDRA       3
#define CEL_ANEIS       4       /* topo e base do tronco */
#define CEL_CASCA       5       /* laterais do tronco */
#define CEL_FOLHA       6       /* com vazados em magenta (cor-chave) */
#define CEL_VIDRO       7
#define CEL_AGUA        8
#define CEL_AREIA       9
#define CEL_NEVE        10
#define SEM_TEXTURA     (-1)    /* nuvem e personagem: cor sólida */
/* As 6 faces de um bloco, uma por bit, para dizer quais desenhar:
   FACE_TOPO | FACE_BASE são só o topo e a base */
#define FACE_FRENTE   1         /* z = 1 */
#define FACE_TRAS     2         /* z = 0 */
#define FACE_DIREITA  4         /* x = 1 */
#define FACE_ESQUERDA 8         /* x = 0 */
#define FACE_TOPO     16        /* y = 1 */
#define FACE_BASE     32        /* y = 0 */
#define TODAS_FACES   63

/* opacidade (alfa, de 0 a 255) dada às células semitransparentes no
   carregamento: o PPM não tem canal alfa */
#define ALFA_VIDRO 102          /* 0,4 */
#define ALFA_AGUA  153          /* 0,6 */

/* ------------------------------------------------------------------ */
/* Personagem                                                         */
/* Para a colisão, o personagem é uma caixa de LARGURA_JOGADOR x      */
/* ALTURA_JOGADOR x LARGURA_JOGADOR, centrada em x e z e apoiada nos  */
/* pés. Os olhos ficam ALTURA_OLHOS acima dos pés.                    */
/* ------------------------------------------------------------------ */

#define LARGURA_JOGADOR 0.6f    /* lado da caixa em x e z */
#define ALTURA_JOGADOR  1.8f    /* dos pés ao topo da cabeça */
#define ALTURA_OLHOS    1.6f    /* dos pés aos olhos */

#define VELOCIDADE       5.0f   /* blocos por segundo */
#define VELOCIDADE_GIRO  90.0f  /* graus por segundo (setas) */
#define DT_MAXIMO        0.1f   /* maior tempo de quadro aceito, em s */
#define FATOR_CORRIDA    2.0f   /* segurando R, a velocidade dobra */
#define PASSO_MAXIMO     0.5f   /* maior deslocamento testado de uma vez */
#define GRAVIDADE        18.0f  /* aceleração vertical, em blocos/s² */
#define VELOCIDADE_SALTO 7.0f   /* impulso vertical inicial */
#define VELOCIDADE_QUEDA 30.0f  /* limite da velocidade de queda */
#define VIDA_MAXIMA      20.0f  /* dez corações, como referência visual */
#define FOME_MAXIMA      20.0f  /* dez ícones de alimento */
#define FOME_DRENO       0.08f  /* pontos por segundo em repouso */
#define CURA_MACA        6.0f   /* pontos restaurados por maçã */
#define SATURACAO_MAXIMA 20.0f
#define SATURACAO_MACA   4.0f
#define FOME_MINIMA_CORRIDA 3.0f
#define ALTURA_QUEDA_SEGURA 3.0f
#define DURACAO_DIA       240.0f /* segundos de um ciclo completo */
#define DISTANCIA_DESENHO 28.0f  /* culling simples para mundos maiores */
#define DISTANCIA_CARGA_CHUNK 24.0f

#define SENSIBILIDADE 0.1f      /* graus de giro por pixel de mouse */
#define LIMITE_PITCH  89.0f     /* inclinação máxima, para cima e para baixo */

/* ------------------------------------------------------------------ */
/* Câmera                                                             */
/* ------------------------------------------------------------------ */

#define PRIMEIRA_PESSOA 0
#define TERCEIRA_PESSOA 1

#define DISTANCIA_CAMERA 4.0f   /* recuo da câmera em terceira pessoa */
#define ELEVACAO_CAMERA  1.5f   /* altura da câmera acima da cabeça */

/* ------------------------------------------------------------------ */
/* Boneco (desenho do personagem em terceira pessoa)                  */
/* Medidas em blocos, no estilo do Minecraft: pernas, tronco e cabeça */
/* empilhados até ALTURA_JOGADOR, braços dos lados do tronco. Tudo    */
/* cabe num círculo de raio 0.3 em volta do eixo vertical, para o     */
/* boneco girado não sair da caixa de colisão (ver desenhaJogador).   */
/* ------------------------------------------------------------------ */

#define COMPRIMENTO_MEMBRO 0.7f /* braços e pernas */
#define PROFUNDIDADE_CORPO 0.2f /* espessura (z) de tronco e membros */
#define LARGURA_PERNA      0.15f
#define LARGURA_TRONCO     0.3f
#define LARGURA_BRACO      0.1f
#define LADO_CABECA        0.38f
#define BALANCO_MAXIMO     30.0f    /* graus de balanço de braços e pernas */
#define FASE_POR_BLOCO     3.9f     /* radianos de balanço por bloco andado */
#define VELOCIDADE_AMPLITUDE 6.0f   /* quão rápido o balanço começa e para */

#define FOV_MIN 20.0f
#define FOV_MAX 90.0f
#define MAX_LUZES 32


/* ================================================================== */
/* Declarações antecipadas                                            */
/* Funções e variáveis usadas antes do ponto do arquivo em que        */
/* são definidas. Servem também de índice do que cada parte faz.      */
/* ================================================================== */

/*
 * Texturas e transparência: o atlas, a célula de cada face, as faces
 * ocultas, a casinha de vidro e o desenho em três passadas. Tudo
 * definido na seção TEXTURAS E TRANSPARÊNCIA.
 */
/* 1. atlas: leitura do arquivo e envio à placa de vídeo */
extern GLuint atlas;                /* objeto de textura do atlas */
unsigned char *lerPPM(const char *nome, int *l, int *a);
unsigned char *converteRGBA(const unsigned char *rgb, int l, int a);
void carregaAtlas(void);
void carregaTexturasUI(void);

/* 2. célula do atlas de cada face */
int  celulaDaFace(int tipo, int face);
void coordenadasDaCelula(int celula, float *s0, float *t0, float *lado);

/* 3. passada de cada tipo e faces que um vizinho esconde */
int  passadaDoBloco(int tipo);
int  blocoOpaco(int tipo);
int  faceVisivel(int tipo, int vizinho);
int  facesVisiveis(int x, int y, int z);    /* soma de bits FACE_ */

/* 4. casinha de vidro */
void casinhaEm(int x, int z);

/* 5. desenho da cena, em três passadas (ver desenhaOpacos) */
typedef struct {
    int x, y, z;                    /* posição do bloco na matriz */
    float distancia;                /* ao olho, ao quadrado */
} BlocoTransparente;
#define MAX_TRANSPARENTES (LADO * ALTURA_MUNDO * LADO)
#ifndef CHUNK_LADO
#define CHUNK_LADO 16
#define NUM_CHUNKS_X ((LADO + CHUNK_LADO - 1) / CHUNK_LADO)
#define NUM_CHUNKS_Z ((LADO + CHUNK_LADO - 1) / CHUNK_LADO)
#endif

static GLuint listasChunks[NUM_CHUNKS_X][NUM_CHUNKS_Z][2];
static int listasChunksInicializadas = 0;
static int epocaLuzDasListas = -1;

void desenhaOpacos(void);           /* passadas 1 (opacos) e 2 (recorte) */
int  listaTransparentes(float olhoX, float olhoY, float olhoZ,
                        BlocoTransparente *lista);
void desenhaTransparentes(void);    /* passada 3 (vidro e água) */

/* 6. posição do olho da câmera */
void posicaoDoOlho(float *x, float *y, float *z);

/*
 * O cenário: o mapa de alturas, a matriz 3D com o tipo de cada bloco,
 * a construção do mundo a partir do mapa e o desenho dos blocos.
 */
/* tipo de cada bloco, indexado como mundo[x][y][z] (ver MUNDO) */
extern unsigned char mundo[LADO][ALTURA_MUNDO][LADO];

/* Barra de itens da Semana 10. Os nove tipos representam os blocos
   utilizáveis; as quantidades formam o estoque inicial do jogador e são
   alteradas por colocação e coleta de drops. */
#define NUM_ITENS 13
extern const int tiposItens[NUM_ITENS];
extern int quantidadesItens[NUM_ITENS];
extern int itemSelecionado;
extern int slotSelecionadoInventario;

#define MAX_ITEM_STACK 64
#define MAX_DROPS 128
#define INVENTORY_SLOTS 36
#include "include/game_types.h"
#include "include/inventory.h"
#include "include/crafting.h"
#include "include/entities.h"
#include "include/world.h"
#include "include/worldgen.h"
#include "include/save.h"
#include "include/effects.h"
#include "include/audio.h"
#include "include/blocks.h"
#include "include/chunks.h"
#include "include/player.h"

extern Inventory inventario;

static int consomeAlimento(void);

static LuzPonto luzes[MAX_LUZES];
static int numeroLuzes = 0;
static float pontoDesenhoX = 0.0f;
static float pontoDesenhoY = 0.0f;
static float pontoDesenhoZ = 0.0f;

/* desenho de um bloco */
void corDaFace(int tipo, float brilho, float tom);
void desenhaBloco(int tipo, float tom, int faces);  /* cubo de lado 1 na origem */
void blocoEm(int tipo, int x, int y, int z);

/* acesso à matriz */
int  alturaDoTerreno(int x, int z);
int  dentroDoMundo(int x, int y, int z);
void colocaBloco(int tipo, int x, int y, int z);
int  blocoNoMundo(int x, int y, int z);

/* construção (uma vez, no início) */
void colunaEm(int x, int z);
void arvoreEm(int x, int z);
void nuvemEm(int x, int z, int largura, int fundo);
void construirMundo(void);

/*
 * A posição do personagem e a colisão dele com o mundo.
 */
/* posição do personagem; jogadorY é a altura dos PÉS (ver COLISÃO) */
extern float jogadorX, jogadorY, jogadorZ;
extern float tempoMundo;
extern float fomeJogador;
extern float saturacaoJogador;

int  blocoSolido(int x, int y, int z);
int  posicaoLivre(float x, float y, float z);
void moveJogador(float dx, float dy, float dz);
int  blocoMirado(HitBloco *hit);
void interageComBloco(int botao);
void atualizaDrops(float dt);
void desenhaDrops(void);
void reconstroiLuzes(void);

/*
 * Orientação do olhar, lente (projeção), posição da câmera em primeira
 * e terceira pessoa e desenho do personagem.
 */
extern float yaw, pitch;            /* orientação do olhar, em graus */
extern int   modoCamera;            /* PRIMEIRA_PESSOA ou TERCEIRA_PESSOA */

extern int   larguraJanela, alturaJanela;
extern int   projecaoOrtogonal;     /* 0 perspectiva, 1 ortográfica */
extern float campoDeVisao;          /* graus */

void aplicaProjecao(void);
void redimensiona(int largura, int altura);
void direcaoDoOlhar(float *dx, float *dy, float *dz);
void direcaoDoRaio(float *dx, float *dy, float *dz);
void desenhaJogador(void);
void posicionaCamera(void);

/*
 * Teclado, mouse e roda, e a atualização do movimento a cada quadro.
 * As funções de callback são registradas na GLUT por main().
 */
extern int ultimoTempo;             /* tempo da atualização anterior, em ms */

/* animação de caminhada, atualizada em atualiza() e usada pelo desenho
   do personagem */
extern float faseCaminhada;         /* radianos; avança com a distância */
extern float amplitudeCaminhada;    /* 0 parado, 1 andando */

/* teclado */
void teclado(unsigned char tecla, int x, int y);
void tecladoSolto(unsigned char tecla, int x, int y);
void especial(int tecla, int x, int y);
void especialSolto(int tecla, int x, int y);

/* mouse */
void movimentoMouse(int x, int y);
void capturaCursor(void);
void liberaCursor(void);
void alternaCaptura(void);
void cliqueMouse(int botao, int estado, int x, int y);
void entradaJanela(int estado);
void mudaZoom(int direcao);
void rodaMouse(int roda, int direcao, int x, int y);

/* atualização */
void atualiza(void);

/* Interface e estados do jogo. */
typedef enum {
    ESTADO_MENU_PRINCIPAL,
    ESTADO_JOGANDO,
    ESTADO_PAUSADO,
    ESTADO_INSTRUCOES,
    ESTADO_INVENTARIO,
    ESTADO_CRAFTING
} EstadoJogo;

extern EstadoJogo estadoJogo;
extern int botaoSelecionado;

void iniciaInterface(void);
void terminaInterface(void);
void abreMenu(void);
void fechaMenu(void);
void alternaMenu(void);
void desenhaInterface(void);
void movimentoInterface(int x, int y);
static int numeroBotoesTela(void);
static void acionaBotaoTela(int indice);
static int pontoNoBotao(int x, int y, int indice);
static void desenhaInventario(void);
static int slotInventarioEm(int x, int y);
static void interageSlotInventario(int slot, int botao);
static void desenhaCrafting(void);
static void interageCrafting(int x, int y, int botao);
static void desenhaAstros(void);
static void desenhaCeuShader(void);
static void desenhaSelecaoBloco(void);
static void iniciaAnimacaoBraco(int tipo);


/* ================================================================== */
/* TEXTURAS E TRANSPARÊNCIA                                           */
/*                                                                    */
/* Tudo o que entrou com a aula de texturas e transparência, reunido  */
/* num lugar só, na ordem em que acontece:                            */
/*   1. atlas: leitura do PPM, canal alfa, envio à placa e filtro     */
/*   2. célula do atlas de cada face e suas coordenadas (s, t)        */
/*   3. passada de cada tipo de bloco e faces que um vizinho esconde  */
/*   4. casinha de vidro                                              */
/*   5. desenho da cena em três passadas                              */
/*   6. posição do olho, para ordenar os semitransparentes            */
/* desenhaBloco(), na seção MUNDO, usa as funções daqui para pôr a    */
/* textura em cada face.                                              */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/* Leitura do atlas: PPM binário                                      */
/* Nenhuma das bibliotecas da disciplina lê arquivos de imagem. O PPM */
/* resolve com poucas linhas: um cabeçalho em texto seguido dos bytes */
/* de cor, R, G, B de cada pixel, linha por linha, de cima para baixo:*/
/*                                                                    */
/*   P6                                                               */
/*   64 64          largura e altura                                  */
/*   255            valor máximo de cada componente                   */
/*   RGB RGB RGB ...                                                  */
/*                                                                    */
/* O leitor assume um cabeçalho sem linhas de comentário (#), como o  */
/* do atlas.ppm. Devolve NULL se o arquivo não abrir ou               */
/* não for um P6 completo, com no máximo 4096 pixels de lado.        */
/* ------------------------------------------------------------------ */

unsigned char *lerPPM(const char *nome, int *l, int *a)
{
    FILE *f = fopen(nome, "rb");
    int max;

    if (f == NULL) return NULL;
    /* o teto nas medidas impede que a conta do malloc estoure com um
       cabeçalho absurdo; o atlas tem 64 x 64 */
    if (fscanf(f, "P6 %d %d %d", l, a, &max) != 3 || max != 255 ||
        *l <= 0 || *a <= 0 || *l > 4096 || *a > 4096) {
        fclose(f);
        return NULL;
    }
    fgetc(f);                               /* o espaço após o cabeçalho */

    unsigned char *rgb = malloc(3 * *l * *a);
    if (rgb != NULL && fread(rgb, 3, *l * *a, f) != (size_t)(*l * *a)) {
        free(rgb);                          /* arquivo cortado */
        rgb = NULL;
    }
    fclose(f);
    return rgb;
}

/* ------------------------------------------------------------------ */
/* RGB para RGBA: o canal alfa (a quarta componente da cor, presente  */
/* desde a semana 1) diz a opacidade de cada texel. O PPM não tem     */
/* alfa, então ele é decidido aqui:                                   */
/*   magenta (255, 0, 255)  alfa 0: a cor-chave, os vazados da folha  */
/*   células de vidro/água  alfa parcial (ALFA_VIDRO, ALFA_AGUA)      */
/*   o resto                alfa 255, opaco                           */
/* O texel transparente também recebe o verde da folha no lugar do    */
/* magenta: com GL_NEAREST não faz diferença, mas um filtro que faça  */
/* média com os vizinhos deixaria uma franja rosa nas bordas.         */
/* ------------------------------------------------------------------ */

unsigned char *converteRGBA(const unsigned char *rgb, int l, int a)
{
    unsigned char *rgba = malloc(4 * l * a);
    int i;

    for (i = 0; i < l * a; i++) {
        unsigned char r = rgb[3*i], g = rgb[3*i+1], b = rgb[3*i+2];
        int x = i % l, y = i / l;           /* coluna e linha do texel */
        int celula = (y / TAMANHO_CELULA) * CELULAS_POR_LADO + x / TAMANHO_CELULA;

        rgba[4*i]   = r;
        rgba[4*i+1] = g;
        rgba[4*i+2] = b;
        rgba[4*i+3] = 255;

        if (r == 255 && g == 0 && b == 255) {
            rgba[4*i] = 59; rgba[4*i+1] = 122; rgba[4*i+2] = 33;   /* verde da folha */
            rgba[4*i+3] = 0;
        }
        else if (celula == CEL_VIDRO) rgba[4*i+3] = ALFA_VIDRO;
        else if (celula == CEL_AGUA)  rgba[4*i+3] = ALFA_AGUA;

    }
    return rgba;
}

/* ------------------------------------------------------------------ */
/* Da imagem lida à textura ativa, uma vez só, na inicialização:      */
/*   glGenTextures   reserva um identificador de textura              */
/*   glBindTexture   torna essa textura parte do estado corrente,     */
/*                   como a cor corrente (semana 1)                   */
/*   glTexImage2D    copia os pixels RGBA para a placa de vídeo       */
/*   GL_NEAREST      o texel mais próximo: o visual pixelado; e sem   */
/*                   misturar células vizinhas do atlas nas arestas   */
/*   GL_MODULATE     cor final = textura x cor do vértice, para o     */
/*                   brilho fixo por face continuar valendo           */
/* Como só existe essa textura, ela fica ativa o tempo todo: nenhum   */
/* bloco exige troca de textura entre um desenho e outro.             */
/* ------------------------------------------------------------------ */

GLuint atlas;

typedef struct {
    GLuint id;
    int largura;
    int altura;
} TexturaUI;

TexturaUI texturaLogo;
TexturaUI texturaSubtitulo;
TexturaUI texturaVersao;
TexturaUI texturaEspada;
TexturaUI texturaPicareta;
TexturaUI texturaFomeCheia;
TexturaUI texturaFomeMeia;
TexturaUI texturaFomeVazia;
TexturaUI texturaCoracaoCheio;
TexturaUI texturaCoracaoMeio;
TexturaUI texturaCoracaoVazio;

static GLuint shaderAgua = 0;
static GLint shaderAguaTempo = -1;
static GLint shaderAguaBloco = -1;

static GLuint shaderCeu = 0;
static GLint shaderCeuTempo = -1;
static GLint shaderCeuSeed = -1;
static GLint shaderCeuAnimacao = -1;
static float tempoAnimacaoCeu = 0.0f;
static GLUquadric *ceuDomo = NULL;

static GLuint compilaShaderAgua(GLenum tipo, const char *fonte)
{
    GLuint shader = glCreateShader(tipo);
    GLint compilou = GL_FALSE;

    if (!shader) return 0;
    glShaderSource(shader, 1, &fonte, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compilou);
    if (!compilou) {
        char log[1024];
        GLsizei tamanho = 0;
        glGetShaderInfoLog(shader, sizeof(log), &tamanho, log);
        log[(size_t)(tamanho < (GLsizei)sizeof(log) ? tamanho : (GLsizei)sizeof(log) - 1)] = '\0';
        printf("Shader da agua: %s\n", log);
        fflush(stdout);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static void inicializaShaderAgua(void)
{
    static const char *vertexShader =
        "#version 120\n"
        "uniform float u_tempo;\n"
        "uniform vec3 u_bloco;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_cor;\n"
        "varying vec3 v_mundo;\n"
        "varying float v_topo;\n"
        "void main() {\n"
        "  vec4 p = gl_Vertex;\n"
        "  float fase = (u_bloco.x + p.x) * 1.7 + (u_bloco.z + p.z) * 1.35 + u_tempo * 0.55;\n"
        "  float topo = step(0.99, p.y);\n"
        "  p.y += topo * (sin(fase) * 0.014 + cos(fase * 0.63) * 0.006);\n"
        "  gl_Position = gl_ModelViewProjectionMatrix * p;\n"
        "  v_uv = gl_MultiTexCoord0.st;\n"
        "  v_cor = gl_Color;\n"
        "  v_mundo = u_bloco + p.xyz;\n"
        "  v_topo = topo;\n"
        "}\n";
    static const char *fragmentShader =
        "#version 120\n"
        "uniform sampler2D u_atlas;\n"
        "uniform float u_tempo;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_cor;\n"
        "varying vec3 v_mundo;\n"
        "varying float v_topo;\n"
        "void main() {\n"
        "  vec2 celula = floor(v_uv * 4.0) / 4.0;\n"
        "  float onda = sin(v_mundo.x * 8.0 + v_mundo.z * 5.0 + u_tempo * 0.8);\n"
        "  float onda2 = cos(v_mundo.x * 3.0 - v_mundo.z * 9.0 + u_tempo * 0.55);\n"
        "  vec2 local = fract(v_uv * 4.0 + vec2(onda, onda2) * 0.035 + vec2(u_tempo * 0.004, u_tempo * 0.002));\n"
        "  vec4 texel = texture2D(u_atlas, celula + local / 4.0);\n"
        "  float textura = dot(texel.rgb, vec3(0.30, 0.59, 0.11));\n"
        "  float movimento = 0.5 + 0.5 * onda;\n"
        "  vec3 profundo = vec3(0.015, 0.12, 0.28);\n"
        "  vec3 raso = vec3(0.035, 0.32, 0.48);\n"
        "  vec3 agua = mix(profundo, raso, 0.34 + movimento * 0.22);\n"
        "  agua += (texel.rgb - vec3(0.50)) * 0.08 * textura;\n"
        "  float reflexo = pow(max(0.0, sin(v_mundo.x * 10.0 + v_mundo.z * 4.0 + u_tempo * 0.7)), 14.0);\n"
        "  float faixa = pow(max(0.0, sin(v_mundo.x * 3.2 - v_mundo.z * 1.1 + u_tempo * 0.35)), 18.0);\n"
        "  float brilho = pow(max(0.0, sin(v_mundo.x * 15.0 + v_mundo.z * 2.0 + u_tempo * 0.9)), 28.0);\n"
        "  agua += vec3(0.30, 0.66, 0.78) * reflexo * v_topo * 0.32;\n"
        "  agua += vec3(0.42, 0.82, 0.86) * faixa * v_topo * 0.28;\n"
        "  agua += vec3(0.70, 0.92, 0.95) * brilho * v_topo * 0.12;\n"
        "  float alpha = 0.42 + 0.10 * movimento * v_topo;\n"
        "  gl_FragColor = vec4(agua * v_cor.rgb, texel.a * alpha);\n"
        "}\n";
    GLuint vs, fs;
    GLint ligado = GL_FALSE;

    if (shaderAgua || !glCreateShader) return;
    vs = compilaShaderAgua(GL_VERTEX_SHADER, vertexShader);
    fs = compilaShaderAgua(GL_FRAGMENT_SHADER, fragmentShader);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }

    shaderAgua = glCreateProgram();
    glAttachShader(shaderAgua, vs);
    glAttachShader(shaderAgua, fs);
    glLinkProgram(shaderAgua);
    glGetProgramiv(shaderAgua, GL_LINK_STATUS, &ligado);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ligado) {
        char log[1024];
        GLsizei tamanho = 0;
        glGetProgramInfoLog(shaderAgua, sizeof(log), &tamanho, log);
        log[(size_t)(tamanho < (GLsizei)sizeof(log) ? tamanho : (GLsizei)sizeof(log) - 1)] = '\0';
        printf("Programa da agua: %s\n", log);
        fflush(stdout);
        glDeleteProgram(shaderAgua);
        shaderAgua = 0;
        return;
    }
    shaderAguaTempo = glGetUniformLocation(shaderAgua, "u_tempo");
    shaderAguaBloco = glGetUniformLocation(shaderAgua, "u_bloco");
    glUseProgram(shaderAgua);
    glUniform1i(glGetUniformLocation(shaderAgua, "u_atlas"), 0);
    glUseProgram(0);
    printf("Shader da agua ativo.\n");
    fflush(stdout);
}

static void inicializaShaderCeu(void)
{
    static const char *vertexShader =
        "#version 120\n"
        "varying vec3 v_direcao;\n"
        "void main() {\n"
        "  v_direcao = gl_Vertex.xyz;\n"
        "  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n";
    static const char *fragmentShader =
        "#version 120\n"
        "uniform float u_tempo;\n"
        "uniform float u_seed;\n"
        "uniform float u_animacao;\n"
        "varying vec3 v_direcao;\n"
        "const float PI = 3.14159265;\n"
        "float hash(vec2 p) {\n"
        "  return fract(sin(dot(p + vec2(u_seed * 0.013, u_seed * 0.021), vec2(127.1, 311.7))) * 43758.5453);\n"
        "}\n"
        "float ruido(vec2 p) {\n"
        "  vec2 i = floor(p);\n"
        "  vec2 f = smoothstep(0.0, 1.0, fract(p));\n"
        "  float a = hash(i);\n"
        "  float b = hash(i + vec2(1.0, 0.0));\n"
        "  float c = hash(i + vec2(0.0, 1.0));\n"
        "  float d = hash(i + vec2(1.0, 1.0));\n"
        "  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);\n"
        "}\n"
        "float fbm(vec2 p) {\n"
        "  float valor = 0.0;\n"
        "  float peso = 0.5;\n"
        "  for (int i = 0; i < 4; i++) {\n"
        "    valor += ruido(p) * peso;\n"
        "    p = p * 2.03 + vec2(13.7, 7.1);\n"
        "    peso *= 0.5;\n"
        "  }\n"
        "  return valor;\n"
        "}\n"
        "void main() {\n"
        "  vec3 dir = normalize(v_direcao);\n"
        "  float solAltura = sin((u_tempo - 0.25) * 2.0 * PI);\n"
        "  float dia = smoothstep(-0.14, 0.16, solAltura);\n"
        "  float noite = 1.0 - dia;\n"
        "  float horizonte = smoothstep(-0.12, 0.78, dir.y);\n"
        "  vec3 diaHorizonte = vec3(0.58, 0.82, 0.96);\n"
        "  vec3 diaAlto = vec3(0.10, 0.40, 0.72);\n"
        "  vec3 noiteHorizonte = vec3(0.08, 0.12, 0.28);\n"
        "  vec3 noiteAlto = vec3(0.005, 0.012, 0.045);\n"
        "  vec3 corDia = mix(diaHorizonte, diaAlto, horizonte);\n"
        "  vec3 corNoite = mix(noiteHorizonte, noiteAlto, horizonte);\n"
        "  vec3 cor = mix(corNoite, corDia, dia);\n"
        "  float angulo = (u_tempo - 0.25) * 2.0 * PI;\n"
        "  vec3 solDir = normalize(vec3(cos(angulo), 0.78 * sin(angulo), 0.18));\n"
        "  vec3 luaDir = -solDir;\n"
        "  float distSol = 1.0 - dot(dir, solDir);\n"
        "  float distLua = 1.0 - dot(dir, luaDir);\n"
        "  float haloSol = 1.0 - smoothstep(0.0004, 0.055, distSol);\n"
        "  float discoSol = 1.0 - smoothstep(0.00012, 0.00065, distSol);\n"
        "  float haloLua = 1.0 - smoothstep(0.0003, 0.040, distLua);\n"
        "  float discoLua = 1.0 - smoothstep(0.00010, 0.00050, distLua);\n"
        "  float solVisivel = dia * smoothstep(-0.18, 0.10, solDir.y);\n"
        "  float luaVisivel = noite * smoothstep(-0.18, 0.10, luaDir.y);\n"
        "  cor += vec3(1.0, 0.48, 0.08) * haloSol * solVisivel * 0.20;\n"
        "  cor += vec3(1.0, 0.85, 0.38) * discoSol * solVisivel;\n"
        "  cor += vec3(0.28, 0.40, 0.75) * haloLua * luaVisivel * 0.18;\n"
        "  cor += vec3(0.78, 0.86, 1.0) * discoLua * luaVisivel * 0.90;\n"
        "  vec2 estrelaUv = vec2(atan(dir.z, dir.x) / (2.0 * PI) + 0.5, dir.y * 0.5 + 0.5) * 64.0;\n"
        "  vec2 celulaEstrela = floor(estrelaUv);\n"
        "  vec2 localEstrela = fract(estrelaUv) - 0.5;\n"
        "  float sorteEstrela = hash(celulaEstrela);\n"
        "  float estrela = step(0.91, sorteEstrela) * (1.0 - smoothstep(0.03, 0.13, length(localEstrela)));\n"
        "  float cintilacao = 0.60 + 0.40 * sin(u_animacao * 2.0 + sorteEstrela * 35.0);\n"
        "  cor += vec3(0.72, 0.86, 1.0) * estrela * cintilacao * noite * 0.95;\n"
        "  float longitude = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;\n"
        "  float latitude = dir.y * 0.5 + 0.5;\n"
        "  float nuvens = 0.0;\n"
        "  for (int i = 0; i < 8; i++) {\n"
        "    float fi = float(i);\n"
        "    float velocidade = 0.0005 + hash(vec2(fi + 11.0, 41.0)) * 0.0006;\n"
        "    float centroX = fract(hash(vec2(fi + 4.0, 18.0)) + u_animacao * velocidade);\n"
        "    float centroY = 0.58 + hash(vec2(fi + 22.0, 7.0)) * 0.25;\n"
        "    float escala = 5.0 + hash(vec2(fi + 31.0, 3.0)) * 4.0;\n"
        "    float dx = fract(longitude - centroX + 0.5) - 0.5;\n"
        "    float dy = latitude - centroY;\n"
        "    vec2 uvNuvem = vec2(dx * escala, dy * escala * 2.0);\n"
        "    float forma = fbm(uvNuvem + vec2(fi * 9.3, fi * 3.7));\n"
        "    float bordaNuvem = 1.0 - smoothstep(0.34, 0.58, length(vec2(dx * 2.3, dy * 3.0)));\n"
        "    nuvens = max(nuvens, smoothstep(0.30, 0.53, forma) * bordaNuvem);\n"
        "  }\n"
        "  float faixaNuvem = smoothstep(0.10, 0.38, dir.y) * (1.0 - smoothstep(0.78, 0.96, dir.y));\n"
        "  float alfaNuvem = nuvens * faixaNuvem * mix(0.14, 0.80, dia);\n"
        "  cor = mix(cor, vec3(0.92, 0.96, 1.0), alfaNuvem);\n"
        "  float tCometa = mod(u_animacao, 30.0);\n"
        "  float ativoCometa = smoothstep(22.0, 22.5, tCometa);\n"
        "  ativoCometa *= 1.0 - smoothstep(26.0, 26.5, tCometa);\n"
        "  float progresso = clamp((tCometa - 22.0) / 4.0, 0.0, 1.0);\n"
        "  vec3 inicioCometa = normalize(vec3(-0.72, 0.68, -0.75));\n"
        "  vec3 fimCometa = normalize(vec3(0.62, 0.28, -0.75));\n"
        "  vec3 posCometa = normalize(mix(inicioCometa, fimCometa, progresso));\n"
        "  vec3 direcaoCometa = normalize(fimCometa - inicioCometa);\n"
        "  vec3 cauda = normalize(posCometa - direcaoCometa * 0.14);\n"
        "  vec3 relativo = dir - cauda;\n"
        "  float aoLongo = clamp(dot(relativo, direcaoCometa), 0.0, 0.14);\n"
        "  float distanciaCauda = length(relativo - direcaoCometa * aoLongo);\n"
        "  float brilhoCauda = (1.0 - smoothstep(0.0, 0.018, distanciaCauda)) * (aoLongo / 0.14);\n"
        "  float brilhoNucleo = 1.0 - smoothstep(0.0, 0.018, length(dir - posCometa));\n"
        "  cor += vec3(1.0, 0.78, 0.42) * (brilhoCauda * 0.55 + brilhoNucleo) * ativoCometa * noite;\n"
        "  float tMeteoro = mod(u_animacao, 19.0);\n"
        "  float ativoMeteoro = smoothstep(8.0, 8.2, tMeteoro) * (1.0 - smoothstep(9.7, 10.0, tMeteoro));\n"
        "  float progressoMeteoro = clamp((tMeteoro - 8.0) / 2.0, 0.0, 1.0);\n"
        "  vec3 meteoro = normalize(mix(vec3(-0.4, 0.85, -0.55), vec3(0.45, 0.35, -0.82), progressoMeteoro));\n"
        "  vec3 rastroDir = normalize(vec3(0.85, -0.5, -0.27));\n"
        "  vec3 rastroInicio = normalize(meteoro - rastroDir * 0.16);\n"
        "  vec3 rastroRel = dir - rastroInicio;\n"
        "  float rastroPos = clamp(dot(rastroRel, rastroDir), 0.0, 0.16);\n"
        "  float rastro = (1.0 - smoothstep(0.003, 0.012, length(rastroRel - rastroDir * rastroPos))) * (rastroPos / 0.16);\n"
        "  float nucleo = 1.0 - smoothstep(0.002, 0.012, length(dir - meteoro));\n"
        "  cor += vec3(0.8, 0.9, 1.0) * (nucleo + rastro * 0.8) * ativoMeteoro * noite;\n"
        "  gl_FragColor = vec4(cor, 1.0);\n"
        "}\n";
    GLuint vs, fs;
    GLint ligado = GL_FALSE;

    if (shaderCeu || !glCreateShader) return;
    vs = compilaShaderAgua(GL_VERTEX_SHADER, vertexShader);
    fs = compilaShaderAgua(GL_FRAGMENT_SHADER, fragmentShader);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }
    shaderCeu = glCreateProgram();
    glAttachShader(shaderCeu, vs);
    glAttachShader(shaderCeu, fs);
    glLinkProgram(shaderCeu);
    glGetProgramiv(shaderCeu, GL_LINK_STATUS, &ligado);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ligado) {
        char log[1024];
        GLsizei tamanho = 0;
        glGetProgramInfoLog(shaderCeu, sizeof(log), &tamanho, log);
        log[(size_t)(tamanho < (GLsizei)sizeof(log) ? tamanho : (GLsizei)sizeof(log) - 1)] = '\0';
        printf("Programa do ceu: %s\n", log);
        fflush(stdout);
        glDeleteProgram(shaderCeu);
        shaderCeu = 0;
        return;
    }
    shaderCeuTempo = glGetUniformLocation(shaderCeu, "u_tempo");
    shaderCeuSeed = glGetUniformLocation(shaderCeu, "u_seed");
    shaderCeuAnimacao = glGetUniformLocation(shaderCeu, "u_animacao");
    ceuDomo = gluNewQuadric();
    if (!ceuDomo) {
        glDeleteProgram(shaderCeu);
        shaderCeu = 0;
        printf("Nao foi possivel criar o domo do ceu.\n");
        return;
    }
    printf("Shader do ceu ativo.\n");
    fflush(stdout);
}

void carregaAtlas(void)
{
    int l, a;
    unsigned char *rgb = lerPPM(ARQUIVO_ATLAS, &l, &a);

    if (rgb == NULL || l != CELULAS_POR_LADO * TAMANHO_CELULA ||
                       a != CELULAS_POR_LADO * TAMANHO_CELULA) {
        printf("Erro: não consegui ler %s (rode o jogo de dentro da pasta do projeto)\n",
               ARQUIVO_ATLAS);
        exit(1);
    }
    unsigned char *rgba = converteRGBA(rgb, l, a);

    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, l, a, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    free(rgb);                              /* a placa de vídeo já tem a cópia */
    free(rgba);
}

/* ------------------------------------------------------------------ */
/* Texturas da interface                                              */
/* As imagens do menu ficam fora do atlas: são faixas largas, não     */
/* células 16 x 16 de blocos. O mesmo leitor PPM é reutilizado, mas   */
/* os pixels transparentes vêm como magenta e viram alfa zero; o      */
/* preto real das bordas permanece opaco.                              */
/* ------------------------------------------------------------------ */

static unsigned char *converteUIRGBA(const unsigned char *rgb, int l, int a)
{
    unsigned char *rgba = malloc(4 * l * a);
    int i;

    if (rgba == NULL) return NULL;
    for (i = 0; i < l * a; i++) {
        unsigned char r = rgb[3*i], g = rgb[3*i+1], b = rgb[3*i+2];
        rgba[4*i] = r;
        rgba[4*i+1] = g;
        rgba[4*i+2] = b;
        rgba[4*i+3] = (r == 255 && g == 0 && b == 255) ? 0 : 255;
    }
    return rgba;
}

static TexturaUI carregaUmaTexturaPAM(const char *nome)
{
    TexturaUI resultado = { 0, 0, 0 };
    FILE *arquivo = fopen(nome, "rb");
    char linha[128];
    int profundidade = 0, maximo = 0;
    unsigned char *rgba;
    size_t esperado, lidos;

    if (!arquivo || !fgets(linha, sizeof(linha), arquivo) ||
        strncmp(linha, "P7", 2) != 0) {
        if (arquivo) fclose(arquivo);
        printf("Erro: não consegui ler %s\n", nome);
        exit(1);
    }
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (strncmp(linha, "WIDTH", 5) == 0)
            sscanf(linha + 5, "%d", &resultado.largura);
        else if (strncmp(linha, "HEIGHT", 6) == 0)
            sscanf(linha + 6, "%d", &resultado.altura);
        else if (strncmp(linha, "DEPTH", 5) == 0)
            sscanf(linha + 5, "%d", &profundidade);
        else if (strncmp(linha, "MAXVAL", 6) == 0)
            sscanf(linha + 6, "%d", &maximo);
        else if (strncmp(linha, "ENDHDR", 6) == 0)
            break;
    }
    if (resultado.largura <= 0 || resultado.altura <= 0 || profundidade != 4 ||
        maximo != 255) {
        fclose(arquivo);
        printf("Erro: formato inválido em %s\n", nome);
        exit(1);
    }
    esperado = (size_t)resultado.largura * (size_t)resultado.altura * 4u;
    rgba = malloc(esperado);
    lidos = rgba ? fread(rgba, 1, esperado, arquivo) : 0;
    fclose(arquivo);
    if (!rgba || lidos != esperado) {
        free(rgba);
        printf("Erro: dados incompletos em %s\n", nome);
        exit(1);
    }

    glGenTextures(1, &resultado.id);
    glBindTexture(GL_TEXTURE_2D, resultado.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resultado.largura,
                 resultado.altura, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(rgba);
    return resultado;
}

static TexturaUI carregaUmaTexturaUI(const char *nome)
{
    TexturaUI resultado = { 0, 0, 0 };
    unsigned char *rgb, *rgba;

    rgb = lerPPM(nome, &resultado.largura, &resultado.altura);
    if (rgb == NULL) {
        printf("Erro: não consegui ler %s\n", nome);
        exit(1);
    }
    rgba = converteUIRGBA(rgb, resultado.largura, resultado.altura);
    free(rgb);
    if (rgba == NULL) {
        printf("Erro: memória insuficiente para %s\n", nome);
        exit(1);
    }

    glGenTextures(1, &resultado.id);
    glBindTexture(GL_TEXTURE_2D, resultado.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 resultado.largura, resultado.altura, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(rgba);
    return resultado;
}

void carregaTexturasUI(void)
{
    texturaLogo = carregaUmaTexturaUI("assets/ui/ui_logo.ppm");
    texturaSubtitulo = carregaUmaTexturaUI("assets/ui/ui_subtitulo.ppm");
    texturaVersao = carregaUmaTexturaUI("assets/ui/ui_versao.ppm");
    texturaEspada = carregaUmaTexturaPAM("assets/ui/espada.pam");
    texturaPicareta = carregaUmaTexturaPAM("assets/ui/picareta.pam");
    texturaFomeCheia = carregaUmaTexturaPAM("assets/ui/fome_cheia.pam");
    texturaFomeMeia = carregaUmaTexturaPAM("assets/ui/fome_meia.pam");
    texturaFomeVazia = carregaUmaTexturaPAM("assets/ui/fome_vazia.pam");
    texturaCoracaoCheio = carregaUmaTexturaPAM("assets/ui/coracao_cheio.pam");
    texturaCoracaoMeio = carregaUmaTexturaPAM("assets/ui/coracao_meio.pam");
    texturaCoracaoVazio = carregaUmaTexturaPAM("assets/ui/coracao_vazio.pam");
}

/* ------------------------------------------------------------------ */
/* Células por face (atlas)                                           */
/* Cada face de bloco tem a aparência de uma célula do atlas. A       */
/* maioria dos blocos usa a mesma célula nas 6 faces; a grama e o     */
/* tronco usam uma no topo, outra nas laterais e outra na base:       */
/*                                                                    */
/*   bloco    topo         laterais        base                       */
/*   GRAMA    grama topo   grama lateral   terra                      */
/*   TRONCO   anéis        casca           anéis                      */
/*                                                                    */
/* face é um dos bits FACE_ (ver desenhaBloco). Nuvem e personagem    */
/* não têm célula: devolvem SEM_TEXTURA e ficam com cor sólida.       */
/* ------------------------------------------------------------------ */

int celulaDaFace(int tipo, int face)
{
    switch (tipo) {
        case GRAMA:
            if (face == FACE_TOPO) return CEL_GRAMA_TOPO;
            if (face == FACE_BASE) return CEL_TERRA;
            return CEL_GRAMA_LADO;
        case TRONCO:
            if (face == FACE_TOPO || face == FACE_BASE) return CEL_ANEIS;
            return CEL_CASCA;
        case TERRA: return CEL_TERRA;
        case PEDRA: return CEL_PEDRA;
        case FOLHA: return CEL_FOLHA;
        case AGUA:  return CEL_AGUA;
        case VIDRO: return CEL_VIDRO;
        case AREIA: return CEL_AREIA;
        case NEVE:  return CEL_NEVE;
        default:    return SEM_TEXTURA;
    }
}

/* ------------------------------------------------------------------ */
/* Do índice da célula às coordenadas de textura (s, t).              */
/* Em um atlas de N x N células, a célula i fica na coluna i % N e na */
/* linha i / N (divisão inteira), começa em (s0, t0) e mede 1/N em    */
/* cada direção:                                                      */
/*                                                                    */
/*   s0 = coluna / N      t0 = linha / N      lado = 1 / N            */
/*                                                                    */
/* Exemplo, folha (i = 6, N = 4): coluna 2, linha 1, s de 0,50 a      */
/* 0,75 e t de 0,25 a 0,50.                                           */
/* ------------------------------------------------------------------ */

void coordenadasDaCelula(int celula, float *s0, float *t0, float *lado)
{
    *lado = 1.0f / CELULAS_POR_LADO;
    *s0 = (celula % CELULAS_POR_LADO) * *lado;
    *t0 = (celula / CELULAS_POR_LADO) * *lado;
}

/* ------------------------------------------------------------------ */
/* Transparência: dois casos distintos                                */
/*   recorte            folhas: cada texel é totalmente opaco ou      */
/*                      totalmente transparente (os vazados)          */
/*   semitransparência  vidro e água: a cor do bloco é misturada à    */
/*                      cor do que está atrás, na proporção do alfa   */
/* O resto é opaco. Cada caso é desenhado numa passada própria.       */
/* ------------------------------------------------------------------ */

int passadaDoBloco(int tipo)
{
    if (tipo == FOLHA) return PASSADA_RECORTE;
    if (tipo == VIDRO || tipo == AGUA) return PASSADA_TRANSLUCIDA;
    return PASSADA_OPACA;
}

/* 1 se o bloco esconde por completo o que está atrás dele */
int blocoOpaco(int tipo)
{
    if (tipo == TOCHA) return 0; /* tocha é um objeto, não um cubo cheio */
    return tipo != VAZIO && passadaDoBloco(tipo) == PASSADA_OPACA;
}

/* ------------------------------------------------------------------ */
/* Faces ocultas: uma face encostada num bloco opaco nunca aparece,   */
/* então nem é desenhada. Através do vidro, da água e das folhas, a   */
/* face do vizinho fica à mostra e precisa ser desenhada:             */
/*                                                                    */
/*   face de...     encostada em...          desenhar?                */
/*   qualquer       bloco opaco              não, continua oculta     */
/*   qualquer       vazio, vidro, água,      sim                      */
/*                  folha                                             */
/*   água           água                     não (exceção)            */
/*   vidro          vidro                    não (exceção)            */
/*                                                                    */
/* As exceções evitam as paredes internas de um mar ou de uma janela  */
/* feitos de vários blocos. Entre folhas a face continua: pelos       */
/* vazados se vê o interior da copa. Fora do mundo conta como vazio   */
/* (blocoNoMundo), para as bordas do terreno aparecerem.              */
/* ------------------------------------------------------------------ */

int faceVisivel(int tipo, int vizinho)
{
    if (blocoOpaco(vizinho)) return 0;
    if (vizinho == tipo && (tipo == AGUA || tipo == VIDRO)) return 0;
    return 1;
}

/* As faces do bloco em (x, y, z) que precisam ser desenhadas, como
   soma de bits FACE_, olhando o vizinho de cada lado */
int facesVisiveis(int x, int y, int z)
{
    int tipo = blocoNoMundo(x, y, z);
    int faces = 0;

    if (cacheFacesValido(x, y, z))
        return obtemFacesCache(x, y, z);

    if (faceVisivel(tipo, blocoNoMundo(x, y, z + 1))) faces |= FACE_FRENTE;
    if (faceVisivel(tipo, blocoNoMundo(x, y, z - 1))) faces |= FACE_TRAS;
    if (faceVisivel(tipo, blocoNoMundo(x + 1, y, z))) faces |= FACE_DIREITA;
    if (faceVisivel(tipo, blocoNoMundo(x - 1, y, z))) faces |= FACE_ESQUERDA;
    if (faceVisivel(tipo, blocoNoMundo(x, y + 1, z))) faces |= FACE_TOPO;
    if (faceVisivel(tipo, blocoNoMundo(x, y - 1, z))) faces |= FACE_BASE;
    gravaFacesCache(x, y, z, faces);
    return faces;
}

/* ------------------------------------------------------------------ */
/* Casinha de vidro, oca, com 4 x 4 blocos de base e o canto em       */
/* (x, z). Serve para mostrar a semitransparência: de fora, vê-se o   */
/* terreno através dela; de dentro, o mundo em volta. Vista de cima,  */
/* em cada andar das paredes (V vidro, . vazio, P porta):             */
/*                                                                    */
/*   z           V  V  V  V                                           */
/*   z+1         V  .  .  V                                           */
/*   z+2         V  .  .  V                                           */
/*   z+3         V  P  V  V      parede virada para +z, para o        */
/*               x        x+3    personagem, que começa mais à frente */
/*                                                                    */
/* As paredes vão de chao+1 a chao+3 e o telhado, inteiro, fica em    */
/* chao+4. A porta tem 1 bloco de largura e 2 de altura, para o       */
/* personagem (1.8 de altura) passar.                                 */
/* ------------------------------------------------------------------ */

void casinhaEm(int x, int z)
{
    int chao = alturaDoTerreno(x, z);
    int y, dx, dz;

    for (y = chao + 1; y <= chao + 3; y++)
        for (dx = 0; dx < 4; dx++)
            for (dz = 0; dz < 4; dz++)
                if (dx == 0 || dx == 3 || dz == 0 || dz == 3)   /* só o perímetro */
                    colocaBloco(VIDRO, x + dx, y, z + dz);

    for (dx = 0; dx < 4; dx++)              /* telhado */
        for (dz = 0; dz < 4; dz++)
            colocaBloco(VIDRO, x + dx, chao + 4, z + dz);

    colocaBloco(VAZIO, x + 1, chao + 1, z + 3);     /* porta */
    colocaBloco(VAZIO, x + 1, chao + 2, z + 3);
}

/* ------------------------------------------------------------------ */
/* A cena inteira, em três passadas. Percorre a matriz do mundo, que  */
/* construirMundo() já preencheu, e desenha cada bloco na passada do  */
/* seu tipo (passadaDoBloco):                                         */
/*                                                                    */
/*   1. opacos            estado normal                               */
/*   2. recorte (folhas)  teste de alfa: o fragmento com alfa até 0,5 */
/*                        é descartado, e o resto é desenhado como    */
/*                        opaco. Sem mistura, a ordem não importa     */
/*   3. semitransparentes vidro e água, com mistura (desenhaTranspa-  */
/*                        rentes), do mais distante ao mais próximo   */
/*                                                                    */
/* A mistura usa a cor que JÁ está no buffer, então tudo o que fica   */
/* atrás do vidro precisa ter sido desenhado antes dele. Por isso os  */
/* opacos vêm primeiro, e o personagem é desenhado entre as passadas  */
/* 2 e 3 (ver desenha). Se o vidro viesse antes, a mistura usaria o   */
/* azul do fundo, e o terreno atrás falharia no teste de profundidade */
/* contra o vidro: desapareceria.                                     */
/* ------------------------------------------------------------------ */

static void constroiListaChunk(int chunkX, int chunkZ)
{
    int passada, x, y, z;

    for (passada = PASSADA_OPACA; passada <= PASSADA_RECORTE; passada++) {
        glNewList(listasChunks[chunkX][chunkZ][passada], GL_COMPILE);
        for (x = chunkX * CHUNK_LADO;
             x < (chunkX + 1) * CHUNK_LADO && x < LADO; x++)
            for (y = 0; y < ALTURA_MUNDO; y++)
                for (z = chunkZ * CHUNK_LADO;
                     z < (chunkZ + 1) * CHUNK_LADO && z < LADO; z++)
                    if (mundo[x][y][z] != VAZIO &&
                        passadaDoBloco(mundo[x][y][z]) == passada)
                        blocoEm(mundo[x][y][z], x, y, z);
        glEndList();
    }
    limpaMarcaChunk(chunkX, chunkZ);
}

static void preparaListasChunks(void)
{
    int x, z;
    int epoca = (int)floorf(tempoMundo * 96.0f);

    if (!listasChunksInicializadas) {
        for (x = 0; x < NUM_CHUNKS_X; x++)
            for (z = 0; z < NUM_CHUNKS_Z; z++)
                for (int passada = PASSADA_OPACA;
                     passada <= PASSADA_RECORTE; passada++)
                    listasChunks[x][z][passada] = glGenLists(1);
        listasChunksInicializadas = 1;
        epocaLuzDasListas = -1;
    }
    if (epoca != epocaLuzDasListas) {
        marcaTodosChunksSujo();
        epocaLuzDasListas = epoca;
    }
}

/* desenha todos os chunks visíveis usando listas reconstruídas sob demanda */
static void desenhaPassada(int passada)
{
    int chunkX, chunkZ;
    float olhoX, olhoY, olhoZ;

    posicaoDoOlho(&olhoX, &olhoY, &olhoZ);
    preparaListasChunks();

    for (chunkX = 0; chunkX < NUM_CHUNKS_X; chunkX++)
        for (chunkZ = 0; chunkZ < NUM_CHUNKS_Z; chunkZ++)
            if (chunkVisivel(chunkX, chunkZ, olhoX, olhoZ,
                             DISTANCIA_DESENHO))
                {
                    if (chunkEstaSujo(chunkX, chunkZ))
                        constroiListaChunk(chunkX, chunkZ);
                    glCallList(listasChunks[chunkX][chunkZ][passada]);
                }
}

/* passadas 1 e 2 */
void desenhaOpacos(void)
{
    glDisable(GL_BLEND);                    /* o menu deixa a mistura ligada */

    desenhaPassada(PASSADA_OPACA);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);          /* só passa o que tem alfa > 0,5 */
    desenhaPassada(PASSADA_RECORTE);
    glDisable(GL_ALPHA_TEST);
}

/* ------------------------------------------------------------------ */
/* Ordem dos semitransparentes. Um vidro desenhado antes de outro que */
/* está atrás dele não se mistura com ele, então os blocos de vidro e */
/* água são ordenados do mais distante ao mais próximo do olho, a     */
/* cada quadro (o olho muda). A distância é a do centro do bloco, ao  */
/* quadrado: para comparar, a raiz não faz diferença.                 */
/* ------------------------------------------------------------------ */

static int maisDistantePrimeiro(const void *a, const void *b)
{
    float da = ((const BlocoTransparente *)a)->distancia;
    float db = ((const BlocoTransparente *)b)->distancia;

    return (da < db) - (da > db);           /* maior distância antes */
}

int listaTransparentes(float olhoX, float olhoY, float olhoZ,
                       BlocoTransparente *lista)
{
    int chunkX, chunkZ, x, y, z, n = 0;

    for (chunkX = 0; chunkX < NUM_CHUNKS_X; chunkX++)
        for (chunkZ = 0; chunkZ < NUM_CHUNKS_Z; chunkZ++)
            if (chunkVisivel(chunkX, chunkZ, olhoX, olhoZ,
                             DISTANCIA_DESENHO))
                for (x = chunkX * CHUNK_LADO;
                     x < (chunkX + 1) * CHUNK_LADO && x < LADO; x++)
                    for (y = 0; y < ALTURA_MUNDO; y++)
                        for (z = chunkZ * CHUNK_LADO;
                             z < (chunkZ + 1) * CHUNK_LADO && z < LADO; z++)
                            if (passadaDoBloco(mundo[x][y][z]) == PASSADA_TRANSLUCIDA) {
                                float dx = x + 0.5f - olhoX;
                                float dy = y + 0.5f - olhoY;
                                float dz = z + 0.5f - olhoZ;

                                if (dx * dx + dy * dy + dz * dz >
                                    DISTANCIA_DESENHO * DISTANCIA_DESENHO)
                                    continue;

                                if (n >= MAX_TRANSPARENTES) continue;
                                lista[n].x = x;
                                lista[n].y = y;
                                lista[n].z = z;
                                lista[n].distancia = dx * dx + dy * dy + dz * dz;
                                n++;
                            }

    qsort(lista, n, sizeof(BlocoTransparente), maisDistantePrimeiro);
    return n;
}

/* ------------------------------------------------------------------ */
/* Passada 3: mistura proporcional ao alfa,                           */
/*                                                                    */
/*   cor = alfa x nova + (1 - alfa) x existente                       */
/*                                                                    */
/* (glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)). A escrita no  */
/* buffer de profundidade fica desligada (glDepthMask), para um bloco */
/* transparente não esconder o que está atrás dele; o TESTE de        */
/* profundidade continua ligado, para o vidro atrás de uma montanha   */
/* continuar escondido. Ao final, a escrita volta.                    */
/* ------------------------------------------------------------------ */

static BlocoTransparente transparentes[MAX_TRANSPARENTES];

void desenhaTransparentes(void)
{
    float olhoX, olhoY, olhoZ;
    int n, i;

    posicaoDoOlho(&olhoX, &olhoY, &olhoZ);
    n = listaTransparentes(olhoX, olhoY, olhoZ, transparentes);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (i = 0; i < n; i++) {
        int tipo = blocoNoMundo(transparentes[i].x,
                                transparentes[i].y,
                                transparentes[i].z);
        if (tipo == AGUA && shaderAgua) {
            glUseProgram(shaderAgua);
            glUniform1f(shaderAguaTempo, tempoMundo * DURACAO_DIA);
            glUniform3f(shaderAguaBloco,
                        (float)transparentes[i].x,
                        (float)transparentes[i].y,
                        (float)transparentes[i].z);
        } else {
            glUseProgram(0);
        }
        blocoEm(tipo, transparentes[i].x, transparentes[i].y,
                transparentes[i].z);
    }

    glUseProgram(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

/* Contorno do alvo do raycast e prévia da célula de colocação. */
static void desenhaContornoCubo(int x, int y, int z, float escala,
                                float r, float g, float b)
{
    float minimoX = x + 0.5f - escala;
    float maximoX = x + 0.5f + escala;
    float minimoY = y + 0.5f - escala;
    float maximoY = y + 0.5f + escala;
    float minimoZ = z + 0.5f - escala;
    float maximoZ = z + 0.5f + escala;

    glColor3f(r, g, b);
    glBegin(GL_LINES);
    glVertex3f(minimoX, minimoY, minimoZ); glVertex3f(maximoX, minimoY, minimoZ);
    glVertex3f(maximoX, minimoY, minimoZ); glVertex3f(maximoX, minimoY, maximoZ);
    glVertex3f(maximoX, minimoY, maximoZ); glVertex3f(minimoX, minimoY, maximoZ);
    glVertex3f(minimoX, minimoY, maximoZ); glVertex3f(minimoX, minimoY, minimoZ);
    glVertex3f(minimoX, maximoY, minimoZ); glVertex3f(maximoX, maximoY, minimoZ);
    glVertex3f(maximoX, maximoY, minimoZ); glVertex3f(maximoX, maximoY, maximoZ);
    glVertex3f(maximoX, maximoY, maximoZ); glVertex3f(minimoX, maximoY, maximoZ);
    glVertex3f(minimoX, maximoY, maximoZ); glVertex3f(minimoX, maximoY, minimoZ);
    glVertex3f(minimoX, minimoY, minimoZ); glVertex3f(minimoX, maximoY, minimoZ);
    glVertex3f(maximoX, minimoY, minimoZ); glVertex3f(maximoX, maximoY, minimoZ);
    glVertex3f(maximoX, minimoY, maximoZ); glVertex3f(maximoX, maximoY, maximoZ);
    glVertex3f(minimoX, minimoY, maximoZ); glVertex3f(minimoX, maximoY, maximoZ);
    glEnd();
}

static void desenhaSelecaoBloco(void)
{
    HitBloco hit;
    int tipo;

    if (!blocoMirado(&hit)) return;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glLineWidth(2.0f);

    /* Amarelo: bloco que será removido com o botão esquerdo. */
    desenhaContornoCubo(hit.x, hit.y, hit.z, 0.507f,
                        1.0f, 0.78f, 0.08f);

    /* Ciano: célula adjacente que receberá o item com o botão direito. */
    tipo = tiposItens[itemSelecionado];
    if (tipo != MACA && tipo != PICARETA && tipo != ESPADA &&
        quantidadesItens[itemSelecionado] > 0 &&
        dentroDoMundo(hit.adjX, hit.adjY, hit.adjZ) &&
        blocoNoMundo(hit.adjX, hit.adjY, hit.adjZ) == VAZIO)
        desenhaContornoCubo(hit.adjX, hit.adjY, hit.adjZ, 0.503f,
                            0.20f, 0.90f, 1.0f);

    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
}

/* ------------------------------------------------------------------ */
/* Onde está o olho da câmera (ver CÂMERA): na altura dos olhos em    */
/* primeira pessoa; atrás e acima do personagem em terceira. Usada    */
/* pelo gluLookAt e para ordenar os semitransparentes do mais         */
/* distante ao mais próximo.                                          */
/* ------------------------------------------------------------------ */

void posicaoDoOlho(float *x, float *y, float *z)
{
    *x = jogadorX;                                      /* olhos do personagem */
    *y = jogadorY + ALTURA_OLHOS;
    *z = jogadorZ;

    if (modoCamera == TERCEIRA_PESSOA) {
        float ry = yaw * PI / 180.0f;
        float frenteX = sinf(ry), frenteZ = -cosf(ry);  /* só o plano do chão */

        *x -= frenteX * DISTANCIA_CAMERA;               /* atrás */
        *y += ELEVACAO_CAMERA;                          /* acima, altura fixa */
        *z -= frenteZ * DISTANCIA_CAMERA;
    }
}


/* ================================================================== */
/* MUNDO                                                              */
/*                                                                    */
/* O cenário: cores e desenho dos blocos, o mapa de alturas, a matriz */
/* 3D do mundo e a construção do terreno, da árvore e das nuvens. A   */
/* casinha de vidro e o desenho da cena em três passadas ficam em     */
/* TEXTURAS E TRANSPARÊNCIA.                                          */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/* Cores sólidas: só para o que não tem textura (nuvem e as partes do */
/* personagem). Escolhe a cor de UMA face a partir de:                */
/*   tipo    qual peça é, define a cor base                           */
/*   brilho  quanto clarear ou escurecer a face (ver desenhaBloco)    */
/*   tom     pequena variação entre blocos vizinhos (xadrez), para    */
/*           dar para enxergar os blocos de uma nuvem                 */
/* A cor final é a cor base multiplicada por brilho e tom.            */
/* ------------------------------------------------------------------ */

void corDaFace(int tipo, float brilho, float tom)
{
    float r, g, b;                          /* vermelho, verde, azul, de 0 a 1 */

    switch (tipo) {
        case NUVEM:   r = 1.00f; g = 1.00f; b = 1.00f; break;
        case PELE:    r = 0.80f; g = 0.60f; b = 0.47f; break;   /* personagem: */
        case CABELO:  r = 0.30f; g = 0.20f; b = 0.12f; break;
        case CAMISA:  r = 0.00f; g = 0.62f; b = 0.66f; break;
        case CALCA:   r = 0.22f; g = 0.22f; b = 0.55f; break;
        case SAPATO:  r = 0.35f; g = 0.35f; b = 0.35f; break;
        case OLHO_BRANCO: r = 0.95f; g = 0.95f; b = 0.95f; break;
        case OLHO:    r = 0.30f; g = 0.25f; b = 0.65f; break;
        case TOCHA:   r = 1.00f; g = 0.34f; b = 0.04f; break;
        case MACA:    r = 0.88f; g = 0.08f; b = 0.06f; break;
        case MINERIO_CARVAO: r = 0.08f; g = 0.08f; b = 0.09f; break;
        case MINERIO_FERRO:  r = 0.62f; g = 0.65f; b = 0.68f; break;
        case PICARETA:       r = 0.36f; g = 0.23f; b = 0.12f; break;
        case ESPADA:         r = 0.72f; g = 0.78f; b = 0.86f; break;
        case CRISTAL:        r = 0.18f; g = 0.82f; b = 0.95f; break;
        default:     r = 1.00f; g = 0.00f; b = 1.00f; break;   /* rosa = tipo desconhecido */
    }

    glColor3f(r * brilho * tom, g * brilho * tom, b * brilho * tom);
}

/* ------------------------------------------------------------------ */
/* Um bloco: cubo de lado 1, do canto (0,0,0) ao canto (1,1,1).       */
/* São 6 faces, cada uma um GL_QUADS de 4 vértices, descritas na      */
/* tabela abaixo. Os vértices de cada face estão em sentido           */
/* anti-horário quando vistos de fora do cubo, começando pelo canto   */
/* de baixo à esquerda. É assim que o OpenGL sabe qual lado da face é */
/* a frente, e o GL_CULL_FACE pode pular as faces viradas de costas   */
/* para a câmera (semanas 2 e 3).                                     */
/*                                                                    */
/* faces diz quais faces desenhar (soma de bits FACE_): blocoEm()     */
/* pula as que um vizinho esconde; o personagem pede TODAS_FACES.     */
/*                                                                    */
/* Brilho por face: combina a orientação da face, a posição aparente do
   sol e as fontes artificiais próximas, preservando o estilo facetado. */
/*                                                                    */
/* Textura: antes de cada glVertex3f vai um glTexCoord2f, com o canto */
/* correspondente da célula da face. A coordenada de textura é estado */
/* corrente aplicado ao próximo vértice, como a cor. Com GL_MODULATE, */
/* cor final = texel x cor do vértice; por isso a cor do vértice é um */
/* cinza do brilho da face, e o texel verde de uma lateral sai mais   */
/* escuro que o do topo, como antes com as cores sólidas.             */
/* A primeira linha do arquivo de imagem corresponde a t = 0, então o */
/* alto da célula é t0 e o pé é t0 + lado: invertido em relação ao y  */
/* da face (t' = 1 - t). Sem isso a grama ficaria embaixo da lateral. */
/* ------------------------------------------------------------------ */

static const int bitDaFace[6] = {
    FACE_FRENTE, FACE_TRAS, FACE_DIREITA, FACE_ESQUERDA, FACE_TOPO, FACE_BASE
};

static float brilhoDaFaceAtual(int face)
{
    static const float normalX[6] = { 0, 0, 1, -1, 0, 0 };
    static const float normalY[6] = { 0, 0, 0, 0, 1, -1 };
    static const float normalZ[6] = { 1, -1, 0, 0, 0, 0 };
    float angulo = (tempoMundo - 0.25f) * 2.0f * PI;
    float luzX = cosf(angulo);
    float luzY = sinf(angulo);
    float luzZ = 0.35f;
    float comprimento = sqrtf(luzX * luzX + luzY * luzY + luzZ * luzZ);
    float dia = sinf(angulo);
    float difusa, ambiente, brilho;
    int i;

    luzX /= comprimento;
    luzY /= comprimento;
    luzZ /= comprimento;
    if (dia < 0.0f) dia = 0.0f;

    ambiente = 0.12f + 0.12f * (1.0f - dia);
    difusa = 0.78f * dia *
             (normalX[face] * luzX + normalY[face] * luzY + normalZ[face] * luzZ);
    if (difusa < 0.0f) difusa = 0.0f;
    brilho = ambiente + difusa;

    /* Luzes artificiais são avaliadas por distância. A contribuição é
       aditiva, portanto várias tochas próximas se combinam naturalmente. */
    for (i = 0; i < numeroLuzes; i++) {
        float dx = pontoDesenhoX - luzes[i].x;
        float dy = pontoDesenhoY - luzes[i].y;
        float dz = pontoDesenhoZ - luzes[i].z;
        float distancia = sqrtf(dx * dx + dy * dy + dz * dz);
        if (distancia < luzes[i].alcance)
            brilho += luzes[i].intensidade *
                      (1.0f - distancia / luzes[i].alcance);
    }

    if (face == 5) brilho *= 0.60f;
    else if (face == 0 || face == 1) brilho *= 0.88f;
    else if (face == 2 || face == 3) brilho *= 0.78f;

    if (brilho < 0.10f) brilho = 0.10f;
    if (brilho > 1.0f) brilho = 1.0f;
    return brilho;
}

static const float verticesDaFace[6][4][3] = {
    { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} },     /* frente (z = 1) */
    { {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} },     /* trás (z = 0) */
    { {1,0,1}, {1,0,0}, {1,1,0}, {1,1,1} },     /* direita (x = 1) */
    { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} },     /* esquerda (x = 0) */
    { {0,1,1}, {1,1,1}, {1,1,0}, {0,1,0} },     /* topo (y = 1) */
    { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} },     /* base (y = 0) */
};

void desenhaBloco(int tipo, float tom, int faces)
{
    int texturizado = celulaDaFace(tipo, FACE_TOPO) != SEM_TEXTURA;
    int f, v;

    /* ligar e desligar a textura é proibido entre glBegin e glEnd */
    if (texturizado) glEnable(GL_TEXTURE_2D);
    else             glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    for (f = 0; f < 6; f++) {
        if (!(faces & bitDaFace[f])) continue;

        if (texturizado) {
            float s0, t0, lado;
            float b = brilhoDaFaceAtual(f);

            coordenadasDaCelula(celulaDaFace(tipo, bitDaFace[f]), &s0, &t0, &lado);
            /* s e t de cada canto, na ordem dos vértices da tabela:
               baixo-esquerda, baixo-direita, cima-direita, cima-esquerda */
            float s[4] = { s0, s0 + lado, s0 + lado, s0 };
            float t[4] = { t0 + lado, t0 + lado, t0, t0 };

            glColor3f(b, b, b);
            for (v = 0; v < 4; v++) {
                glTexCoord2f(s[v], t[v]);
                glVertex3f(verticesDaFace[f][v][0], verticesDaFace[f][v][1],
                           verticesDaFace[f][v][2]);
            }
        } else {
            corDaFace(tipo, brilhoDaFaceAtual(f), tom);
            for (v = 0; v < 4; v++)
                glVertex3f(verticesDaFace[f][v][0], verticesDaFace[f][v][1],
                           verticesDaFace[f][v][2]);
        }
    }
    glEnd();
}

/* ------------------------------------------------------------------ */
/* Instância (semana 3): desenha um bloco do tipo pedido na posição   */
/* (x, y, z) do mundo. Todos os blocos da cena acabam aqui.           */
/*                                                                    */
/*   glPushMatrix   guarda a transformação atual                      */
/*   glTranslatef   desloca a origem para (x, y, z)                   */
/*   desenhaBloco   desenha o cubo de sempre, agora na nova origem    */
/*   glPopMatrix    desfaz o deslocamento, para o próximo bloco       */
/*                  partir do mesmo ponto                             */
/*                                                                    */
/* Como o cubo tem lado 1 e as posições são inteiras, cada bloco      */
/* termina exatamente onde o vizinho começa: eles se encaixam numa    */
/* grade, sem buracos.                                                */
/* ------------------------------------------------------------------ */

static void desenhaTocha(float tom)
{
    /* haste: estreita, centralizada na célula e apoiada no bloco */
    glPushMatrix();
    glTranslatef(0.43f, 0.0f, 0.43f);
    glScalef(0.14f, 0.70f, 0.14f);
    desenhaBloco(TRONCO, tom, TODAS_FACES);
    glPopMatrix();

    /* chama em dois volumes pequenos, mais próxima da proporção da
       referência e sem esconder o bloco de apoio. */
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 0.25f, 0.02f);
    glPushMatrix();
    glTranslatef(0.50f, 0.78f, 0.50f);
    glScalef(0.24f, 0.22f, 0.24f);
    glutSolidCube(1.0f);
    glPopMatrix();
    glColor3f(1.0f, 0.78f, 0.08f);
    glPushMatrix();
    glTranslatef(0.50f, 0.92f, 0.50f);
    glScalef(0.13f, 0.10f, 0.13f);
    glutSolidCube(1.0f);
    glPopMatrix();
    glEnable(GL_TEXTURE_2D);
}

void blocoEm(int tipo, int x, int y, int z)
{
    /* soma par: tom normal; soma ímpar: um pouco mais escuro (xadrez).
       Só vale para o que tem cor sólida: a textura já mostra onde cada
       bloco termina */
    float tom = ((x + y + z) % 2 == 0) ? 1.0f : 0.92f;
    int faces = facesVisiveis(x, y, z);     /* sem as que um vizinho esconde */

    if (faces == 0 && tipo != TOCHA)
        return;                             /* bloco enterrado: nada a desenhar */

    pontoDesenhoX = x + 0.5f;
    pontoDesenhoY = y + 0.5f;
    pontoDesenhoZ = z + 0.5f;
    glPushMatrix();
    glTranslatef((float)x, (float)y, (float)z);
    if (tipo == TOCHA)
        desenhaTocha(tom);
    else
        desenhaBloco(tipo, tom, faces);
    glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Mapa: o mundo visto de cima. Cada número é a altura de uma coluna  */
/* de blocos. Cada linha é um valor de z (de trás para a frente) e    */
/* cada número na linha é um valor de x (da esquerda para a direita). */
/*                                                                    */
/*   0     mar (areia no fundo, água por cima)                        */
/*   1     praia                                                      */
/*   2+    terra firme com grama no topo; quanto maior, mais alto     */
/*   6+    (NIVEL_NEVE) topo coberto de neve                          */
/*                                                                    */
/* O mapa guarda só a ALTURA de cada coluna. O tipo de cada bloco da  */
/* coluna (grama, terra, pedra...) é decidido depois, em colunaEm().  */
/*                                                                    */
/* O quadrado de x 4 a 7 e z 7 a 10 é todo de altura 2: um chão plano */
/* para a casinha de vidro (ver casinhaEm).                           */
/*                                                                    */
/* Para mudar o cenário, basta editar os números. Experimente pôr um  */
/* 0 no meio do gramado (lago) ou um 9 (torre).                       */
/* ------------------------------------------------------------------ */

int mapa[LADO][LADO] = {
/*  x: 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15                 */
    {  3, 3, 4, 4, 5, 5, 5, 4, 4, 3, 3, 2, 2, 2, 3, 3 },   /* z =  0 */
    {  3, 4, 5, 5, 6, 6, 5, 5, 4, 3, 2, 2, 2, 3, 3, 3 },   /* z =  1 */
    {  4, 5, 6, 6, 7, 7, 6, 5, 4, 3, 2, 2, 2, 2, 3, 4 },   /* z =  2 */
    {  4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 2, 2, 2, 2, 3 },   /* z =  3  pico (8) */
    {  4, 5, 6, 7, 7, 7, 6, 5, 3, 3, 2, 2, 2, 2, 2, 2 },   /* z =  4 */
    {  3, 4, 5, 6, 6, 6, 5, 4, 3, 2, 2, 2, 2, 2, 2, 2 },   /* z =  5 */
    {  3, 4, 4, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 2, 2, 2 },   /* z =  6 */
    {  3, 3, 4, 4, 2, 2, 2, 2, 2, 2, 1, 0, 0, 1, 2, 2 },   /* z =  7  laguinho */
    {  2, 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 0, 0, 1, 2, 2 },   /* z =  8 */
    {  2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2 },   /* z =  9 */
    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },   /* z = 10 */
    {  1, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2 },   /* z = 11 */
    {  1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1 },   /* z = 12  praia */
    {  0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0 },   /* z = 13 */
    {  0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },   /* z = 14  mar */
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   /* z = 15 */
};

/* Consulta a altura de um quadrado do mapa. Atenção à ordem dos
   índices: primeiro a linha (z), depois a coluna (x). É por isso que a
   tabela acima aparece no código do mesmo jeito que o mundo visto de
   cima. */
int alturaDoTerreno(int x, int z)
{
    if (x < 16 && z < 16) return mapa[z][x];

    return alturaProcedural(x, z);
}

/* ------------------------------------------------------------------ */
/* Matriz do mundo: o tipo de cada bloco, indexado como               */
/* mundo[x][y][z] (o mapa é [z][x]). A célula que contém um ponto é   */
/* (floor(x), floor(y), floor(z)), por isso a colisão é uma consulta  */
/* direta à matriz. Todo acesso passa por colocaBloco() e             */
/* blocoNoMundo(), que testam os limites.                             */
/* ------------------------------------------------------------------ */

/* Procura o primeiro bloco não vazio usando travessia DDA 3D. O algoritmo
   visita cada célula atravessada pelo raio, sem amostrar repetidamente a
   mesma célula; isso reduz o custo da mineração e funciona bem nas bordas
   entre chunks. */
static int raioAtingeJogador(float ox, float oy, float oz,
                             float dx, float dy, float dz,
                             float *entrada)
{
    float minimo[3] = { jogadorX - LARGURA_JOGADOR / 2.0f,
                        jogadorY, jogadorZ - LARGURA_JOGADOR / 2.0f };
    float maximo[3] = { jogadorX + LARGURA_JOGADOR / 2.0f,
                        jogadorY + ALTURA_JOGADOR,
                        jogadorZ + LARGURA_JOGADOR / 2.0f };
    float origem[3] = { ox, oy, oz };
    float direcao[3] = { dx, dy, dz };
    float tEntrada = 0.0f, tSaida = 8.0f;
    int eixo;

    for (eixo = 0; eixo < 3; eixo++) {
        float t1, t2, temporario;
        if (fabsf(direcao[eixo]) < 0.00001f) {
            if (origem[eixo] < minimo[eixo] || origem[eixo] > maximo[eixo])
                return 0;
            continue;
        }
        t1 = (minimo[eixo] - origem[eixo]) / direcao[eixo];
        t2 = (maximo[eixo] - origem[eixo]) / direcao[eixo];
        if (t1 > t2) {
            temporario = t1;
            t1 = t2;
            t2 = temporario;
        }
        if (t1 > tEntrada) tEntrada = t1;
        if (t2 < tSaida) tSaida = t2;
        if (tEntrada > tSaida) return 0;
    }
    *entrada = tEntrada < 0.0f ? 0.0f : tEntrada;
    return 1;
}

int blocoMirado(HitBloco *hit)
{
    float olhoX, olhoY, olhoZ, dx, dy, dz;
    float limiteJogador = FLT_MAX;
    int x, y, z, anteriorX, anteriorY, anteriorZ;
    int passoX, passoY, passoZ;
    float t = 0.0f, tMaxX, tMaxY, tMaxZ;
    float tDeltaX, tDeltaY, tDeltaZ;

    hit->encontrado = 0;
    posicaoDoOlho(&olhoX, &olhoY, &olhoZ);
    direcaoDoRaio(&dx, &dy, &dz);
    if (modoCamera == TERCEIRA_PESSOA)
        raioAtingeJogador(olhoX, olhoY, olhoZ, dx, dy, dz,
                          &limiteJogador);

    x = (int)floorf(olhoX);
    y = (int)floorf(olhoY);
    z = (int)floorf(olhoZ);
    anteriorX = x; anteriorY = y; anteriorZ = z;

    passoX = dx >= 0.0f ? 1 : -1;
    passoY = dy >= 0.0f ? 1 : -1;
    passoZ = dz >= 0.0f ? 1 : -1;
    tDeltaX = fabsf(dx) > 0.00001f ? 1.0f / fabsf(dx) : FLT_MAX;
    tDeltaY = fabsf(dy) > 0.00001f ? 1.0f / fabsf(dy) : FLT_MAX;
    tDeltaZ = fabsf(dz) > 0.00001f ? 1.0f / fabsf(dz) : FLT_MAX;
    tMaxX = fabsf(dx) > 0.00001f
          ? (passoX > 0 ? (x + 1.0f - olhoX) : (olhoX - x)) / fabsf(dx)
          : FLT_MAX;
    tMaxY = fabsf(dy) > 0.00001f
          ? (passoY > 0 ? (y + 1.0f - olhoY) : (olhoY - y)) / fabsf(dy)
          : FLT_MAX;
    tMaxZ = fabsf(dz) > 0.00001f
          ? (passoZ > 0 ? (z + 1.0f - olhoZ) : (olhoZ - z)) / fabsf(dz)
          : FLT_MAX;

    while (t <= 8.0f) {
        int tipo = blocoNoMundo(x, y, z);
        if (t >= limiteJogador - 0.0001f)
            return 0;
        if (tipo != VAZIO) {
            hit->encontrado = 1;
            hit->x = x; hit->y = y; hit->z = z;
            hit->adjX = anteriorX; hit->adjY = anteriorY; hit->adjZ = anteriorZ;
            hit->tipo = tipo;
            return 1;
        }

        anteriorX = x; anteriorY = y; anteriorZ = z;
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            x += passoX;
            t = tMaxX;
            tMaxX += tDeltaX;
        } else if (tMaxY < tMaxZ) {
            y += passoY;
            t = tMaxY;
            tMaxY += tDeltaY;
        } else {
            z += passoZ;
            t = tMaxZ;
            tMaxZ += tDeltaZ;
        }
    }
    return 0;
}

/* Interação de mineração/construção. O raycast seleciona o primeiro bloco
   visível e os botões do mouse quebram, coletam e colocam itens da hotbar. */
void interageComBloco(int botao)
{
    HitBloco hit;

    /* A maçã é consumida com o botão direito, sem depender de haver um
       bloco na mira. O F continua funcionando como atalho de teclado. */
    if (botao == GLUT_RIGHT_BUTTON && tiposItens[itemSelecionado] == MACA) {
        iniciaAnimacaoBraco(1);
        consomeAlimento();
        return;
    }

    /* O ataque desarmado também é permitido. Se houver um inimigo na
       linha da mira, o clique vira ataque; caso contrário continua sendo
       mineração. A espada apenas aumenta o dano e gasta durabilidade. */
    if (botao == GLUT_LEFT_BUTTON ||
        (botao == GLUT_RIGHT_BUTTON && tiposItens[itemSelecionado] == ESPADA)) {
        iniciaAnimacaoBraco(1);
        if (atacaInimigo()) return;
    }

    if (!blocoMirado(&hit)) return;

    if (botao == GLUT_LEFT_BUTTON) {
        if ((hit.tipo == MINERIO_CARVAO || hit.tipo == MINERIO_FERRO) &&
            tiposItens[itemSelecionado] != PICARETA)
            return;
        if (tiposItens[itemSelecionado] == PICARETA &&
            !gastaDurabilidade(PICARETA))
            return;
        criaDrop(hit.tipo, hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f);
        audioQuebraBloco(hit.tipo);
        emiteParticulasBloco(hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f, hit.tipo);
        /* Folhas podem soltar maçãs, criando uma fonte renovável simples
           de alimento sem introduzir ainda um sistema de entidades. */
        if (hit.tipo == FOLHA &&
            (hit.x + hit.y + hit.z) % 4 == 0)
            criaDrop(MACA, hit.x + 0.5f, hit.y + 0.8f, hit.z + 0.5f);
        colocaBloco(VAZIO, hit.x, hit.y, hit.z);
        reconstroiLuzes();
    } else if (botao == GLUT_RIGHT_BUTTON) {
        int indice = itemSelecionado;
        int tipo = tiposItens[indice];

        /* Alimentos são itens de uso, não blocos da matriz do mundo. */
        if (tipo == MACA || tipo == PICARETA || tipo == ESPADA) return;

        if (quantidadesItens[indice] <= 0 ||
            !dentroDoMundo(hit.adjX, hit.adjY, hit.adjZ) ||
            blocoNoMundo(hit.adjX, hit.adjY, hit.adjZ) != VAZIO)
            return;

        if (!posicaoLivre(jogadorX, jogadorY, jogadorZ)) return;
        colocaBloco(tipo, hit.adjX, hit.adjY, hit.adjZ);
        if (!posicaoLivre(jogadorX, jogadorY, jogadorZ)) {
            colocaBloco(VAZIO, hit.adjX, hit.adjY, hit.adjZ);
            return;
        }
        removeDoInventario(tipo, 1);
        iniciaAnimacaoBraco(2);
        reconstroiLuzes();
    }
}

/* ------------------------------------------------------------------ */
/* Uma coluna do terreno na posição (x, z) do chão.                   */
/* Lê a altura h no mapa e coloca blocos de y = 0 até o topo. Para    */
/* cada andar y, a sequência de if escolhe o tipo comparando y com h: */
/*                                                                    */
/*   y acima do chão (y > h)      AGUA   só acontece em colunas mais  */
/*                                       baixas que NIVEL_AGUA (mar)  */
/*   coluna baixa (h <= NIVEL)    AREIA  praia e fundo do mar         */
/*   topo de coluna alta          NEVE   y == h e h >= NIVEL_NEVE     */
/*   último andar (y == h)        GRAMA  o topo das demais colunas    */
/*   2 andares abaixo do topo     TERRA                               */
/*   o resto, mais embaixo        PEDRA                               */
/*                                                                    */
/* A ordem dos if importa: o primeiro que for verdadeiro decide. Por  */
/* isso a NEVE vem antes da GRAMA; se viesse depois, nunca seria      */
/* escolhida, porque o topo já teria virado grama.                    */
/*                                                                    */
/* Exemplo, coluna de altura 6: PEDRA de y = 0 a 3, TERRA em 4 e 5,   */
/* NEVE em 6. Coluna de altura 4: PEDRA em 0 e 1, TERRA em 2 e 3,     */
/* GRAMA em 4. Coluna do mar (altura 0): AREIA em 0, AGUA em 1.       */
/* ------------------------------------------------------------------ */

void colunaEm(int x, int z)
{
    int h = alturaDoTerreno(x, z);
    int procedural = x >= 16 || z >= 16;
    int bioma = procedural ? biomaProcedural(x, z) : 0;
    int y;

    /* sobe até o topo da coluna, ou até o nível da água se ela for mais
       baixa, para preencher o mar */
    for (y = 0; y <= h || y <= NIVEL_AGUA; y++) {
        int minerio = procedural ? minerioProcedural(x, y, z) : 0;
        if (procedural && y > 1 && y < h - 1 &&
            cavernaProcedural(x, y, z)) {
            colocaBloco(VAZIO, x, y, z);
        } else if (y > h) {
            colocaBloco(AGUA, x, y, z);
        } else if (h <= NIVEL_AGUA || (bioma == 1 && y >= h - 3)) {
            colocaBloco(AREIA, x, y, z);
        } else if (y == h && (h >= NIVEL_NEVE || bioma == 2)) {
            colocaBloco(NEVE, x, y, z);
        } else if (y == h) {
            colocaBloco(GRAMA, x, y, z);
        } else if (minerio != 0) {
            colocaBloco(minerio, x, y, z);
        } else if (y >= h - 2) {
            colocaBloco(TERRA, x, y, z);
        } else {
            colocaBloco(PEDRA, x, y, z);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Árvore no formato do Minecraft, plantada no quadrado (x, z).       */
/* Primeiro descobre a altura do chão ali, para a árvore nascer em    */
/* cima da grama. Depois empilha, a partir do chão:                   */
/*                                                                    */
/*   chao+7          F            1 folha no topo                     */
/*   chao+6       F  F  F         camada 3 x 3                        */
/*   chao+5    F  F  T  F  F      camada 5 x 5, tronco no meio        */
/*   chao+4    F  F  T  F  F      camada 5 x 5, tronco no meio        */
/*   chao+3          T                                                */
/*   chao+2          T                                                */
/*   chao+1          T            tronco: 5 blocos                    */
/*   chao      (grama do mapa)                                        */
/*                                                                    */
/* dx e dz são deslocamentos em relação ao tronco: dx = -2 é dois     */
/* blocos à esquerda, dx = 2 é dois à direita, e assim por diante.    */
/* ------------------------------------------------------------------ */

void arvoreEm(int x, int z)
{
    int chao = alturaDoTerreno(x, z);
    int y, dx, dz;

    /* tronco: mesma posição (x, z), subindo 5 andares */
    for (y = chao + 1; y <= chao + 5; y++)
        colocaBloco(TRONCO, x, y, z);

    /* duas camadas largas de folhas, de 2 para cada lado do tronco */
    for (y = chao + 4; y <= chao + 5; y++)
        for (dx = -2; dx <= 2; dx++)
            for (dz = -2; dz <= 2; dz++)
                if (dx != 0 || dz != 0)     /* pula o centro: ali está o tronco */
                    colocaBloco(FOLHA, x + dx, y, z + dz);

    /* camada menor, de 1 para cada lado; o tronco já acabou, então o
       centro também recebe folha */
    for (dx = -1; dx <= 1; dx++)
        for (dz = -1; dz <= 1; dz++)
            colocaBloco(FOLHA, x + dx, chao + 6, z + dz);

    /* folha do topo, bem em cima do tronco */
    colocaBloco(FOLHA, x, chao + 7, z);
}

/* ------------------------------------------------------------------ */
/* Nuvens: a aparência agora é gerada pelo shader do céu.             */
/* A função permanece como ponto de compatibilidade para a construção  */
/* do mundo e não cria mais placas estáticas de blocos.                */
/* ------------------------------------------------------------------ */

void nuvemEm(int x, int z, int largura, int fundo)
{
    (void)x;
    (void)z;
    (void)largura;
    (void)fundo;
}

static void adicionaLuz(float x, float y, float z,
                        float intensidade, float alcance)
{
    if (numeroLuzes >= MAX_LUZES) return;
    luzes[numeroLuzes].x = x;
    luzes[numeroLuzes].y = y;
    luzes[numeroLuzes].z = z;
    luzes[numeroLuzes].intensidade = intensidade;
    luzes[numeroLuzes].alcance = alcance;
    numeroLuzes++;
}

void reconstroiLuzes(void)
{
    int x, y, z;

    numeroLuzes = 0;
    for (x = 0; x < LADO; x++)
        for (y = 0; y < ALTURA_MUNDO; y++)
            for (z = 0; z < LADO; z++)
                if (mundo[x][y][z] == TOCHA)
                adicionaLuz(x + 0.5f, y + 0.5f, z + 0.5f,
                                0.85f, 6.0f);
                else if (mundo[x][y][z] == CRISTAL)
                    adicionaLuz(x + 0.5f, y + 0.5f, z + 0.5f,
                                0.45f, 4.0f);
}

/* ------------------------------------------------------------------ */
/* Preenche a matriz uma vez, no início: zera tudo e coloca o         */
/* terreno, a árvore, a casinha de vidro e as nuvens. A árvore e a   */
/* casinha vêm depois do terreno e escrevem por cima.                 */
/* ------------------------------------------------------------------ */

void construirMundo(void)
{
    int x, y, z;

    for (x = 0; x < LADO; x++)
        for (y = 0; y < ALTURA_MUNDO; y++)
            for (z = 0; z < LADO; z++)
                mundo[x][y][z] = VAZIO;

    for (x = 0; x < LADO; x++)
        for (z = 0; z < LADO; z++)
            colunaEm(x, z);

    /* Duas tochas de demonstração para que a iluminação artificial seja
       perceptível mesmo antes de existir a colocação de tochas pelo jogador. */
    colocaBloco(TOCHA, 3, 5, 7);
    colocaBloco(TOCHA, 9, 3, 10);

    arvoreEm(13, 6);                        /* no gramado, à direita da montanha */
    casinhaEm(4, 7);                        /* à esquerda de onde o personagem começa */
    arvoreEm(24, 7);                         /* extensão procedural do mundo */
    arvoreEm(27, 20);
    casinhaEm(20, 17);

    /* Vegetação determinística nas áreas de floresta da extensão. */
    for (x = 18; x < LADO - 2; x += 4)
        for (z = 2; z < LADO - 2; z += 5)
            if (biomaProcedural(x, z) == 3 && alturaDoTerreno(x, z) > NIVEL_AGUA)
                arvoreEm(x, z);

    /* Cristais luminosos são uma assinatura original do projeto: surgem
       em pequenos campos de tundra e iluminam o terreno à noite. */
    for (x = 18; x < LADO - 1; x += 5)
        for (z = 3; z < LADO - 1; z += 6)
            if (biomaProcedural(x, z) == 2) {
                int cristalY = alturaDoTerreno(x, z) + 1;
                if (cristalY < ALTURA_MUNDO)
                    colocaBloco(CRISTAL, x, cristalY, z);
            }

    nuvemEm(1, 2, 5, 3);                    /* sobre a montanha */
    nuvemEm(10, 0, 4, 2);                   /* sobre a árvore, mais ao fundo */
    nuvemEm(21, 8, 6, 3);                   /* sobre a extensão */

    reconstroiLuzes();
}

const int tiposItens[NUM_ITENS] = {
    GRAMA, TERRA, PEDRA, TRONCO, FOLHA, AREIA, TOCHA, NEVE, VIDRO, MACA,
    PICARETA, ESPADA, CRISTAL
};
int quantidadesItens[NUM_ITENS] = { 0 };
int itemSelecionado = 0;
int slotSelecionadoInventario = -1;
Inventory inventario;
ItemStack gradeCrafting[4];
ItemStack resultadoCrafting = { VAZIO, 0, 0 };

static int consomeAlimento(void)
{
    if (fomeJogador >= FOME_MAXIMA || itemSelecionado < 0 ||
        itemSelecionado >= NUM_ITENS || tiposItens[itemSelecionado] != MACA ||
        quantidadesItens[itemSelecionado] <= 0)
        return 0;

    if (!removeDoInventario(MACA, 1)) return 0;
    audioComer();
    fomeJogador += CURA_MACA;
    if (fomeJogador > FOME_MAXIMA) fomeJogador = FOME_MAXIMA;
    saturacaoJogador += SATURACAO_MACA;
    if (saturacaoJogador > fomeJogador) saturacaoJogador = fomeJogador;
    return 1;
}

static void interageSlotInventario(int slot, int botao)
{
    ItemStack *origem;
    ItemStack *destino;

    if (slot < 0 || slot >= INVENTORY_SLOTS) return;

    if (slotSelecionadoInventario < 0) {
        if (inventario.slots[slot].quantidade > 0)
            slotSelecionadoInventario = slot;
        return;
    }

    if (slotSelecionadoInventario == slot) {
        slotSelecionadoInventario = -1;
        return;
    }

    origem = &inventario.slots[slotSelecionadoInventario];
    destino = &inventario.slots[slot];

    if (botao == GLUT_RIGHT_BUTTON) {
        if (origem->quantidade > 0 &&
            (destino->quantidade == 0 || destino->tipo == origem->tipo) &&
            destino->quantidade < MAX_ITEM_STACK) {
            destino->tipo = origem->tipo;
            destino->quantidade++;
            origem->quantidade--;
            if (origem->quantidade == 0) {
                origem->tipo = VAZIO;
                slotSelecionadoInventario = -1;
            }
        }
    } else if (destino->quantidade == 0) {
        *destino = *origem;
        origem->tipo = VAZIO;
        origem->quantidade = 0;
        slotSelecionadoInventario = -1;
    } else if (destino->tipo == origem->tipo &&
               destino->quantidade < MAX_ITEM_STACK) {
        int espaco = MAX_ITEM_STACK - destino->quantidade;
        int movido = origem->quantidade < espaco
                    ? origem->quantidade : espaco;
        destino->quantidade += movido;
        origem->quantidade -= movido;
        if (origem->quantidade == 0) {
            origem->tipo = VAZIO;
            slotSelecionadoInventario = -1;
        }
    } else {
        ItemStack temporario = *destino;
        *destino = *origem;
        *origem = temporario;
        slotSelecionadoInventario = -1;
    }

    if (slot < NUM_ITENS) itemSelecionado = slot;
    sincronizaHotbar();
}

static int slotCraftingEm(int x, int y)
{
    const int origemX = 260, origemY = 470, lado = 58, espaco = 6;
    int coluna, linha;

    y = alturaJanela - y;
    x -= origemX;
    y -= origemY;
    if (x < 0 || y < 0) return -1;
    coluna = x / (lado + espaco);
    linha = y / (lado + espaco);
    if (coluna < 0 || coluna >= 2 || linha < 0 || linha >= 2) return -1;
    if (x % (lado + espaco) >= lado || y % (lado + espaco) >= lado) return -1;
    return linha * 2 + coluna;
}

static void interageCrafting(int x, int y, int botao)
{
    const int invX = 220, invY = 72, lado = 58, espaco = 6;
    int slotCraft = slotCraftingEm(x, y);
    int i, slotInventario = -1;

    /* saída: um clique fabrica a receita e consome os ingredientes. */
    if (x >= 520 && x <= 520 + lado &&
        alturaJanela - y >= 470 && alturaJanela - y <= 470 + lado &&
        resultadoCrafting.quantidade > 0) {
        if (adicionaAoInventario(resultadoCrafting.tipo, resultadoCrafting.quantidade)
            == resultadoCrafting.quantidade)
            consomeReceita();
        return;
    }

    if (slotCraft >= 0) {
        ItemStack *slot = &gradeCrafting[slotCraft];
        int pedido = botao == GLUT_RIGHT_BUTTON ? 1 : slot->quantidade;
        int devolvido = 0;
        if (slot->quantidade > 0) {
            devolvido = adicionaAoInventario(slot->tipo, pedido);
            slot->quantidade -= devolvido;
            if (slot->quantidade == 0) slot->tipo = VAZIO;
            if (devolvido > 0) atualizaReceita();
        }
        return;
    }

    /* Os 36 slots do inventário ocupam quatro linhas na parte inferior. */
    y = alturaJanela - y;
    x -= invX;
    y -= invY;
    if (x < 0 || y < 0) return;
    {
        int coluna = x / (lado + espaco);
        int linha = y / (lado + espaco);
        if (coluna < 0 || coluna >= 9 || linha < 0 || linha >= 4) return;
        if (x % (lado + espaco) >= lado || y % (lado + espaco) >= lado) return;
        slotInventario = linha * 9 + coluna;
    }

    if (inventario.slots[slotInventario].quantidade <= 0) return;
    for (i = 0; i < 4; i++)
        if (gradeCrafting[i].quantidade == 0) {
            gradeCrafting[i].tipo = inventario.slots[slotInventario].tipo;
            gradeCrafting[i].quantidade = 1;
            removeDoInventario(gradeCrafting[i].tipo, 1);
            atualizaReceita();
            return;
        }
}

static void iniciaInventario(void)
{
    /* Estoque inicial pequeno e previsível. Os blocos do mapa não são mais
       tratados como inventário: só entram na barra quando são coletados. */
    int i;
    slotSelecionadoInventario = -1;
    for (i = 0; i < INVENTORY_SLOTS; i++) {
        inventario.slots[i].tipo = VAZIO;
        inventario.slots[i].quantidade = 0;
    }

    adicionaAoInventario(GRAMA, 64);
    adicionaAoInventario(TERRA, 64);
    adicionaAoInventario(PEDRA, 64);
    adicionaAoInventario(TRONCO, 32);
    adicionaAoInventario(FOLHA, 32);
    adicionaAoInventario(AREIA, 32);
    adicionaAoInventario(TOCHA, 8);
    adicionaAoInventario(NEVE, 32);
    adicionaAoInventario(VIDRO, 16);
    adicionaAoInventario(MACA, 12);
    adicionaAoInventario(PICARETA, 1);
    adicionaAoInventario(ESPADA, 1);
}


/* ================================================================== */
/* COLISÃO                                                            */
/*                                                                    */
/* A posição do personagem e a colisão com o mundo: que blocos são    */
/* sólidos, se a caixa do personagem cabe numa posição e como aplicar */
/* um deslocamento sem atravessar paredes.                            */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/* Posição do personagem                                              */
/* jogadorY é a altura dos PÉS; os olhos ficam ALTURA_OLHOS acima.    */
/* Só moveJogador() a altera, sempre depois de testar a colisão.      */
/* ------------------------------------------------------------------ */

/* Começa no meio do dia: sin(0.25 * 2*pi) = 1, céu claro e sol alto. */
float tempoMundo = 0.25f;      /* 0..1: posição dentro do ciclo */
static float tempoDanoFome = 0.0f;
float tempoRegeneracaoJogador = 0.0f;

/* ------------------------------------------------------------------ */
/* Uma célula impede a passagem? Fora do mundo conta como sólido      */
/* (paredes nas bordas, piso e teto); dentro, VAZIO e AGUA deixam     */
/* passar.                                                            */
/* ------------------------------------------------------------------ */

/* ================================================================== */
/* CÂMERA                                                             */
/*                                                                    */
/* Tudo o que decide COMO a cena é vista: a lente (projeção), a       */
/* direção do olhar a partir de yaw e pitch, a posição da câmera em   */
/* primeira e terceira pessoa e o desenho do personagem, que só aparece */
/* em terceira pessoa.                                                */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/* Orientação e modo de câmera                                        */
/* yaw é o giro em torno do eixo vertical (0 olha para -z) e pitch a  */
/* inclinação (0 olha na horizontal, positivo para cima). O teclado   */
/* e o mouse os alteram; posicionaCamera() os usa a cada quadro.      */
/* ------------------------------------------------------------------ */

float yaw   = 0.0f;
float pitch = 0.0f;
int modoCamera = TERCEIRA_PESSOA;    /* começa vendo o personagem */
static float tempoAnimacaoBraco = 0.0f;
static int tipoAnimacaoBraco = 0;    /* 1: ataque, 2: colocação */

/* ------------------------------------------------------------------ */
/* Janela e projeção                                                  */
/* ------------------------------------------------------------------ */

int larguraJanela = 1024;       /* atualizados em redimensiona() */
int alturaJanela  = 768;
int projecaoOrtogonal = 0;      /* 0 perspectiva, 1 ortográfica (tecla P) */
float campoDeVisao = 60.0f;     /* graus; alterado pela roda do mouse */

/* ------------------------------------------------------------------ */
/* Projeção: monta a "lente" da câmera na matriz GL_PROJECTION.       */
/* Perspectiva (gluPerspective): campo de visão, proporção da janela, */
/* plano próximo e distante; o que está longe fica menor. Ortográfica */
/* (glOrtho): uma caixa, em que o tamanho não depende da distância. A */
/* proporção evita a imagem esticada. O plano próximo é 0.1 nos dois  */
/* modos: positivo, senão blocos atrás do olho entrariam na imagem.   */
/* Chamada ao redimensionar, na tecla P e no zoom.                    */
/* ------------------------------------------------------------------ */

void aplicaProjecao(void)
{
    double aspecto = (double)larguraJanela / alturaJanela;

    glMatrixMode(GL_PROJECTION);            /* as próximas operações mexem na lente */
    glLoadIdentity();

    if (projecaoOrtogonal)
        glOrtho(-10.0 * aspecto, 10.0 * aspecto, -10.0, 10.0, 0.1, 200.0);
    else
        gluPerspective(campoDeVisao, aspecto, 0.1, 200.0);

    glMatrixMode(GL_MODELVIEW);             /* volta para a matriz dos objetos */
}

/* ------------------------------------------------------------------ */
/* Redimensionamento (semana 4)                                       */
/* A GLUT chama esta função ao abrir a janela e sempre que ela muda   */
/* de tamanho. Guarda o tamanho (a projeção e o centro do mouse       */
/* dependem dele), ajusta a área de desenho e refaz a lente.          */
/* ------------------------------------------------------------------ */

void redimensiona(int largura, int altura)
{
    if (altura == 0) altura = 1;            /* evita divisão por zero */

    larguraJanela = largura;
    alturaJanela  = altura;

    glViewport(0, 0, largura, altura);      /* desenha na janela inteira */
    aplicaProjecao();
}

/* ------------------------------------------------------------------ */
/* Direção do olhar a partir de yaw e pitch (coordenadas esféricas).  */
/* O pitch divide o vetor unitário entre vertical, sen(pitch), e      */
/* horizontal, cos(pitch); o yaw distribui a parte horizontal entre x */
/* e z, com yaw 0 olhando para -z. Quem chama soma o vetor ao olho    */
/* para obter o alvo do gluLookAt.                                    */
/* ------------------------------------------------------------------ */

void direcaoDoOlhar(float *dx, float *dy, float *dz)
{
    float ry = yaw   * PI / 180.0f;
    float rp = pitch * PI / 180.0f;

    *dx =  sinf(ry) * cosf(rp);
    *dy =  sinf(rp);
    *dz = -cosf(ry) * cosf(rp);
}

/* Direção que realmente atravessa o centro da tela. Em primeira pessoa
   ela coincide com yaw/pitch; em terceira pessoa é a direção entre o
   olho deslocado e o alvo usado por gluLookAt. */
void direcaoDoRaio(float *dx, float *dy, float *dz)
{
    if (modoCamera == PRIMEIRA_PESSOA) {
        direcaoDoOlhar(dx, dy, dz);
    } else {
        float olhoX, olhoY, olhoZ;
        float alvoX = jogadorX;
        float alvoY = jogadorY + ALTURA_OLHOS;
        float alvoZ = jogadorZ;
        float comprimento;

        posicaoDoOlho(&olhoX, &olhoY, &olhoZ);
        direcaoDoOlhar(dx, dy, dz);
        alvoY += *dy * DISTANCIA_CAMERA;
        *dx = alvoX - olhoX;
        *dy = alvoY - olhoY;
        *dz = alvoZ - olhoZ;
        comprimento = sqrtf(*dx * *dx + *dy * *dy + *dz * *dz);
        if (comprimento > 0.0001f) {
            *dx /= comprimento;
            *dy /= comprimento;
            *dz /= comprimento;
        }
    }
}

static void iniciaAnimacaoBraco(int tipo)
{
    tipoAnimacaoBraco = tipo;
    tempoAnimacaoBraco = 0.36f;
}

/* ------------------------------------------------------------------ */
/* Personagem, desenhado só em terceira pessoa, como modelo           */
/* hierárquico. O corpo é levado aos pés do jogador e girado por      */
/* -yaw, para a frente local (-z) coincidir com a do olhar. Cada      */
/* braço e perna é desenhado pendurado abaixo da origem, girado ali e */
/* só então levado à articulação, para girar em torno do ombro ou do  */
/* quadril.                                                           */
/*                                                                    */
/* O balanço é BALANCO_MAXIMO x amplitude x sen(fase); a fase avança  */
/* com a distância andada (ver atualiza). Tudo fica a menos de 0.3 do */
/* eixo vertical, dentro da caixa de colisão em qualquer giro.        */
/* ------------------------------------------------------------------ */

/* Uma peça do boneco: caixa com o canto mínimo em (x, y, z) do sistema
   local atual e as medidas dadas */
static void caixa(int tipo, float x, float y, float z,
                  float largura, float altura, float profundidade)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(largura, altura, profundidade);
    desenhaBloco(tipo, 1.0f, TODAS_FACES);
    glPopMatrix();
}

/* Braço ou perna pendurado na articulação (x, y), girado 'angulo' graus
   em torno do eixo x (positivo: para a frente). A parte de cima tem a
   cor tipoCima; os últimos alturaBaixo, a cor tipoBaixo. */
static void membro(float x, float y, float angulo, float largura,
                   int tipoCima, int tipoBaixo, float alturaBaixo)
{
    float meiaLargura = largura / 2.0f;
    float meiaProf = PROFUNDIDADE_CORPO / 2.0f;
    float alturaCima = COMPRIMENTO_MEMBRO - alturaBaixo;

    glPushMatrix();
    glTranslatef(x, y, 0.0f);                   /* 3. até a articulação */
    glRotatef(angulo, 1.0f, 0.0f, 0.0f);        /* 2. gira em torno dela */
    caixa(tipoCima,  -meiaLargura, -alturaCima, -meiaProf,       /* 1. pendurado */
          largura, alturaCima, PROFUNDIDADE_CORPO);
    caixa(tipoBaixo, -meiaLargura, -COMPRIMENTO_MEMBRO, -meiaProf,
          largura, alturaBaixo, PROFUNDIDADE_CORPO);
    glPopMatrix();
}

static void desenhaFerramentaNaMao(float angulo)
{
    int tipo = tiposItens[itemSelecionado];

    if (tipo != PICARETA && tipo != ESPADA) return;

    glPushMatrix();
    glTranslatef(0.20f, 2.0f * COMPRIMENTO_MEMBRO, -0.08f);
    glRotatef(angulo, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -COMPRIMENTO_MEMBRO - 0.08f, -0.08f);

    if (tipo == PICARETA) {
        /* Cabo marrom e cabeça metálica em forma de T. */
        caixa(PICARETA, -0.045f, -0.48f, -0.045f,
              0.09f, 0.62f, 0.09f);
        caixa(ESPADA, -0.30f, 0.08f, -0.045f,
              0.60f, 0.10f, 0.09f);
    } else {
        caixa(PICARETA, -0.045f, -0.48f, -0.045f,
              0.09f, 0.62f, 0.09f);
        caixa(ESPADA, -0.07f, 0.10f, -0.035f,
              0.14f, 0.48f, 0.07f);
    }
    glPopMatrix();
}

void desenhaJogador(void)
{
    float quadril = COMPRIMENTO_MEMBRO;                 /* altura das articulações */
    float ombro   = 2.0f * COMPRIMENTO_MEMBRO;
    float meiaProf = PROFUNDIDADE_CORPO / 2.0f;
    float meiaCabeca = LADO_CABECA / 2.0f;
    float xPerna = LARGURA_PERNA / 2.0f;                /* centro de cada perna */
    float xBraco = (LARGURA_TRONCO + LARGURA_BRACO) / 2.0f;
    float angulo = BALANCO_MAXIMO * amplitudeCaminhada * sinf(faseCaminhada);
    float gestoBraco = 0.0f;

    if (tempoAnimacaoBraco > 0.0f)
        gestoBraco = sinf((1.0f - tempoAnimacaoBraco / 0.36f) * PI) *
                     (tipoAnimacaoBraco == 1 ? 82.0f : 48.0f);

    glPushMatrix();
    glTranslatef(jogadorX, jogadorY, jogadorZ);         /* pés, centro da caixa */
    glRotatef(-yaw, 0.0f, 1.0f, 0.0f);                  /* frente do boneco = -z local */

    /* pernas (x negativo é a esquerda, x positivo a direita) */
    membro(-xPerna, quadril,  angulo, LARGURA_PERNA, CALCA, SAPATO, 0.1f);
    membro( xPerna, quadril, -angulo, LARGURA_PERNA, CALCA, SAPATO, 0.1f);

    /* tronco */
    caixa(CAMISA, -LARGURA_TRONCO / 2.0f, quadril, -meiaProf,
          LARGURA_TRONCO, COMPRIMENTO_MEMBRO, PROFUNDIDADE_CORPO);

    /* braços: manga curta e mão, em sentido oposto à perna do mesmo lado */
    membro(-xBraco, ombro, -angulo, LARGURA_BRACO, CAMISA, PELE, 0.45f);
    membro( xBraco, ombro,  angulo + gestoBraco,
            LARGURA_BRACO, CAMISA, PELE, 0.45f);
    desenhaFerramentaNaMao(angulo + gestoBraco);

    /* cabeça, de ombro até ombro + LADO_CABECA (perto de ALTURA_JOGADOR) */
    caixa(PELE, -meiaCabeca, ombro, -meiaCabeca,
          LADO_CABECA, LADO_CABECA, LADO_CABECA);

    /* cabelo: casca 0.01 maior dos lados, em cima e atrás, da altura dos
       olhos para cima; a frente dela fica dentro da cabeça, atrás do rosto */
    caixa(CABELO, -meiaCabeca - 0.01f, ombro + 0.15f, -meiaCabeca + 0.01f,
          LADO_CABECA + 0.02f, LADO_CABECA - 0.14f, LADO_CABECA);

    /* franja: faixa de cabelo no alto da testa */
    caixa(CABELO, -meiaCabeca, ombro + LADO_CABECA - 0.05f, -meiaCabeca - 0.01f,
          LADO_CABECA, 0.05f, 0.01f);

    /* olhos, na altura ALTURA_OLHOS: branco por fora, íris por dentro */
    caixa(OLHO_BRANCO, -0.14f, ALTURA_OLHOS - 0.03f, -meiaCabeca - 0.01f,
          0.1f, 0.05f, 0.01f);
    caixa(OLHO_BRANCO,  0.04f, ALTURA_OLHOS - 0.03f, -meiaCabeca - 0.01f,
          0.1f, 0.05f, 0.01f);
    caixa(OLHO, -0.09f, ALTURA_OLHOS - 0.03f, -meiaCabeca - 0.015f,
          0.05f, 0.05f, 0.01f);
    caixa(OLHO,  0.04f, ALTURA_OLHOS - 0.03f, -meiaCabeca - 0.015f,
          0.05f, 0.05f, 0.01f);

    glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Câmera: gluLookAt(olho, alvo, cima) com os mesmos dados nos dois   */
/* modos. Primeira pessoa: olho na altura dos olhos, alvo logo à      */
/* frente na direção do olhar. Terceira pessoa: olho DISTANCIA_CAMERA */
/* atrás, só no plano do chão, e ELEVACAO_CAMERA acima, para a câmera */
/* nunca entrar no chão; o alvo é a cabeça, deslocada na vertical     */
/* pelo pitch.                                                        */
/* ------------------------------------------------------------------ */

void posicionaCamera(void)
{
    float dx, dy, dz, olhoX, olhoY, olhoZ;
    float cabecaX = jogadorX;
    float cabecaY = jogadorY + ALTURA_OLHOS;
    float cabecaZ = jogadorZ;

    direcaoDoOlhar(&dx, &dy, &dz);
    posicaoDoOlho(&olhoX, &olhoY, &olhoZ);

    if (modoCamera == PRIMEIRA_PESSOA) {
        gluLookAt(olhoX, olhoY, olhoZ,
                  olhoX + dx, olhoY + dy, olhoZ + dz,
                  0.0, 1.0, 0.0);
    } else {
        gluLookAt(olhoX, olhoY, olhoZ,
                  cabecaX,                               /* olha o personagem, */
                  cabecaY + dy * DISTANCIA_CAMERA,       /* inclinado pelo pitch */
                  cabecaZ,
                  0.0, 1.0, 0.0);
    }
}


/* ================================================================== */
/* ENTRADA                                                            */
/*                                                                    */
/* Teclado (por evento e por estado), mouse (giro do olhar e captura do */
/* cursor), roda e teclas + e - (zoom), e a atualização do movimento a */
/* cada quadro, escalada pelo tempo de quadro.                        */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/* Teclado. A GLUT só avisa eventos (pressionar e soltar); o estado é */
/* construído aqui: teclas[] fica em 1 do pressionar ao soltar e é    */
/* lido a cada quadro em atualiza(). Por isso a repetição automática  */
/* do sistema está desligada (glutIgnoreKeyRepeat em main).           */
/*                                                                    */
/* As ações discretas (menu, cursor, projeção, câmera, zoom)          */
/* continuam por evento, uma vez por toque. tolower nos dois lados    */
/* evita tecla presa quando o Shift muda entre pressionar e soltar.   */
/* As setas usam o mesmo esquema em especiais[].                      */
/* ------------------------------------------------------------------ */

int teclas[256];        /* teclas de caractere: 1 pressionada, 0 solta */
int especiais[256];     /* teclas especiais (setas, F5): idem */
void teclado(unsigned char tecla, int x, int y)
{
    (void)x; (void)y;                       /* posição do mouse: não usada */

    teclas[tolower(tecla)] = 1;             /* estado: para as ações contínuas */

    if (estadoJogo == ESTADO_MENU_PRINCIPAL) {
        if (tecla == 27 || tolower(tecla) == 'q') exit(0);
        if (tecla == 9) {
            botaoSelecionado = (botaoSelecionado + 1) % 2;
        } else if (tecla == 13 || tecla == ' ') {
            if (botaoSelecionado == 0) fechaMenu();
            else exit(0);
        }
        glutPostRedisplay();
        return;
    }

    if (estadoJogo == ESTADO_PAUSADO) {
        if (tecla == 27) fechaMenu();
        else if (tolower(tecla) == 'q') exit(0);
        else if (tecla == 9) botaoSelecionado = (botaoSelecionado + 1) % 3;
        else if (tecla == 13 || tecla == ' ') {
            if (botaoSelecionado == 0) fechaMenu();
            else if (botaoSelecionado == 1) estadoJogo = ESTADO_INSTRUCOES;
            else exit(0);
        }
        glutPostRedisplay();
        return;
    }

    if (estadoJogo == ESTADO_INSTRUCOES) {
        if (tecla == 27 || tecla == 13 || tecla == ' ') {
            estadoJogo = ESTADO_PAUSADO;
            botaoSelecionado = 0;
        } else if (tolower(tecla) == 'q') {
            exit(0);
        }
        glutPostRedisplay();
        return;
    }

    if (estadoJogo == ESTADO_INVENTARIO) {
        if (tecla == 27 || tolower(tecla) == 'e') {
            estadoJogo = ESTADO_JOGANDO;
            capturaCursor();
        } else if (tolower(tecla) == 'c') {
            limpaCrafting();
            estadoJogo = ESTADO_CRAFTING;
        }
        glutPostRedisplay();
        return;
    }

    if (estadoJogo == ESTADO_CRAFTING) {
        if (tecla == 27 || tolower(tecla) == 'c') {
            estadoJogo = ESTADO_INVENTARIO;
        } else if (tolower(tecla) == 'e') {
            estadoJogo = ESTADO_JOGANDO;
            capturaCursor();
        }
        glutPostRedisplay();
        return;
    }

    switch (tolower(tecla)) {               /* evento: para as ações discretas */
        case 27:  abreMenu(); break;        /* ESC: menu de pausa */
        case 'e':
            estadoJogo = ESTADO_INVENTARIO;
            liberaCursor();
            break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            itemSelecionado = tolower(tecla) - '1';
            break;
        case '0':
            itemSelecionado = NUM_ITENS - 1;
            break;
        case 'f':
            consomeAlimento();
            break;
        case 'g':
            iniciaAnimacaoBraco(1);
            atacaInimigo();
            break;
        case 't':
            alternaChuva();
            break;
        case 9:   alternaCaptura(); break;  /* 9 é o código da tecla Tab */
        case 'p':
            projecaoOrtogonal = !projecaoOrtogonal;
            aplicaProjecao();
            break;
        case 'v':
            modoCamera = (modoCamera == PRIMEIRA_PESSOA) ? TERCEIRA_PESSOA
                                                         : PRIMEIRA_PESSOA;
            break;
        case '+': case '=': mudaZoom(+1); break;    /* '=' é o + sem Shift */
        case '-': case '_': mudaZoom(-1); break;
    }
}

void tecladoSolto(unsigned char tecla, int x, int y)
{
    (void)x; (void)y;
    teclas[tolower(tecla)] = 0;
}

void especial(int tecla, int x, int y)
{
    (void)x; (void)y;

    if (tecla >= 0 && tecla < 256) especiais[tecla] = 1;

    if (tecla == GLUT_KEY_F5 && estadoJogo == ESTADO_JOGANDO)
        modoCamera = (modoCamera == PRIMEIRA_PESSOA) ? TERCEIRA_PESSOA
                                                     : PRIMEIRA_PESSOA;
    if (tecla == GLUT_KEY_F6 && estadoJogo == ESTADO_JOGANDO)
        printf("Jogo salvo: %s\n", salvaJogo("savegame.dat") ? "ok" : "erro");
    if (tecla == GLUT_KEY_F7 && estadoJogo == ESTADO_JOGANDO)
        printf("Jogo carregado: %s\n", carregaJogo("savegame.dat") ? "ok" : "erro");
}

void especialSolto(int tecla, int x, int y)
{
    (void)x; (void)y;
    if (tecla >= 0 && tecla < 256) especiais[tecla] = 0;
}

/* ------------------------------------------------------------------ */
/* Mouse: o olhar gira pelo deslocamento em relação ao centro da      */
/* janela. Em coordenadas de tela y cresce para baixo, por isso -dy   */
/* no pitch. Com o cursor capturado, ele volta ao centro a cada       */
/* movimento (glutWarpPointer); o evento desse reposicionamento chega */
/* com deslocamento zero e é ignorado. Registrada com e sem botão     */
/* pressionado.                                                       */
/*                                                                    */
/* No macOS o cursor não é reposicionado: depois de cada              */
/* glutWarpPointer o sistema ignora o mouse por cerca de 0,25 s, e a  */
/* câmera andaria aos trancos. Lá, capturaCursor() desacopla o cursor */
/* do mouse (CGAssociateMouseAndMouseCursorPosition): ele fica parado */
/* e oculto, e o deslocamento de cada movimento é lido direto do      */
/* sistema (CGGetLastMouseDelta).                                     */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* O pitch fica entre -LIMITE_PITCH e LIMITE_PITCH (89 graus). Em 90, */
/* a direção do olhar fica paralela ao "cima" do gluLookAt, o produto */
/* vetorial que monta os eixos da câmera se anula e a imagem inverte. */
/* ------------------------------------------------------------------ */

int cursorCapturado = 0;    /* 1 enquanto o mouse controla o olhar */
int recentrar = 0;          /* 1: o próximo evento só leva o cursor ao centro */

void movimentoMouse(int x, int y)
{
    int cx = larguraJanela / 2, cy = alturaJanela / 2;
    int dx = x - cx, dy = y - cy;

    if (estadoJogo != ESTADO_JOGANDO) {
        movimentoInterface(x, y);
        return;
    }

    if (!cursorCapturado) return;

    if (recentrar) {                        /* cursor fora do centro: só */
        recentrar = 0;                      /* reposiciona, sem girar    */
#ifndef __APPLE__
        glutWarpPointer(cx, cy);
#endif
        return;
    }

#ifdef __APPLE__
    {
        int32_t mx, my;                     /* deslocamento deste movimento */
        CGGetLastMouseDelta(&mx, &my);
        dx = mx;
        dy = my;
    }
#endif

    if (dx == 0 && dy == 0) return;         /* eco do glutWarpPointer */

    yaw   += dx * SENSIBILIDADE;
    pitch -= dy * SENSIBILIDADE;            /* y de tela cresce para baixo */

    if (pitch >  LIMITE_PITCH) pitch =  LIMITE_PITCH;
    if (pitch < -LIMITE_PITCH) pitch = -LIMITE_PITCH;

#ifndef __APPLE__
    glutWarpPointer(cx, cy);                /* devolve o cursor ao centro */
#endif
}

/* Captura e liberação do cursor. O jogo começa com o cursor capturado;
   Tab alterna, e um clique na janela captura de novo. Sem uma forma de
   liberar, o usuário não conseguiria sair da janela.
   capturaCursor() não move o cursor na hora: só o oculta e pede que o
   próximo movimento o leve ao centro. Assim funciona também no início,
   quando a janela talvez ainda nem esteja na tela, e o cursor já está
   oculto quando é reposicionado (em algumas sessões Wayland o
   reposicionamento só funciona com o cursor oculto). */
void capturaCursor(void)
{
    cursorCapturado = 1;
    recentrar = 1;
    glutSetCursor(GLUT_CURSOR_NONE);
#ifdef __APPLE__
    CGAssociateMouseAndMouseCursorPosition(false);  /* cursor parado */
#endif
}

void liberaCursor(void)
{
    cursorCapturado = 0;
    glutSetCursor(GLUT_CURSOR_INHERIT);
#ifdef __APPLE__
    CGAssociateMouseAndMouseCursorPosition(true);   /* cursor volta a andar */
#endif
}

void alternaCaptura(void)
{
    if (cursorCapturado) liberaCursor();
    else                 capturaCursor();
}

/* Clique com o cursor liberado: captura de novo */
void cliqueMouse(int botao, int estado, int x, int y)
{
    if (estado != GLUT_DOWN) return;

    if (estadoJogo == ESTADO_INVENTARIO) {
        if (botao == GLUT_LEFT_BUTTON || botao == GLUT_RIGHT_BUTTON)
            interageSlotInventario(slotInventarioEm(x, y), botao);
        glutPostRedisplay();
        return;
    }

    if (estadoJogo == ESTADO_CRAFTING) {
        if (botao == GLUT_LEFT_BUTTON || botao == GLUT_RIGHT_BUTTON)
            interageCrafting(x, y, botao);
        glutPostRedisplay();
        return;
    }

    if (estadoJogo == ESTADO_MENU_PRINCIPAL ||
        estadoJogo == ESTADO_PAUSADO ||
        estadoJogo == ESTADO_INSTRUCOES) {
        if (botao == GLUT_LEFT_BUTTON) {
            int i;

            for (i = 0; i < numeroBotoesTela(); i++) {
                if (pontoNoBotao(x, y, i)) {
                    botaoSelecionado = i;
                    acionaBotaoTela(i);
                    return;
                }
            }
        }
        return;
    }

    (void)x; (void)y;
    if (!cursorCapturado) {
        capturaCursor();
        return;
    }
    if (botao == GLUT_LEFT_BUTTON || botao == GLUT_RIGHT_BUTTON) {
        interageComBloco(botao);
        glutPostRedisplay();
    }
}

/* O cursor entrou na janela (por exemplo, depois de trocar de janela
   com Alt+Tab): ele chega pela borda, então o próximo movimento só o
   leva ao centro, sem girar o olhar */
void entradaJanela(int estado)
{
    if (estado == GLUT_ENTERED) recentrar = 1;
}

/* ------------------------------------------------------------------ */
/* Zoom: a roda e as teclas + e - mudam o campo de visão (trocam a    */
/* lente), sem mover a câmera. Passos de 5 graus, entre FOV_MIN e     */
/* FOV_MAX. glutMouseWheelFunc é extensão da freeglut, por isso       */
/* GL/freeglut.h.                                                     */
/* ------------------------------------------------------------------ */

/* +1 aproxima, -1 afasta; usada pela roda e pelas teclas + e - */
void mudaZoom(int direcao)
{
    campoDeVisao -= direcao * 5.0f;         /* fov menor = mais perto */
    if (campoDeVisao < FOV_MIN) campoDeVisao = FOV_MIN;
    if (campoDeVisao > FOV_MAX) campoDeVisao = FOV_MAX;

    aplicaProjecao();
}

void rodaMouse(int roda, int direcao, int x, int y)
{
    (void)roda; (void)x; (void)y;
    if (estadoJogo == ESTADO_JOGANDO)
        itemSelecionado = (itemSelecionado + (direcao > 0 ? 1 : NUM_ITENS - 1)) % NUM_ITENS;
    glutPostRedisplay();
}

/* ------------------------------------------------------------------ */
/* Atualização, a cada quadro (glutIdleFunc). dt é o tempo desde o    */
/* quadro anterior, limitado a DT_MAXIMO; posição += velocidade x dt  */
/* deixa a velocidade igual em qualquer taxa de quadros.              */
/*                                                                    */
/* A frente e a direita saem só do yaw, então o avanço fica no plano  */
/* do chão. A direção das teclas é normalizada (a diagonal não fica   */
/* mais rápida), R multiplica o passo e moveJogador() aplica a        */
/* colisão. No fim, a animação de caminhada avança com a distância    */
/* realmente andada.                                                  */
/* ------------------------------------------------------------------ */

int ultimoTempo = 0;    /* tempo da atualização anterior, em ms */

float faseCaminhada = 0.0f;      /* radianos; avança com a distância andada */
float amplitudeCaminhada = 0.0f; /* 0 parado, 1 andando; suaviza o balanço */

void atualiza(void)
{
    int agora = glutGet(GLUT_ELAPSED_TIME);
#if DEMO_UI_PAUSE_CLOCK
    /* No modo didático, o relógio fica congelado enquanto o jogo está
       pausado. Ao retornar, dt inclui toda a duração da pausa. */
    if (estadoJogo != ESTADO_JOGANDO) {
        glutPostRedisplay();
        return;
    }

    float dt = (agora - ultimoTempo) / 1000.0f;
    ultimoTempo = agora;
#else
    float dt = (agora - ultimoTempo) / 1000.0f;
    ultimoTempo = agora;
    if (dt > DT_MAXIMO) dt = DT_MAXIMO;

    if (estadoJogo != ESTADO_JOGANDO) {
        glutPostRedisplay();
        return;
    }
#endif

    if (tempoAnimacaoBraco > 0.0f) {
        tempoAnimacaoBraco -= dt;
        if (tempoAnimacaoBraco <= 0.0f) {
            tempoAnimacaoBraco = 0.0f;
            tipoAnimacaoBraco = 0;
        }
    }

    tempoAnimacaoCeu += dt;
    tempoMundo += dt / DURACAO_DIA;
    while (tempoMundo >= 1.0f) tempoMundo -= 1.0f;
    while (tempoMundo < 0.0f) tempoMundo += 1.0f;
    audioAtualizaNoite(tempoMundo, DURACAO_DIA);

    float passo = VELOCIDADE * dt;
    float giro  = VELOCIDADE_GIRO * dt;
    if (teclas['r'] && fomeJogador >= FOME_MINIMA_CORRIDA)
        passo *= FATOR_CORRIDA;                 /* correr */

    if (especiais[GLUT_KEY_LEFT])  yaw -= giro;
    if (especiais[GLUT_KEY_RIGHT]) yaw += giro;

    float rad = yaw * PI / 180.0f;
    float frenteX  =  sinf(rad), frenteZ  = -cosf(rad);
    float direitaX =  cosf(rad), direitaZ =  sinf(rad);

    /* direção desejada no chão: cada tecla vale 1 (pressionada) ou 0 */
    float frente = (float)(teclas['w'] - teclas['s']);
    float lado   = (float)(teclas['d'] - teclas['a']);
    float mx = frenteX * frente + direitaX * lado;
    float mz = frenteZ * frente + direitaZ * lado;

    float comprimento = sqrtf(mx * mx + mz * mz);
    if (comprimento > 0.0f) {               /* normaliza: na diagonal o */
        mx /= comprimento;                  /* comprimento seria 1.41   */
        mz /= comprimento;
    }

    /* A fome diminui lentamente e acelera durante o deslocamento/corrida.
       Em zero, o jogador sofre dano periódico até se alimentar ou morrer. */
    {
        float multiplicadorFome = comprimento > 0.0f ? 1.5f : 1.0f;
        if (teclas['r'] && comprimento > 0.0f &&
            fomeJogador >= FOME_MINIMA_CORRIDA)
            multiplicadorFome = 3.0f;
        {
            float gasto = FOME_DRENO * multiplicadorFome * dt;
            float usadoSaturacao = gasto < saturacaoJogador
                                  ? gasto : saturacaoJogador;
            saturacaoJogador -= usadoSaturacao;
            fomeJogador -= gasto - usadoSaturacao;
        }
        if (fomeJogador < 0.0f) fomeJogador = 0.0f;
        if (saturacaoJogador < 0.0f) saturacaoJogador = 0.0f;
        if (saturacaoJogador > fomeJogador) saturacaoJogador = fomeJogador;
        if (fomeJogador <= 0.0f) {
            tempoDanoFome += dt;
            if (tempoDanoFome >= 1.0f) {
                vidaJogador -= 1.0f;
                audioDano();
                tempoDanoFome = 0.0f;
            }
        } else {
            tempoDanoFome = 0.0f;
        }

        if (fomeJogador >= 18.0f && vidaJogador < VIDA_MAXIMA) {
            tempoRegeneracaoJogador += dt;
            if (tempoRegeneracaoJogador >= 4.0f) {
                vidaJogador += 1.0f;
                if (vidaJogador > VIDA_MAXIMA) vidaJogador = VIDA_MAXIMA;
                if (saturacaoJogador >= 1.0f) saturacaoJogador -= 1.0f;
                else if (fomeJogador > 0.0f) fomeJogador -= 1.0f;
                tempoRegeneracaoJogador = 0.0f;
            }
        } else {
            tempoRegeneracaoJogador = 0.0f;
        }
    }

    /* A movimentação vertical é física: Espaço ou seta para cima inicia
       um salto quando o jogador está apoiado. A seta para baixo e Shift
       deixam de mover o jogador através dos blocos. */
    if ((teclas[' '] || especiais[GLUT_KEY_UP]) && jogadorNoChao())
        velocidadeVertical = VELOCIDADE_SALTO;

    velocidadeVertical -= GRAVIDADE * dt;
    if (velocidadeVertical < -VELOCIDADE_QUEDA)
        velocidadeVertical = -VELOCIDADE_QUEDA;

    int estavaNoChao = jogadorNoChao();
    float antesX = jogadorX, antesY = jogadorY, antesZ = jogadorZ;
    moveJogador(mx * passo, velocidadeVertical * dt, mz * passo);
    atualizaChunks(jogadorX, jogadorZ, DISTANCIA_CARGA_CHUNK);

    /* Se a colisão bloqueou o deslocamento vertical, zerar a velocidade
       evita acumular queda contra o chão ou impulso contra o teto. */
    if (jogadorY == antesY)
        velocidadeVertical = 0.0f;

    if (!estavaNoChao && jogadorY < antesY)
        distanciaQueda += antesY - jogadorY;
    if (!estavaNoChao && jogadorNoChao()) {
        if (distanciaQueda > ALTURA_QUEDA_SEGURA) {
            vidaJogador -= (distanciaQueda - ALTURA_QUEDA_SEGURA) * 2.0f;
            audioDano();
        }
        distanciaQueda = 0.0f;
    }
    if (vidaJogador <= 0.0f)
        reapareceJogador();

    /* animação: quanto o personagem realmente andou no chão (a colisão
       pode ter recusado parte do movimento) */
    float andou = sqrtf((jogadorX - antesX) * (jogadorX - antesX) +
                        (jogadorZ - antesZ) * (jogadorZ - antesZ));
    audioAtualizaPassos(dt, andou > 0.0001f && estavaNoChao);
    {
        int px = (int)floorf(jogadorX);
        int py = (int)floorf(jogadorY + 0.15f);
        int pz = (int)floorf(jogadorZ);
        int dentroDaAgua = blocoNoMundo(px, py, pz) == AGUA ||
                           blocoNoMundo(px, py + 1, pz) == AGUA ||
                           blocoNoMundo(px, py + 2, pz) == AGUA;
        audioAtualizaAgua(dentroDaAgua);
    }
    faseCaminhada += andou * FASE_POR_BLOCO;
    if (andou > 0.0f) amplitudeCaminhada += VELOCIDADE_AMPLITUDE * dt;
    else              amplitudeCaminhada -= VELOCIDADE_AMPLITUDE * dt;
    if (amplitudeCaminhada > 1.0f) amplitudeCaminhada = 1.0f;
    if (amplitudeCaminhada < 0.0f) amplitudeCaminhada = 0.0f;

    atualizaDrops(dt);
    atualizaEntidades(dt);
    atualizaEfeitos(dt);
    if (vidaJogador <= 0.0f)
        reapareceJogador();

    glutPostRedisplay();
}


/* ================================================================== */
/* MENU                                                               */
/*                                                                    */
/* Menu de pausa, como o do Minecraft: ESC abre e fecha, o jogo para de */
/* se mover, o cursor fica livre e a tela mostra as instruções de     */
/* movimentação por cima da cena escurecida. Com o menu aberto, Q sai do */
/* jogo.                                                              */
/* ================================================================== */


EstadoJogo estadoJogo = ESTADO_MENU_PRINCIPAL;
int botaoSelecionado = 0;
int fpsExibido = 0;

void abreMenu(void)
{
    if (estadoJogo == ESTADO_JOGANDO) {
        estadoJogo = ESTADO_PAUSADO;
        botaoSelecionado = 0;
        liberaCursor();
    }
}

void fechaMenu(void)
{
    EstadoJogo estadoAnterior = estadoJogo;
    estadoJogo = ESTADO_JOGANDO;
    capturaCursor();
#if DEMO_UI_PAUSE_CLOCK
    /* O menu principal não deve gerar um arremesso ao iniciar. O defeito
       demonstrado vale somente para o retorno de uma pausa. */
    if (estadoAnterior != ESTADO_PAUSADO)
        ultimoTempo = glutGet(GLUT_ELAPSED_TIME);
#else
    (void)estadoAnterior;
    ultimoTempo = glutGet(GLUT_ELAPSED_TIME);
#endif
}

void alternaMenu(void)
{
    if (estadoJogo == ESTADO_JOGANDO) abreMenu();
    else if (estadoJogo == ESTADO_PAUSADO) fechaMenu();
}

/* ------------------------------------------------------------------ */
/* Texto                                                              */
/* As fontes da GLUT (glutBitmapCharacter) usam a codificação         */
/* Latin-1: um byte por letra, e as letras acentuadas do português    */
/* têm códigos de 192 a 255. Este arquivo, como o resto do código,    */
/* está em UTF-8, em que essas mesmas letras ocupam DOIS bytes (por   */
/* exemplo, "ç" é 0xC3 0xA7). Enviados direto para a GLUT, os dois    */
/* bytes virariam dois símbolos estranhos.                            */
/* proximoCaractere() lê um caractere UTF-8 e devolve o código        */
/* Latin-1 dele. Nos dois bytes 110xxxxx 10yyyyyy, os bits xxxxx e    */
/* yyyyyy juntos formam o código: (b1 & 0x1F) << 6 | (b2 & 0x3F).     */
/* Caracteres fora do Latin-1 (como as setas) viram '?', por isso o   */
/* menu escreve "seta para cima" em vez do símbolo.                   */
/* ------------------------------------------------------------------ */

#define FONTE       GLUT_BITMAP_HELVETICA_18
#define FONTE_TITULO GLUT_BITMAP_TIMES_ROMAN_24

static int proximoCaractere(const char **texto)
{
    unsigned char b1 = (unsigned char)**texto;
    (*texto)++;

    if (b1 < 0x80) return b1;                       /* ASCII: 1 byte */

    if ((b1 & 0xE0) == 0xC0 && **texto) {           /* 2 bytes */
        unsigned char b2 = (unsigned char)**texto;
        int codigo = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
        (*texto)++;
        return codigo <= 0xFF ? codigo : '?';
    }

    while ((**texto & 0xC0) == 0x80) (*texto)++;    /* pula o resto */
    return '?';
}

/* largura do texto na tela, em pixels */
static int larguraTexto(void *fonte, const char *texto)
{
    int largura = 0;
    while (*texto)
        largura += glutBitmapWidth(fonte, proximoCaractere(&texto));
    return largura;
}

/* escreve com o início da linha de base em (x, y), em pixels */
static void escreve(void *fonte, float x, float y, const char *texto)
{
    glRasterPos2f(x, y);
    while (*texto)
        glutBitmapCharacter(fonte, proximoCaractere(&texto));
}

/* um retângulo preenchido, em pixels */
static void retangulo(float x0, float y0, float x1, float y1)
{
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x1, y0);
    glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
}

/* ------------------------------------------------------------------ */
/* Interface 2D: esta e a unica passada que usa coordenadas em pixels. */
/* As duas matrizes sao guardadas para a camera 3D nao deslocar a UI.  */
/* ------------------------------------------------------------------ */

void iniciaInterface(void)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    /* Mantem uma unidade em x/y igual a um pixel, mas deixa profundidade
       suficiente para os mini-cubos da barra usarem o Z-buffer. */
    glOrtho(0, larguraJanela, 0, alturaJanela, -200.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
#if DEMO_UI_MATRIX_ONLY
    /* Defeito intencional para a demonstração: a matriz de modelo e visão
       continua contendo o gluLookAt da câmera. */
#else
    glPushMatrix();
    glLoadIdentity();
#endif

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
}

void terminaInterface(void)
{
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    /* Os elementos da UI usam texturas próprias. A cena 3D do próximo
       quadro deve voltar a encontrar o atlas ligado ao estado corrente. */
    glBindTexture(GL_TEXTURE_2D, atlas);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
#if DEMO_UI_MATRIX_ONLY
    /* A matriz MODELVIEW não foi empilhada neste modo didático. */
#else
    glPopMatrix();
#endif
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void desenhaTextoCentralizado(void *fonte, float x, float y,
                                     const char *texto)
{
    escreve(fonte, x - larguraTexto(fonte, texto) / 2.0f, y, texto);
}

static void desenhaImagemUI(const TexturaUI *imagem,
                            float x, float y, float largura, float altura)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, imagem->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    /* O PPM é lido de cima para baixo; as coordenadas verticais são
       invertidas para a imagem aparecer na orientação original. */
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + largura, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + largura, y + altura);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + altura);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

static float larguraLimitada(float larguraOriginal, float larguraMaxima,
                             float margem)
{
    float disponivel = larguraJanela - 2.0f * margem;
    if (disponivel < 1.0f) disponivel = 1.0f;
    return larguraOriginal < larguraMaxima
        ? (larguraOriginal < disponivel ? larguraOriginal : disponivel)
        : (larguraMaxima < disponivel ? larguraMaxima : disponivel);
}

static void desenhaIdentidadeMenuPrincipal(void)
{
    float larguraLogo = larguraLimitada((float)texturaLogo.largura,
                                         760.0f, 40.0f);
    float alturaLogo = larguraLogo * texturaLogo.altura / texturaLogo.largura;
    float xLogo = (larguraJanela - larguraLogo) / 2.0f;
    float yLogo = alturaJanela - 28.0f - alturaLogo;

    float larguraSubtitulo = larguraLimitada((float)texturaSubtitulo.largura,
                                              380.0f, 40.0f);
    float alturaSubtitulo = larguraSubtitulo * texturaSubtitulo.altura /
                            texturaSubtitulo.largura;
    float xSubtitulo = (larguraJanela - larguraSubtitulo) / 2.0f;
    float ySubtitulo = yLogo - alturaSubtitulo + 8.0f;

    float larguraVersao = larguraLimitada((float)texturaVersao.largura,
                                           150.0f, 20.0f);
    float alturaVersao = larguraVersao * texturaVersao.altura /
                         texturaVersao.largura;

    desenhaImagemUI(&texturaLogo, xLogo, yLogo, larguraLogo, alturaLogo);
    desenhaImagemUI(&texturaSubtitulo, xSubtitulo, ySubtitulo,
                    larguraSubtitulo, alturaSubtitulo);
    desenhaImagemUI(&texturaVersao, 20.0f, 20.0f,
                    larguraVersao, alturaVersao);
}

static int numeroBotoesTela(void)
{
    if (estadoJogo == ESTADO_MENU_PRINCIPAL) return 2;
    if (estadoJogo == ESTADO_PAUSADO) return 3;
    return 0; /* instrucoes: retorno pelo teclado */
}

static void acionaBotaoTela(int indice)
{
    if (estadoJogo == ESTADO_MENU_PRINCIPAL) {
        if (indice == 0) fechaMenu();
        else exit(0);
    } else if (estadoJogo == ESTADO_PAUSADO) {
        if (indice == 0) fechaMenu();
        else if (indice == 1) {
            estadoJogo = ESTADO_INSTRUCOES;
            botaoSelecionado = 0;
        } else exit(0);
    } else if (estadoJogo == ESTADO_INSTRUCOES) {
        estadoJogo = ESTADO_PAUSADO;
        botaoSelecionado = 0;
    }
}

static void limitesBotao(int indice, float *x0, float *y0,
                         float *x1, float *y1)
{
    float centroY;

    if (numeroBotoesTela() == 3)
        centroY = alturaJanela * (indice == 0 ? 0.42f :
                                  indice == 1 ? 0.29f : 0.16f);
    else if (numeroBotoesTela() == 1)
        centroY = alturaJanela * 0.16f;
    else
        centroY = alturaJanela * (indice == 0 ? 0.38f : 0.25f);

    *x0 = (larguraJanela - 300.0f) / 2.0f;
    *x1 = *x0 + 300.0f;
    *y0 = centroY - 36.0f;
    *y1 = centroY + 36.0f;
}

static int pontoNoBotao(int x, int y, int indice)
{
    float x0, y0, x1, y1;
    limitesBotao(indice, &x0, &y0, &x1, &y1);
#if !DEMO_UI_RAW_MOUSE_Y
    y = alturaJanela - y;
#endif
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

static void desenhaBotao(int indice, const char *texto)
{
    float x0, y0, x1, y1;
    limitesBotao(indice, &x0, &y0, &x1, &y1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(indice == botaoSelecionado ? 0.10f : 0.06f,
              indice == botaoSelecionado ? 0.65f : 0.18f,
              indice == botaoSelecionado ? 0.95f : 0.25f, 0.95f);
    retangulo(x0, y0, x1, y1);
    glDisable(GL_BLEND);

    glColor3f(1.0f, 1.0f, 1.0f);
    desenhaTextoCentralizado(FONTE_TITULO, (x0 + x1) / 2.0f, y0 + 23.0f, texto);
}

static void desenhaMira(void)
{
    float x = larguraJanela / 2.0f, y = alturaJanela / 2.0f;
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(x - 9.0f, y); glVertex2f(x + 9.0f, y);
    glVertex2f(x, y - 9.0f); glVertex2f(x, y + 9.0f);
    glEnd();
}

static void desenhaIconeMaca(float x, float y, float lado)
{
    float s = lado / 58.0f;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    /* Silhueta escura em degraus, para manter o aspecto pixel art. */
    glColor3f(0.18f, 0.035f, 0.025f);
    retangulo(x + 18.0f*s, y + 10.0f*s, x + 40.0f*s, y + 47.0f*s);
    retangulo(x + 13.0f*s, y + 17.0f*s, x + 45.0f*s, y + 39.0f*s);
    retangulo(x + 22.0f*s, y + 8.0f*s,  x + 36.0f*s, y + 49.0f*s);

    /* Corpo vermelho com duas saliências no topo. */
    glColor3f(0.72f, 0.025f, 0.035f);
    retangulo(x + 19.0f*s, y + 14.0f*s, x + 39.0f*s, y + 44.0f*s);
    retangulo(x + 15.0f*s, y + 21.0f*s, x + 43.0f*s, y + 37.0f*s);
    retangulo(x + 22.0f*s, y + 12.0f*s, x + 36.0f*s, y + 45.0f*s);

    glColor3f(0.95f, 0.045f, 0.07f);
    retangulo(x + 19.0f*s, y + 22.0f*s, x + 37.0f*s, y + 39.0f*s);
    retangulo(x + 23.0f*s, y + 16.0f*s, x + 34.0f*s, y + 42.0f*s);

    /* Brilho e sombra lateral. */
    glColor3f(1.0f, 0.28f, 0.30f);
    retangulo(x + 19.0f*s, y + 29.0f*s, x + 24.0f*s, y + 37.0f*s);
    retangulo(x + 24.0f*s, y + 36.0f*s, x + 30.0f*s, y + 40.0f*s);
    glColor3f(0.48f, 0.015f, 0.025f);
    retangulo(x + 37.0f*s, y + 18.0f*s, x + 43.0f*s, y + 34.0f*s);
    retangulo(x + 22.0f*s, y + 10.0f*s, x + 35.0f*s, y + 14.0f*s);

    /* Cabinho e folha. */
    glColor3f(0.28f, 0.10f, 0.035f);
    retangulo(x + 30.0f*s, y + 43.0f*s, x + 35.0f*s, y + 52.0f*s);
    glColor3f(0.16f, 0.38f, 0.08f);
    retangulo(x + 34.0f*s, y + 48.0f*s, x + 43.0f*s, y + 52.0f*s);
    retangulo(x + 39.0f*s, y + 50.0f*s, x + 45.0f*s, y + 54.0f*s);
}

static void desenhaIconeFerramenta(int tipo, float x, float y, float lado)
{
    float s = lado / 58.0f;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    if (tipo == ESPADA) {
        float altura = lado * 306.0f / 335.0f;
        desenhaImagemUI(&texturaEspada, x + lado * 0.05f,
                        y + lado * 0.07f, lado * 0.90f, altura * 0.90f);
        return;
    }

    if (tipo == PICARETA) {
        float altura = lado * 335.0f / 335.0f;
        desenhaImagemUI(&texturaPicareta, x, y, lado, altura);
        return;
    }

    if (tipo == PICARETA) {
        /* Silhueta pixelada fiel à referência: cabo diagonal, barra
           metálica superior e gancho descendo no lado direito. */
        glColor3f(0.06f, 0.05f, 0.045f);
        glBegin(GL_POLYGON);
        glVertex2f(x+8*s,y+7*s);  glVertex2f(x+16*s,y+7*s);
        glVertex2f(x+41*s,y+36*s); glVertex2f(x+41*s,y+43*s);
        glVertex2f(x+34*s,y+43*s); glVertex2f(x+8*s,y+15*s);
        glEnd();
        glColor3f(0.43f, 0.26f, 0.10f);
        glBegin(GL_POLYGON);
        glVertex2f(x+11*s,y+9*s);  glVertex2f(x+14*s,y+9*s);
        glVertex2f(x+38*s,y+37*s); glVertex2f(x+35*s,y+40*s);
        glVertex2f(x+11*s,y+13*s); glEnd();

        glColor3f(0.06f, 0.06f, 0.065f);
        glBegin(GL_POLYGON);
        glVertex2f(x+22*s,y+36*s); glVertex2f(x+28*s,y+36*s);
        glVertex2f(x+28*s,y+40*s); glVertex2f(x+44*s,y+40*s);
        glVertex2f(x+44*s,y+31*s); glVertex2f(x+50*s,y+31*s);
        glVertex2f(x+50*s,y+45*s); glVertex2f(x+45*s,y+45*s);
        glVertex2f(x+45*s,y+49*s); glVertex2f(x+25*s,y+49*s);
        glVertex2f(x+25*s,y+44*s); glVertex2f(x+22*s,y+44*s);
        glEnd();
        glColor3f(0.72f, 0.73f, 0.74f);
        glBegin(GL_POLYGON);
        glVertex2f(x+25*s,y+39*s); glVertex2f(x+29*s,y+39*s);
        glVertex2f(x+29*s,y+43*s); glVertex2f(x+42*s,y+43*s);
        glVertex2f(x+42*s,y+34*s); glVertex2f(x+46*s,y+34*s);
        glVertex2f(x+46*s,y+42*s); glVertex2f(x+42*s,y+42*s);
        glVertex2f(x+42*s,y+46*s); glVertex2f(x+28*s,y+46*s);
        glVertex2f(x+28*s,y+42*s); glVertex2f(x+25*s,y+42*s);
        glEnd();
        glColor3f(0.96f, 0.97f, 0.97f);
        retangulo(x+28*s,y+40*s,x+35*s,y+44*s);
    } else {
        /* Espada: lâmina longa clara, guarda escura e cabo separado. */
        glColor3f(0.07f, 0.07f, 0.08f);
        glBegin(GL_POLYGON);
        glVertex2f(x+24*s,y+24*s); glVertex2f(x+30*s,y+24*s);
        glVertex2f(x+50*s,y+47*s); glVertex2f(x+45*s,y+52*s);
        glVertex2f(x+20*s,y+29*s); glEnd();
        glColor3f(0.75f, 0.76f, 0.78f);
        glBegin(GL_POLYGON);
        glVertex2f(x+28*s,y+27*s); glVertex2f(x+31*s,y+27*s);
        glVertex2f(x+46*s,y+46*s); glVertex2f(x+43*s,y+49*s);
        glVertex2f(x+24*s,y+29*s); glEnd();
        glColor3f(0.96f, 0.97f, 0.98f);
        retangulo(x+30*s,y+30*s,x+34*s,y+35*s);
        retangulo(x+35*s,y+35*s,x+39*s,y+40*s);
        retangulo(x+40*s,y+40*s,x+44*s,y+45*s);

        glColor3f(0.09f, 0.09f, 0.10f);
        retangulo(x+16*s,y+20*s,x+31*s,y+26*s);
        retangulo(x+21*s,y+17*s,x+27*s,y+29*s);
        glColor3f(0.38f, 0.22f, 0.07f);
        retangulo(x+10*s,y+8*s,x+17*s,y+15*s);
        retangulo(x+15*s,y+13*s,x+22*s,y+20*s);
        glColor3f(0.17f, 0.12f, 0.07f);
        retangulo(x+8*s,y+6*s,x+14*s,y+12*s);
    }
}

/* Desenha um bloco real, reduzido e girado, dentro de um slot 2D. O
   centro e a posicao continuam em pixels; apenas a aparencia do item
   usa a geometria 3D ja existente. */
static void desenhaIconeBloco3D(int tipo, float x, float y, float lado)
{
    float tamanho = lado * 0.429f; /* aumento de 10% sobre a escala atual */

    if (tipo == MACA) {
        desenhaIconeMaca(x, y, lado);
        return;
    }

    if (tipo == PICARETA || tipo == ESPADA) {
        desenhaIconeFerramenta(tipo, x, y, lado);
        return;
    }

    glPushMatrix();
    glTranslatef(x + lado / 2.0f, y + lado / 2.0f, 0.0f);
    glRotatef(28.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-42.0f, 0.0f, 1.0f, 0.0f);
    glScalef(tamanho, tamanho, tamanho);
    glTranslatef(-0.5f, -0.5f, -0.5f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (tipo == TOCHA)
        desenhaTocha(1.0f);
    else
        desenhaBloco(tipo, 1.0f, TODAS_FACES);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPopMatrix();
}

static const char *nomeDoItem(int tipo)
{
    switch (tipo) {
        case GRAMA:         return "GRAMA";
        case TERRA:         return "TERRA";
        case PEDRA:         return "PEDRA";
        case TRONCO:        return "TRONCO";
        case FOLHA:         return "FOLHAS";
        case AREIA:         return "AREIA";
        case TOCHA:         return "TOCHA";
        case NEVE:          return "NEVE";
        case VIDRO:         return "VIDRO";
        case MACA:          return "MACA";
        case PICARETA:      return "PICARETA";
        case ESPADA:        return "ESPADA";
        case CRISTAL:       return "CRISTAL";
        case MINERIO_CARVAO:return "MINERIO DE CARVAO";
        case MINERIO_FERRO: return "MINERIO DE FERRO";
        default:            return "ITEM";
    }
}

static void desenhaCoracoesVida(void)
{
    const float larguraCoracao = 24.0f;
    const float alturaCoracao = 26.0f;
    const float espacamento = 1.0f;
    const float larguraBarra = NUM_ITENS * 58.0f +
                               (NUM_ITENS - 1) * 6.0f + 24.0f;
#if DEMO_UI_FIXED_BAR
    const float barraX0 = DEMO_UI_FIXED_BAR_X;
#else
    const float barraX0 = (larguraJanela - larguraBarra) / 2.0f;
#endif
    /* Alinha a fileira ao início dos slots, em vez de centralizá-la. */
    const float x0 = barraX0 + 14.0f;
    /* Mesmo tamanho e mesma altura dos ícones de fome. */
    const float y0 = 18.0f + 58.0f + 16.0f;
    float vida = vidaJogador;
    int i;

    if (vida < 0.0f) vida = 0.0f;
    if (vida > VIDA_MAXIMA) vida = VIDA_MAXIMA;

    for (i = 0; i < 10; i++) {
        float restante = vida - i * 2.0f;
        int estado = restante >= 2.0f ? 2 : (restante >= 1.0f ? 1 : 0);
        const TexturaUI *icone = estado == 2 ? &texturaCoracaoCheio :
                                 (estado == 1 ? &texturaCoracaoMeio :
                                                 &texturaCoracaoVazio);
        desenhaImagemUI(icone,
                        x0 + i * (larguraCoracao + espacamento), y0,
                        larguraCoracao, alturaCoracao);
    }
}

static void desenhaIconesFome(void)
{
    const float larguraIcone = 24.0f;
    const float alturaIcone = 26.0f;
    const float espacamento = 1.0f;
    const float larguraBarra = NUM_ITENS * 58.0f +
                               (NUM_ITENS - 1) * 6.0f + 24.0f;
    const float larguraTotal = 10.0f * larguraIcone + 9.0f * espacamento;
#if DEMO_UI_FIXED_BAR
    const float barraX0 = DEMO_UI_FIXED_BAR_X;
#else
    const float barraX0 = (larguraJanela - larguraBarra) / 2.0f;
#endif
    const float x0 = barraX0 + larguraBarra - 14.0f - larguraTotal;
    const float y0 = 18.0f + 58.0f + 16.0f;
    float fome = fomeJogador;
    int i;

    if (fome < 0.0f) fome = 0.0f;
    if (fome > FOME_MAXIMA) fome = FOME_MAXIMA;

    for (i = 0; i < 10; i++) {
        float restante = fome - i * 2.0f;
        int estado = restante >= 2.0f ? 2 : (restante >= 1.0f ? 1 : 0);
        const TexturaUI *icone = estado == 2 ? &texturaFomeCheia :
                                 (estado == 1 ? &texturaFomeMeia :
                                                 &texturaFomeVazia);
        desenhaImagemUI(icone, x0 + i * (larguraIcone + espacamento), y0,
                        larguraIcone, alturaIcone);
    }
}

static void desenhaBarraItens(void)
{
    float lado = 58.0f, espaco = 6.0f;
    float largura = NUM_ITENS * lado + (NUM_ITENS - 1) * espaco + 24.0f;
#if DEMO_UI_FIXED_BAR
    /* Defeito intencional: a barra fica presa à posição da janela inicial. */
    float x0 = DEMO_UI_FIXED_BAR_X;
#else
    float x0 = (larguraJanela - largura) / 2.0f;
#endif
    float y0 = 18.0f;
    int i;

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f, 0.02f, 0.03f, 0.80f);
    retangulo(x0, y0, x0 + largura, y0 + lado + 24.0f);
    glDisable(GL_BLEND);

    /* Limpa somente a profundidade para os cubos da interface nao serem
       escondidos pela cena 3D ja desenhada. */
    glClear(GL_DEPTH_BUFFER_BIT);

    for (i = 0; i < NUM_ITENS; i++) {
        float x = x0 + 12.0f + i * (lado + espaco);
        glColor3f(i == itemSelecionado ? 1.0f : 0.35f,
                  i == itemSelecionado ? 0.85f : 0.35f,
                  i == itemSelecionado ? 0.20f : 0.35f);
        retangulo(x - 2.0f, y0 + 10.0f, x + lado + 2.0f, y0 + lado + 14.0f);
        desenhaIconeBloco3D(tiposItens[i], x, y0 + 12.0f, lado);
        glDisable(GL_TEXTURE_2D);
        { char quantidade[16];
          sprintf(quantidade, "%d", quantidadesItens[i]);
          float textoX = x + lado - larguraTexto(FONTE, quantidade) - 3.0f;
          float textoY = y0 + 15.0f;
          /* sombra preta deslocada para destacar o numero */
          glColor3f(0.0f, 0.0f, 0.0f);
          escreve(FONTE, textoX + 1.0f, textoY - 1.0f, quantidade);
          glColor3f(1.0f, 1.0f, 1.0f);
          escreve(FONTE, textoX, textoY, quantidade);
          if (i == itemSelecionado &&
              (tiposItens[i] == PICARETA || tiposItens[i] == ESPADA)) {
              float durabilidade = durabilidadeDoItem(tiposItens[i]) / 100.0f;
              float larguraBarra = lado - 8.0f;
              /* A durabilidade fica dentro do slot, abaixo do icone. */
              glColor3f(0.02f, 0.02f, 0.02f);
              retangulo(x + 4.0f, y0 + 13.0f,
                        x + 4.0f + larguraBarra, y0 + 17.0f);
              if (durabilidade < 0.0f) durabilidade = 0.0f;
              if (durabilidade > 1.0f) durabilidade = 1.0f;
              glColor3f(durabilidade > 0.25f ? 0.25f : 0.90f,
                        durabilidade > 0.25f ? 0.85f : 0.20f,
                        0.18f);
              retangulo(x + 4.0f, y0 + 13.0f,
                        x + 4.0f + larguraBarra * durabilidade,
                        y0 + 17.0f);
          }
        }
    }

    /* Identifica o item atualmente selecionado pela roda do mouse. */
    {
        const char *nome = nomeDoItem(tiposItens[itemSelecionado]);
        float textoW = larguraTexto(FONTE, nome);
        float textoX = x0 + largura / 2.0f;
        float textoY = y0 + lado + 39.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.02f, 0.02f, 0.03f, 0.86f);
        retangulo(textoX - textoW / 2.0f - 10.0f, textoY - 6.0f,
                  textoX + textoW / 2.0f + 10.0f, textoY + 19.0f);
        glDisable(GL_BLEND);

        glColor3f(0.0f, 0.0f, 0.0f);
        escreve(FONTE, textoX - textoW / 2.0f + 1.0f, textoY - 1.0f, nome);
        glColor3f(1.0f, 0.86f, 0.30f);
        escreve(FONTE, textoX - textoW / 2.0f, textoY, nome);
    }
}

static int slotInventarioEm(int x, int y)
{
    const float lado = 58.0f, espaco = 6.0f;
    const float painelW = 9.0f * lado + 8.0f * espaco + 24.0f;
    /* Reserva uma faixa maior no topo para o título ficar separado da grade. */
    const float painelH = 4.0f * lado + 3.0f * espaco + 100.0f;
    const float x0 = (larguraJanela - painelW) / 2.0f;
    const float y0 = (alturaJanela - painelH) / 2.0f;
    int coluna, linha;

    y = alturaJanela - y;
    x -= (int)(x0 + 12.0f);
    y -= (int)(y0 + 42.0f);
    if (x < 0 || y < 0) return -1;

    coluna = (int)(x / (lado + espaco));
    linha = (int)(y / (lado + espaco));
    if (coluna < 0 || coluna >= 9 || linha < 0 || linha >= 4)
        return -1;
    if (x % (int)(lado + espaco) >= (int)lado ||
        y % (int)(lado + espaco) >= (int)lado)
        return -1;
    return linha * 9 + coluna;
}

static void desenhaInventario(void)
{
    const float lado = 58.0f, espaco = 6.0f;
    const float painelW = 9.0f * lado + 8.0f * espaco + 24.0f;
    /* Reserva uma faixa maior no topo para o título ficar separado da grade. */
    const float painelH = 4.0f * lado + 3.0f * espaco + 100.0f;
    const float x0 = (larguraJanela - painelW) / 2.0f;
    const float y0 = (alturaJanela - painelH) / 2.0f;
    int i;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.04f, 0.06f, 0.96f);
    retangulo(x0, y0, x0 + painelW, y0 + painelH);
    glDisable(GL_BLEND);

    glColor3f(0.80f, 0.82f, 0.88f);
    desenhaTextoCentralizado(FONTE, larguraJanela / 2.0f,
                             y0 + 13.0f, "clique: mover pilha | direita: mover 1 | E ou ESC fecha");

    /* A profundidade da cena não deve esconder os mini-blocos do painel. */
    glClear(GL_DEPTH_BUFFER_BIT);
    for (i = 0; i < INVENTORY_SLOTS; i++) {
        int coluna = i % 9;
        int linha = i / 9;
        float x = x0 + 12.0f + coluna * (lado + espaco);
        float y = y0 + 42.0f + linha * (lado + espaco);
        ItemStack *slot = &inventario.slots[i];

        glColor3f(i == slotSelecionadoInventario ? 0.20f :
                  (i == itemSelecionado ? 1.0f : 0.28f),
                  i == slotSelecionadoInventario ? 0.95f :
                  (i == itemSelecionado ? 0.78f : 0.30f),
                  i == slotSelecionadoInventario ? 1.0f :
                  (i == itemSelecionado ? 0.18f : 0.34f));
        retangulo(x - 2.0f, y - 2.0f, x + lado + 2.0f, y + lado + 2.0f);
        glColor3f(0.08f, 0.09f, 0.12f);
        retangulo(x, y, x + lado, y + lado);

        if (slot->quantidade > 0) {
            desenhaIconeBloco3D(slot->tipo, x, y, lado);
            {
                char quantidade[16];
                sprintf(quantidade, "%d", slot->quantidade);
                glDisable(GL_TEXTURE_2D);
                glColor3f(0.0f, 0.0f, 0.0f);
                escreve(FONTE, x + lado - larguraTexto(FONTE, quantidade) - 2.0f,
                        y + 5.0f, quantidade);
                glColor3f(1.0f, 1.0f, 1.0f);
                escreve(FONTE, x + lado - larguraTexto(FONTE, quantidade) - 3.0f,
                        y + 6.0f, quantidade);
            }
        }
    }

    /* O título fica acima dos slots para não ser coberto pela camada da grade. */
    glColor3f(1.0f, 0.86f, 0.30f);
    desenhaTextoCentralizado(FONTE_TITULO, larguraJanela / 2.0f,
                             y0 + painelH - 36.0f, "INVENTARIO");
}

static void desenhaCrafting(void)
{
    const int lado = 58, espaco = 6;
    int i;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.04f, 0.06f, 0.97f);
    retangulo(80.0f, 42.0f, larguraJanela - 80.0f, alturaJanela - 36.0f);
    glDisable(GL_BLEND);

    glColor3f(1.0f, 0.86f, 0.30f);
    desenhaTextoCentralizado(FONTE_TITULO, larguraJanela / 2.0f,
                             alturaJanela - 70.0f, "CRAFTING");
    glColor3f(0.80f, 0.82f, 0.88f);
    desenhaTextoCentralizado(FONTE, larguraJanela / 2.0f, 55.0f,
                             "clique em um item para usar | C ou ESC volta ao inventario");

    glColor3f(0.35f, 0.38f, 0.45f);
    retangulo(240.0f, 450.0f, 400.0f, 604.0f);
    for (i = 0; i < 4; i++) {
        int coluna = i % 2, linha = i / 2;
        float x = 260.0f + coluna * (lado + espaco);
        float y = 470.0f + linha * (lado + espaco);
        ItemStack *slot = &gradeCrafting[i];

        glColor3f(0.18f, 0.22f, 0.28f);
        retangulo(x, y, x + lado, y + lado);
        if (slot->quantidade > 0)
            desenhaIconeBloco3D(slot->tipo, x, y, lado);
    }

    glColor3f(0.95f, 0.75f, 0.18f);
    retangulo(515.0f, 465.0f, 583.0f, 533.0f);
    glColor3f(0.18f, 0.22f, 0.28f);
    retangulo(520.0f, 470.0f, 578.0f, 528.0f);
    if (resultadoCrafting.quantidade > 0) {
        desenhaIconeBloco3D(resultadoCrafting.tipo, 520.0f, 470.0f, lado);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        {
            char quantidade[16];
            sprintf(quantidade, "%d", resultadoCrafting.quantidade);
            escreve(FONTE, 555.0f, 476.0f, quantidade);
        }
    }

    glColor3f(0.75f, 0.78f, 0.84f);
    escreve(FONTE, 252.0f, 435.0f,
            "2 areia = 1 vidro | 1 tronco + 1 folha = 4 tochas");
    escreve(FONTE, 252.0f, 419.0f,
            "2 pedra + 1 tronco = 1 picareta");

    /* Inventário inferior. */
    for (i = 0; i < INVENTORY_SLOTS; i++) {
        int coluna = i % 9, linha = i / 9;
        float x = 220.0f + coluna * (lado + espaco);
        float y = 72.0f + linha * (lado + espaco);
        ItemStack *slot = &inventario.slots[i];

        glColor3f(i < NUM_ITENS && i == itemSelecionado ? 1.0f : 0.30f,
                  i < NUM_ITENS && i == itemSelecionado ? 0.78f : 0.32f,
                  i < NUM_ITENS && i == itemSelecionado ? 0.18f : 0.38f);
        retangulo(x - 2.0f, y - 2.0f, x + lado + 2.0f, y + lado + 2.0f);
        glColor3f(0.08f, 0.09f, 0.12f);
        retangulo(x, y, x + lado, y + lado);
        if (slot->quantidade > 0) {
            char quantidade[16];
            desenhaIconeBloco3D(slot->tipo, x, y, lado);
            sprintf(quantidade, "%d", slot->quantidade);
            glDisable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            escreve(FONTE, x + lado - larguraTexto(FONTE, quantidade) - 3.0f,
                    y + 6.0f, quantidade);
        }
    }
}

static void desenhaDisco3D(float cx, float cy, float cz, float raio,
                            float r, float g, float b)
{
    int i;
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx, cy, cz);
    for (i = 0; i <= 24; i++) {
        float a = 2.0f * PI * i / 24.0f;
        glVertex3f(cx + cosf(a) * raio,
                   cy + sinf(a) * raio,
                   cz);
    }
    glEnd();
}

static void desenhaAstros(void)
{
    if (shaderCeu) return;
    static const float estrelas[][2] = {
        {0.08f, 0.78f}, {0.16f, 0.62f}, {0.23f, 0.86f}, {0.31f, 0.72f},
        {0.39f, 0.91f}, {0.47f, 0.68f}, {0.55f, 0.84f}, {0.63f, 0.75f},
        {0.71f, 0.90f}, {0.80f, 0.66f}, {0.90f, 0.82f}, {0.12f, 0.94f},
        {0.27f, 0.58f}, {0.44f, 0.79f}, {0.58f, 0.61f}, {0.75f, 0.55f},
        {0.86f, 0.95f}, {0.05f, 0.56f}, {0.35f, 0.63f}, {0.68f, 0.97f}
    };
    float angulo = (tempoMundo - 0.25f) * 2.0f * PI;
    float sunPixelX = larguraJanela * (0.5f + 0.48f * cosf(angulo));
    float sunPixelY = alturaJanela * (0.55f + 0.40f * sinf(angulo));
    float moonPixelX = larguraJanela * (0.5f - 0.48f * cosf(angulo));
    float moonPixelY = alturaJanela * (0.55f - 0.40f * sinf(angulo));
    const float distanciaCeu = 80.0f;
    float escalaX, escalaY, escalaRaio;
    float sunX, sunY, moonX, moonY;

    /* Os astros são sprites no espaço da câmera, mas continuam sujeitos
       ao depth test. Desse modo o céu pode acompanhar a tela e, ainda
       assim, uma montanha ou bloco à frente consegue ocultá-los. */
    if (projecaoOrtogonal) {
        double aspecto = (double)larguraJanela / alturaJanela;
        escalaX = 20.0f * (float)aspecto;
        escalaY = 20.0f;
        escalaRaio = 20.0f / alturaJanela;
    } else {
        float metadeAltura = tanf(campoDeVisao * PI / 360.0f) *
                             distanciaCeu;
        escalaY = 2.0f * metadeAltura;
        escalaX = escalaY * (float)larguraJanela / alturaJanela;
        escalaRaio = escalaY / alturaJanela;
    }
    sunX = ((sunPixelX / larguraJanela) - 0.5f) * escalaX;
    sunY = ((sunPixelY / alturaJanela) - 0.5f) * escalaY;
    moonX = ((moonPixelX / larguraJanela) - 0.5f) * escalaX;
    moonY = ((moonPixelY / alturaJanela) - 0.5f) * escalaY;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);

    if (sinf(angulo) < 0.0f) {
        int i;
        glPointSize(2.0f);
        glColor3f(0.88f, 0.93f, 1.0f);
        glBegin(GL_POINTS);
        for (i = 0; i < (int)(sizeof(estrelas) / sizeof(estrelas[0])); i++)
            glVertex3f(((estrelas[i][0]) - 0.5f) * escalaX,
                       ((estrelas[i][1]) - 0.5f) * escalaY,
                       -distanciaCeu + 1.0f);
        glEnd();
        glPointSize(1.0f);
    }
    /* O sol existe apenas durante o dia; a lua e as estrelas apenas à
       noite. Antes ambos eram desenhados ao mesmo tempo. */
    if (sinf(angulo) >= 0.0f &&
        sunPixelY > -50.0f && sunPixelY < alturaJanela + 50.0f)
        desenhaDisco3D(sunX, sunY, -distanciaCeu, 24.0f * escalaRaio,
                       1.0f, 0.82f, 0.24f);
    if (sinf(angulo) < 0.0f &&
        moonPixelY > -40.0f && moonPixelY < alturaJanela + 40.0f)
        desenhaDisco3D(moonX, moonY, -distanciaCeu, 18.0f * escalaRaio,
                       0.80f, 0.86f, 0.95f);

    glDepthMask(GL_TRUE);
    glPopMatrix();
}

static void desenhaCeuShader(void)
{
    float olhoX, olhoY, olhoZ;

    if (!shaderCeu || !ceuDomo) return;
    posicaoDoOlho(&olhoX, &olhoY, &olhoZ);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    /* O domo fica centrado no olho, mas mantém a matriz de câmera. Assim
       a direção do olhar realmente atravessa o céu, em vez de prender
       os astros à tela. */
    glTranslatef(olhoX, olhoY, olhoZ);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glUseProgram(shaderCeu);
    glUniform1f(shaderCeuTempo, tempoMundo);
    glUniform1f(shaderCeuSeed, (float)(sementeMundo % 4093u));
    glUniform1f(shaderCeuAnimacao, tempoAnimacaoCeu);
    gluSphere(ceuDomo, 180.0, 64, 32);
    glUseProgram(0);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

static void desenhaInstrucoes(void)
{
    static const char *linhas[][2] = {
        { "W / S",              "andar para frente / tras" },
        { "A / D",              "andar para os lados" },
        { "R (segurando)",      "correr" },
        { "Espaco / seta cima", "saltar" },
        { "Setas esquerda/direita", "girar o olhar" },
        { "Mouse",              "olhar ao redor" },
        { "Clique esquerdo",    "quebrar bloco" },
        { "Clique direito",     "colocar bloco" },
        { "Espada + clique/G",  "atacar inimigos" },
        { "E",                  "abrir inventario" },
        { "C (inventario)",     "abrir crafting" },
        { "Queda",              "causa dano acima de 3 blocos" },
        { "Hora",               "ciclo de dia e noite" },
        { "Tab",                "liberar ou capturar o cursor" },
        { "V ou F5",            "alternar primeira/terceira pessoa" },
        { "P",                  "alternar perspectiva/ortografica" },
        { "+ / -",              "aproximar ou afastar" },
        { "Roda",               "selecionar item" },
        { "1 a 9",              "selecionar espaco da barra" },
        { "ESC",                "abrir pausa ou voltar" },
        { "Q (menu)",           "sair do jogo" }
    };
    int i;
    int totalLinhas = (int)(sizeof(linhas) / sizeof(linhas[0]));
    float espacamento = alturaJanela < 720 ? 24.0f : 27.0f;
    float painelH = 100.0f + (totalLinhas - 1) * espacamento;
    float painelY;
    float painelW = larguraJanela < 868 ? larguraJanela - 48.0f : 820.0f;
    float painelX = (larguraJanela - painelW) / 2.0f;

    /* Reduz o espaçamento em janelas baixas para que a lista inteira,
       incluindo ESC e Q, permaneça dentro do fundo escurecido. */
    if (painelH > alturaJanela - 24.0f) {
        espacamento = (alturaJanela - 124.0f) / (totalLinhas - 1);
        if (espacamento < 18.0f) espacamento = 18.0f;
        painelH = 100.0f + (totalLinhas - 1) * espacamento;
    }
    painelY = (alturaJanela - painelH) / 2.0f;
    if (painelY < 12.0f) painelY = 12.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.05f, 0.08f, 0.94f);
    retangulo(painelX, painelY, painelX + painelW, painelY + painelH);
    glDisable(GL_BLEND);

    glColor3f(1.0f, 0.86f, 0.30f);
    desenhaTextoCentralizado(FONTE_TITULO, larguraJanela / 2.0f,
                             painelY + painelH - 42.0f, "INSTRUÇÕES");

    for (i = 0; i < totalLinhas; i++) {
        float y = painelY + painelH - 75.0f - i * espacamento;
        glColor3f(1.0f, 0.86f, 0.30f);
        escreve(FONTE, painelX + 28.0f, y, linhas[i][0]);
        glColor3f(0.92f, 0.92f, 0.92f);
        escreve(FONTE, painelX + 220.0f, y, linhas[i][1]);
    }

}

static void desenhaInformacoes(void)
{
    char texto[192];
    const char *nomeBioma;
    int biomaAtual = biomaProcedural((int)floorf(jogadorX),
                                     (int)floorf(jogadorZ));
    float hora = tempoMundo * 24.0f;

    switch (biomaAtual) {
        case 1: nomeBioma = "Deserto"; break;
        case 2: nomeBioma = "Tundra"; break;
        case 3: nomeBioma = "Floresta"; break;
        default: nomeBioma = "Planicie"; break;
    }

    glDisable(GL_TEXTURE_2D);

    sprintf(texto, "FPS: %d   Seed: %u   Bioma: %s   Hora: %05.2f   Posicao: (%.2f, %.2f, %.2f)",
            fpsExibido, sementeMundo, nomeBioma, hora,
            jogadorX, jogadorY, jogadorZ);
    glColor3f(1.0f, 1.0f, 1.0f);
    escreve(FONTE, 14.0f, alturaJanela - 28.0f, texto);

    sprintf(texto, "Saturacao: %.0f / %.0f", saturacaoJogador,
            SATURACAO_MAXIMA);
    glColor3f(0.55f, 0.85f, 1.0f);
    escreve(FONTE, 14.0f, alturaJanela - 52.0f, texto);
}

void movimentoInterface(int x, int y)
{
    if (estadoJogo == ESTADO_MENU_PRINCIPAL ||
        estadoJogo == ESTADO_PAUSADO ||
        estadoJogo == ESTADO_INSTRUCOES) {
        int i;
        for (i = 0; i < numeroBotoesTela(); i++)
            if (pontoNoBotao(x, y, i)) {
                botaoSelecionado = i;
                glutPostRedisplay();
                return;
            }
    }
}

void desenhaInterface(void)
{
    if (estadoJogo == ESTADO_JOGANDO) {
        desenhaMira();
        desenhaBarraItens();
        desenhaCoracoesVida();
        desenhaIconesFome();
        desenhaInformacoes();
        return;
    }

    if (estadoJogo == ESTADO_INVENTARIO) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.62f);
        retangulo(0, 0, larguraJanela, alturaJanela);
        glDisable(GL_BLEND);
        desenhaInventario();
        return;
    }

    if (estadoJogo == ESTADO_CRAFTING) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.62f);
        retangulo(0, 0, larguraJanela, alturaJanela);
        glDisable(GL_BLEND);
        desenhaCrafting();
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f,
              estadoJogo == ESTADO_MENU_PRINCIPAL ? 0.58f : 0.50f);
    retangulo(0, 0, larguraJanela, alturaJanela);
    glDisable(GL_BLEND);

    if (estadoJogo == ESTADO_INSTRUCOES) {
        desenhaInstrucoes();
        return;
    }

    if (estadoJogo == ESTADO_MENU_PRINCIPAL) {
        desenhaIdentidadeMenuPrincipal();
    } else {
        glColor3f(1.0f, 0.86f, 0.30f);
        desenhaTextoCentralizado(FONTE_TITULO, larguraJanela / 2.0f,
                                 alturaJanela * 0.72f, "PAUSADO");
    }
    desenhaBotao(0, estadoJogo == ESTADO_MENU_PRINCIPAL ? "JOGAR" : "CONTINUAR");
    if (estadoJogo == ESTADO_MENU_PRINCIPAL) {
        desenhaBotao(1, "SAIR");
    } else {
        desenhaBotao(1, "INSTRUÇÕES");
        desenhaBotao(2, "SAIR");
    }

    glColor3f(0.82f, 0.82f, 0.86f);
    if (estadoJogo == ESTADO_PAUSADO)
        desenhaTextoCentralizado(FONTE, larguraJanela / 2.0f,
                                 alturaJanela * 0.035f,
                                 "ESC ou CONTINUAR volta ao jogo");
}


/* ================================================================== */
/* QUADRO E PROGRAMA PRINCIPAL                                        */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/* Quadro: a GLUT chama esta função sempre que a janela precisa ser   */
/* redesenhada. A cada chamada:                                       */
/*   1. apaga a imagem anterior (cor e profundidade)                  */
/*   2. posiciona a câmera (primeira ou terceira pessoa)              */
/*   3. desenha a cena e, em terceira pessoa, o personagem            */
/*   4. mostra o resultado na tela                                    */
/*   5. atualiza a contagem de FPS                                    */
/* ------------------------------------------------------------------ */

int inicioContagem = 0;   /* início da contagem de FPS (semana 1), em ms */
int quadros = 0;          /* quadros desenhados desde inicioContagem */

static void atualizaCorCeu(void)
{
    float dia = sinf((tempoMundo - 0.25f) * 2.0f * PI);
    float t;
    float noite[3] = { 0.025f, 0.045f, 0.12f };
    float claro[3] = { 0.53f, 0.81f, 0.92f };

    if (dia < 0.0f) dia = 0.0f;
    t = dia * dia * (3.0f - 2.0f * dia);
    glClearColor(noite[0] + (claro[0] - noite[0]) * t,
                 noite[1] + (claro[1] - noite[1]) * t,
                 noite[2] + (claro[2] - noite[2]) * t, 1.0f);
}

void desenha(void)
{
    atualizaCorCeu();
    /* apaga a cor (fica o azul do glClearColor) e o buffer de
       profundidade, que guarda a distância de cada pixel */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    posicionaCamera();

    /* O shader cobre a tela antes da geometria do mundo. Os astros antigos
       continuam disponíveis como fallback caso o shader não compile. */
    desenhaCeuShader();
    desenhaAstros();

    /* opacos antes dos semitransparentes: a mistura precisa do que está
       atrás já no buffer, e isso inclui o personagem */
    desenhaOpacos();
    desenhaDrops();
    desenhaEntidades();
    desenhaEfeitos();
    if (modoCamera == TERCEIRA_PESSOA)
        desenhaJogador();
    desenhaTransparentes();
    desenhaSelecaoBloco();
    iniciaInterface();
    desenhaInterface();
    terminaInterface();

    /* desenhamos num buffer escondido; aqui ele troca de lugar com o que
       está na tela, para não aparecer a imagem pela metade */
    glutSwapBuffers();

    /* conta os quadros para o texto da interface */
    quadros++;
    int agora = glutGet(GLUT_ELAPSED_TIME);
    if (agora - inicioContagem >= 1000) {
        fpsExibido = quadros;
        quadros = 0;
        inicioContagem = agora;
    }
}

/* ------------------------------------------------------------------ */
/* Programa principal                                                 */
/* Cria a janela, registra as funções que a GLUT vai chamar em cada   */
/* situação (callbacks), liga os testes do OpenGL, constrói o mundo e */
/* entra no laço principal, de onde não sai mais.                     */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    unsigned int seedLinha = sementeMundo;
    int encontrouSeed = 0;
    int argumento, argumentoDestino = 1;

    for (argumento = 1; argumento < argc; argumento++) {
        if (strcmp(argv[argumento], "--seed") == 0 && argumento + 1 < argc) {
            seedLinha = (unsigned int)strtoul(argv[++argumento], 0, 10);
            encontrouSeed = 1;
        } else if (strncmp(argv[argumento], "--seed=", 7) == 0) {
            seedLinha = (unsigned int)strtoul(argv[argumento] + 7, 0, 10);
            encontrouSeed = 1;
        } else {
            argv[argumentoDestino++] = argv[argumento];
        }
    }
    argc = argumentoDestino;
    argv[argc] = 0;
    if (encontrouSeed) defineSementeMundo(seedLinha);

    /* janela com buffer duplo (GLUT_DOUBLE), cores RGB e buffer de
       profundidade (GLUT_DEPTH) */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(larguraJanela, alturaJanela);
    glutCreateWindow("Minecraft");

#ifdef _WIN32
    /* Maximiza a janela mantendo a moldura e a barra de título, e usa
       o ícone embutido no executável nos dois tamanhos da janela. */
    {
        HWND janela = GetForegroundWindow();
        HINSTANCE modulo = GetModuleHandle(NULL);
        HICON iconeGrande = (HICON)LoadImage(
            modulo, MAKEINTRESOURCE(IDI_MINECRAFT), IMAGE_ICON,
            32, 32, LR_DEFAULTSIZE);
        HICON iconePequeno = (HICON)LoadImage(
            modulo, MAKEINTRESOURCE(IDI_MINECRAFT), IMAGE_ICON,
            16, 16, LR_DEFAULTSIZE);

        if (janela != NULL) {
            if (iconeGrande != NULL)
                SendMessage(janela, WM_SETICON, ICON_BIG, (LPARAM)iconeGrande);
            if (iconePequeno != NULL)
                SendMessage(janela, WM_SETICON, ICON_SMALL, (LPARAM)iconePequeno);
            ShowWindow(janela, SW_MAXIMIZE);
        }
    }
#endif

#ifndef __APPLE__
    /* a GLEW só pode ser iniciada depois que a janela existe */
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printf("Erro GLEW: %s\n", glewGetErrorString(err));
        return 1;
    }
#endif

    printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    carregaAtlas();                         /* lê o atlas e liga a textura */
    inicializaShaderAgua();                 /* ondulação e brilho da água */
    inicializaShaderCeu();                  /* gradiente e astros do céu */
    carregaTexturasUI();                    /* lê os elementos gráficos do menu */

    /* as teclas são lidas por estado: a repetição automática do sistema
       operacional não acrescenta nada e é desligada */
    glutIgnoreKeyRepeat(1);

    /* callbacks: a GLUT chama cada função quando o evento acontece */
    glutReshapeFunc(redimensiona);          /* janela abriu ou mudou de tamanho */
    glutDisplayFunc(desenha);               /* hora de desenhar */
    glutKeyboardFunc(teclado);              /* tecla comum pressionada */
    glutKeyboardUpFunc(tecladoSolto);       /* tecla comum solta */
    glutSpecialFunc(especial);              /* tecla especial pressionada */
    glutSpecialUpFunc(especialSolto);       /* tecla especial solta */
    glutPassiveMotionFunc(movimentoMouse);  /* mouse movido sem botão */
    glutMotionFunc(movimentoMouse);         /* mouse movido com botão */
#ifndef __APPLE__
    glutMouseWheelFunc(rodaMouse);          /* roda do mouse (freeglut) */
#endif
    glutMouseFunc(cliqueMouse);             /* clique: captura o cursor */
    glutEntryFunc(entradaJanela);           /* cursor entrou na janela */

    glutIdleFunc(atualiza);                 /* nada acontecendo: atualiza */

    glEnable(GL_DEPTH_TEST);                /* o mais próximo cobre o mais distante,
                                               não importa a ordem de desenho */
    glEnable(GL_CULL_FACE);                 /* não desenha faces de costas */
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);   /* cor de fundo: azul do céu */

    inicializaChunks();                     /* prepara as regiões do mundo */
    construirMundo();                       /* preenche a matriz, uma vez só */
    printf("Seed do mundo: %u\n", sementeMundo);
    iniciaInventario();                     /* estoque inicial da barra */
    inicializaAudio();                      /* abre os WAVs antes dos eventos */
    iniciaEntidades();                      /* entidades hostis iniciais */
    iniciaEfeitos();                        /* chuva e partículas */
    liberaCursor();                         /* menu inicial: cursor visivel */

    ultimoTempo = glutGet(GLUT_ELAPSED_TIME);
    inicioContagem = ultimoTempo;

    glutMainLoop();                         /* laço principal: só sai com Q no menu */
    return 0;
}
