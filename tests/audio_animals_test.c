/* Executar um binario temporario na raiz para encontrar assets/audio. */
#include "../src/audio.c"
#include <assert.h>

int main(void)
{
#ifdef _WIN32
    inicializaAudio();
    assert(latidoDisponivel);
    audioAlertaCachorro();
    assert(latidoTocando && latidosPendentes == 0);
    audioAlertaCachorro();
    assert(latidosPendentes == 1);
    /* Simula o fim do primeiro buffer sem depender da latencia do dispositivo. */
    waveOutReset(buscaEfeito("sndDog")->saida);
    audioAtualizaLatidos();
    assert(latidoTocando && latidosPendentes == 0);
    audioMontaria();
    assert(buscaEfeito("sndHorse") &&
           !(buscaEfeito("sndHorse")->buffer.dwFlags & WHDR_DONE));
    assert(buscaEfeito("sndHorseAngry"));
    audioCavaloProximo();
    assert(!(buscaEfeito("sndHorseAngry")->buffer.dwFlags & WHDR_DONE));
    assert(buscaEfeito("sndPig"));
    audioPorcoProximo();
    assert(!(buscaEfeito("sndPig")->buffer.dwFlags & WHDR_DONE));
    assert(buscaEfeito("sndWalk") && buscaEfeito("sndWalkOther") && buscaEfeito("sndGlass"));
    assert(buscaEfeito("sndWater"));
    audioAtualizaAgua(0);
    audioAtualizaAgua(1);
    assert(estavaNaAgua && !(buscaEfeito("sndWater")->buffer.dwFlags & WHDR_DONE));
    waveOutReset(buscaEfeito("sndWater")->saida);
    audioAtualizaAgua(1);
    assert(buscaEfeito("sndWater")->buffer.dwFlags & WHDR_DONE);
    audioAtualizaAgua(0);
    audioAtualizaAgua(1);
    assert(!(buscaEfeito("sndWater")->buffer.dwFlags & WHDR_DONE));
    audioAtualizaPassos(0.0f, 1, GRAMA);
    assert(!(buscaEfeito("sndWalk")->buffer.dwFlags & WHDR_DONE));
    assert(tempoProximoPasso == 0.50f);
    {
        int tipos[] = { GRAMA, PEDRA, TERRA, AREIA, NEVE, TRONCO, TRONCO_CEREJEIRA };
        unsigned i;
        for (i = 0; i < sizeof(tipos) / sizeof(tipos[0]); i++) {
            assert(usaSomPassoGrama(tipos[i]));
            audioAtualizaPassos(0.0f, 0, VAZIO);
            audioAtualizaPassos(0.0f, 1, tipos[i]);
            assert(tempoProximoPasso == 0.50f);
        }
    }
    audioAtualizaPassos(0.0f, 0, VAZIO);
    audioAtualizaPassos(0.0f, 1, VIDRO);
    assert(!(buscaEfeito("sndWalkOther")->buffer.dwFlags & WHDR_DONE));
    assert(tempoProximoPasso == 0.38f);
    audioAtualizaGalope(1); assert(indiceGalope == 0);
    waveOutReset(buscaEfeito("sndGallop1")->saida);
    audioAtualizaGalope(1); assert(indiceGalope == 1);
    waveOutReset(buscaEfeito("sndGallop2")->saida);
    audioAtualizaGalope(1); assert(indiceGalope == 2);
    waveOutReset(buscaEfeito("sndGallop3")->saida);
    audioAtualizaGalope(1); assert(indiceGalope == 3);
    waveOutReset(buscaEfeito("sndGallop4")->saida);
    audioAtualizaGalope(1); assert(indiceGalope == 0);
    audioAtualizaGalope(0); assert(indiceGalope == -1);
    assert(buscaEfeito("sndZombieFire"));
    audioAtualizaFogoZumbi(1); assert(fogoZumbiTocando);
    waveOutReset(buscaEfeito("sndZombieFire")->saida);
    audioAtualizaFogoZumbi(1);
    assert(fogoZumbiTocando && !(buscaEfeito("sndZombieFire")->buffer.dwFlags & WHDR_DONE));
    audioAtualizaFogoZumbi(0);
    assert(!fogoZumbiTocando && (buscaEfeito("sndZombieFire")->buffer.dwFlags & WHDR_DONE));
    puts("audio_animals_test: OK");
#endif
    return 0;
}
