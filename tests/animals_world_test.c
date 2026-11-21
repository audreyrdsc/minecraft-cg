/* Integracao de geracao, animais, montaria e migracao do save v5. */
static int alertasTeste = 0, montariasTeste = 0, fogoTeste = -1, cavaloProximoTeste = 0, porcoProximoTeste = 0;
#define audioAlertaCachorro testaAlerta
#define audioMontaria testaMontaria
#define audioAtualizaFogoZumbi testaFogo
#define audioCavaloProximo testaCavaloProximo
#define audioPorcoProximo testaPorcoProximo
#include "../src/entities.c"
#undef audioAlertaCachorro
#undef audioMontaria
#undef audioAtualizaFogoZumbi
#undef audioCavaloProximo
#undef audioPorcoProximo
#include "../src/save.c"
#include <assert.h>

void testaAlerta(void) { alertasTeste++; }
void testaMontaria(void) { montariasTeste++; }
void testaFogo(int queimando) { fogoTeste = queimando; }
void testaCavaloProximo(void) { cavaloProximoTeste++; }
void testaPorcoProximo(void) { porcoProximoTeste++; }

static unsigned char mundoAntigo[64][ALTURA_MUNDO][64];

int main(void)
{
    char pasta[MAX_PATH], caminho[MAX_PATH], backup[MAX_PATH + 16];
    CabecalhoSave cabecalho;
    FILE *arquivo;
    float distanciaAntes, distanciaDepois, xCavalo, alturaAntes;
    int x, z, i;
    for (i = 0; i < 100; i++) {
        float fase = i * 0.1f;
        float esquerda = anguloBracoZumbi(fase, 0);
        float direita = anguloBracoZumbi(fase, 1);
        assert(esquerda >= 68.0f && esquerda <= 88.0f);
        assert(direita >= 68.0f && direita <= 88.0f);
        assert(fabsf(esquerda + direita - 156.0f) < 0.001f);
    }
    assert(GetTempPathA(MAX_PATH, pasta) > 0);
    assert(GetTempFileNameA(pasta, "mcg", 0, caminho) != 0);
    snprintf(backup, sizeof(backup), "%s.v5.bak", caminho);
    jogadorX = 8.5f; jogadorY = 3.0f; jogadorZ = 10.5f;
    defineSementeMundo(20261119u);
    construirMundo();
    iniciaAnimais();
    for (i = 0; i < NUM_ANIMAIS; i++) assert(animais[i].ativo);
    assert(animais[ANIMAL_CAVALO].x == 12.0f && animais[ANIMAL_CAVALO].y == 3.0f &&
           animais[ANIMAL_CAVALO].z == 2.5f && animalLivre(12.0f, 3.0f, 2.5f, 2.3f));
    assert(mundo[26][4][7] == TRONCO_CEREJEIRA);
    assert(mundo[30][1][31] == AGUA);
    assert(mundo[30][3][42] == GRAMA);
    memset(mundo, 0, sizeof(mundo));
    for (x = 0; x < LADO; x++)
        for (z = 0; z < LADO; z++) mundo[x][0][z] = GRAMA;
    jogadorX = 40.5f; jogadorY = 1.0f; jogadorZ = 40.5f;
    defineSementeMundo(12345u);
    iniciaAnimais();
    for (i = 0; i < NUM_ANIMAIS; i++) assert(animais[i].ativo);
    distanciaAntes = hypotf(animais[ANIMAL_CACHORRO].x - jogadorX,
                            animais[ANIMAL_CACHORRO].z - jogadorZ);
    for (i = 0; i < 30; i++) atualizaAnimais(0.1f);
    distanciaDepois = hypotf(animais[ANIMAL_CACHORRO].x - jogadorX,
                             animais[ANIMAL_CACHORRO].z - jogadorZ);
    assert(distanciaDepois < distanciaAntes);

    jogadorProximoPorco = 0; porcoProximoTeste = 0;
    jogadorX = animais[ANIMAL_PORCO].x + 2.1f;
    jogadorY = animais[ANIMAL_PORCO].y;
    jogadorZ = animais[ANIMAL_PORCO].z;
    atualizaProximidadePorco(); assert(porcoProximoTeste == 0);
    jogadorX = animais[ANIMAL_PORCO].x + 2.0f;
    atualizaProximidadePorco(); atualizaProximidadePorco();
    assert(porcoProximoTeste == 1);
    jogadorX += 0.2f; atualizaProximidadePorco();
    jogadorX -= 0.4f; atualizaProximidadePorco(); assert(porcoProximoTeste == 2);
    animais[ANIMAL_PORCO].ativo = 0; atualizaProximidadePorco();
    assert(porcoProximoTeste == 2);
    animais[ANIMAL_PORCO].ativo = 1;

    /* Um som por entrada no raio tridimensional de um bloco, nunca montado. */
    jogadorProximoCavalo = 0;
    jogadorX = animais[ANIMAL_CAVALO].x + 1.01f;
    jogadorY = animais[ANIMAL_CAVALO].y;
    jogadorZ = animais[ANIMAL_CAVALO].z;
    atualizaProximidadeCavalo(); assert(cavaloProximoTeste == 0);
    jogadorX = animais[ANIMAL_CAVALO].x + 1.0f;
    atualizaProximidadeCavalo(); atualizaProximidadeCavalo();
    assert(cavaloProximoTeste == 1);
    jogadorX += 0.2f; atualizaProximidadeCavalo();
    jogadorX -= 0.5f; atualizaProximidadeCavalo(); assert(cavaloProximoTeste == 2);
    cavaloMontado = 1; jogadorX += 2.0f; atualizaProximidadeCavalo();
    jogadorX = animais[ANIMAL_CAVALO].x; atualizaProximidadeCavalo();
    assert(cavaloProximoTeste == 2);
    cavaloMontado = 0; atualizaProximidadeCavalo(); assert(cavaloProximoTeste == 2);
    jogadorX += 2.0f; atualizaProximidadeCavalo();
    jogadorX -= 2.0f; atualizaProximidadeCavalo(); assert(cavaloProximoTeste == 3);

    jogadorX = animais[ANIMAL_CAVALO].x + 1.0f;
    jogadorY = animais[ANIMAL_CAVALO].y;
    jogadorZ = animais[ANIMAL_CAVALO].z;
    assert(alternaMontaria() && jogadorMontado());
    assert(montariasTeste == 1);
    xCavalo = animais[ANIMAL_CAVALO].x;
    moveMontaria(0.3f, 0.0f, 0.05f, 0);
    assert(animais[ANIMAL_CAVALO].x > xCavalo && jogadorX == animais[ANIMAL_CAVALO].x);
    alturaAntes = animais[ANIMAL_CAVALO].y;
    moveMontaria(0.0f, 0.0f, 0.05f, 1);
    assert(animais[ANIMAL_CAVALO].y > alturaAntes);
    for (i = 0; i < 40; i++) moveMontaria(0.0f, 0.0f, 0.05f, 0);
    assert(fabsf(animais[ANIMAL_CAVALO].y - alturaAntes) < 0.10f);
    {
        int paredeX = (int)floorf(animais[ANIMAL_CAVALO].x + 0.8f);
        int paredeZ = (int)floorf(animais[ANIMAL_CAVALO].z);
        float antes = animais[ANIMAL_CAVALO].x;
        mundo[paredeX][1][paredeZ] = PEDRA;
        mundo[paredeX][2][paredeZ] = PEDRA;
        moveMontaria(0.8f, 0.0f, 0.05f, 0);
        assert(animais[ANIMAL_CAVALO].x == antes);
        mundo[paredeX][1][paredeZ] = VAZIO;
        mundo[paredeX][2][paredeZ] = VAZIO;
    }
    assert(salvaJogo(caminho));
    cancelaMontaria();
    animais[ANIMAL_CAVALO].x = 10.0f;
    assert(carregaJogo(caminho));
    assert(jogadorMontado() && animais[ANIMAL_CAVALO].x > xCavalo);
    assert(alternaMontaria() && !jogadorMontado());
    assert(montariasTeste == 2);

    /* Cada inimigo gera exatamente um alerta na entrada, inclusive a 5m. */
    memset(zumbis, 0, sizeof(zumbis));
    jogadorX = 40.0f; jogadorY = 1.0f; jogadorZ = 40.0f;
    zumbis[0].ativo = 1; zumbis[0].x = 45.01f;
    zumbis[0].y = 1.0f; zumbis[0].z = 40.0f;
    atualizaAlertaCachorro(); assert(alertasTeste == 0);
    zumbis[0].x = 45.0f;
    atualizaAlertaCachorro(); atualizaAlertaCachorro(); assert(alertasTeste == 1);
    zumbis[1] = zumbis[0];
    atualizaAlertaCachorro(); assert(alertasTeste == 2);
    zumbis[0].x = 46.0f; atualizaAlertaCachorro();
    zumbis[0].x = 44.0f; atualizaAlertaCachorro(); assert(alertasTeste == 3);
    animais[ANIMAL_CACHORRO].ativo = 0;
    zumbis[2] = zumbis[0]; atualizaAlertaCachorro(); assert(alertasTeste == 3);
    memset(fogos, 0, sizeof(fogos));
    atualizaSomFogoZumbis(); assert(fogoTeste == 0);
    fogos[0].ativo = 1; fogos[1].ativo = 1;
    atualizaSomFogoZumbis(); assert(fogoTeste == 1);
    zumbis[0].ativo = 0; atualizaSomFogoZumbis(); assert(fogoTeste == 1);
    zumbis[1].ativo = 0; atualizaSomFogoZumbis(); assert(fogoTeste == 0);

    /* Um save v6 genuino: modifica uma regiao e conserva outra intocada. */
    defineSementeMundo(20261119u);
    defineGeracaoLegada(1);
    construirMundo();
    defineGeracaoLegada(0);
    mundo[35][10][40] = VIDRO;
    jogadorX = 8.5f; jogadorY = 3.0f; jogadorZ = 10.5f;
    iniciaAnimais();
    memset(&cabecalho, 0, sizeof(cabecalho));
    memcpy(cabecalho.assinatura, "MCGSV01", 7);
    cabecalho.versao = 6; cabecalho.seed = 20261119u;
    cabecalho.jogadorX = jogadorX; cabecalho.jogadorY = jogadorY; cabecalho.jogadorZ = jogadorZ;
    arquivo = fopen(caminho, "wb"); assert(arquivo);
    assert(fwrite(&cabecalho, sizeof(cabecalho), 1, arquivo) == 1);
    assert(fwrite(mundo, sizeof(mundo), 1, arquivo) == 1);
    assert(fwrite(&inventario, sizeof(inventario), 1, arquivo) == 1);
    assert(salvaEstadoEntidades(arquivo) && salvaEstadoAnimais(arquivo));
    assert(fclose(arquivo) == 0);
    assert(carregaJogo(caminho));
    assert(mundo[35][10][40] == VIDRO);
    assert(mundo[26][4][7] == TRONCO_CEREJEIRA && mundo[30][1][31] == AGUA);
    snprintf(backup, sizeof(backup), "%s.v6.bak", caminho);
    arquivo = fopen(backup, "rb"); assert(arquivo); fclose(arquivo);
    remove(backup);
    snprintf(backup, sizeof(backup), "%s.v5.bak", caminho);

    memset(&cabecalho, 0, sizeof(cabecalho));
    memcpy(cabecalho.assinatura, "MCGSV01", 7);
    cabecalho.versao = 5; cabecalho.seed = 12345u;
    cabecalho.jogadorX = 8.5f; cabecalho.jogadorY = 3.0f; cabecalho.jogadorZ = 10.5f;
    memset(mundoAntigo, 0, sizeof(mundoAntigo));
    mundoAntigo[5][1][5] = VIDRO;
    arquivo = fopen(caminho, "wb");
    assert(arquivo);
    assert(fwrite(&cabecalho, sizeof(cabecalho), 1, arquivo) == 1);
    assert(fwrite(mundoAntigo, sizeof(mundoAntigo), 1, arquivo) == 1);
    assert(fwrite(&inventario, sizeof(inventario), 1, arquivo) == 1);
    assert(salvaEstadoEntidades(arquivo));
    assert(fclose(arquivo) == 0);
    if (!carregaJogo(caminho)) { perror("migracao v5"); return 4; }
    assert(mundo[5][1][5] == VIDRO && !jogadorMontado());
    assert(mundo[100][0][100] != VAZIO);
    arquivo = fopen(backup, "rb");
    assert(arquivo); fclose(arquivo);
    remove(backup);
    remove(caminho);
    puts("animals_world_test: OK");
    return 0;
}
