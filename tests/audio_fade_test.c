/* Teste de integracao Windows: canal, volume e reproducao independentes. */
#include "../src/audio.c"

int main(void)
{
#ifdef _WIN32
    DWORD volumeOutroAntes = 0, volumeOutroDepois = 0;
    LONG volumeInicial, volumeIntermediario, volumeFinal;
    HWAVEOUT outroCanal = NULL;
    WAVEFORMATEX outroFormato = {0};
    MMTIME posicao = {0};
    int ok = 1;
    inicializaAudio();
    if (!musicaNoiteDisponivel) return 1;
    audioAtualizaNoite(0.75f, 240.0f);
    outroFormato.wFormatTag = WAVE_FORMAT_PCM;
    outroFormato.nChannels = 2;
    outroFormato.nSamplesPerSec = 44100;
    outroFormato.wBitsPerSample = 16;
    outroFormato.nBlockAlign = 4;
    outroFormato.nAvgBytesPerSec = 176400;
    if (waveOutOpen(&outroCanal, WAVE_MAPPER, &outroFormato, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        ok = 0;
    if (outroCanal) waveOutGetVolume(outroCanal, &volumeOutroAntes);
    Sleep(250);
    posicao.wType = TIME_MS;
    if (waveOutGetPosition(musicaSaida, &posicao, sizeof(posicao)) != MMSYSERR_NOERROR ||
        posicao.u.ms == 0 || !musicaNoiteTocando) ok = 0;
    volumeInicial = InterlockedCompareExchange(&musicaGanho, 0, 0);
    audioDano();
    audioAtualizaNoite(0.999f, 240.0f);
    if (InterlockedCompareExchange(&musicaGanho, 0, 0) != volumeInicial) ok = 0;
    audioAtualizaNoite(0.0f, 240.0f);
    if (InterlockedCompareExchange(&musicaGanho, 0, 0) != volumeInicial) ok = 0;
    audioAtualizaNoite(0.125f, 240.0f);
    volumeIntermediario = InterlockedCompareExchange(&musicaGanho, 0, 0);
    if (outroCanal) {
        waveOutGetVolume(outroCanal, &volumeOutroDepois);
        if (volumeOutroAntes != volumeOutroDepois) ok = 0;
    }
    if (!musicaNoiteTocando || volumeIntermediario >= volumeInicial ||
        volumeIntermediario == 0) ok = 0;
    audioAtualizaNoite(0.249f, 240.0f);
    if (InterlockedCompareExchange(&musicaGanho, 0, 0) <= 0 || !musicaNoiteTocando) ok = 0;
    audioAtualizaNoite(0.25f, 240.0f);
    volumeFinal = InterlockedCompareExchange(&musicaGanho, 0, 0);
    if (musicaNoiteTocando || volumeFinal != 0) ok = 0;
    Sleep(30);
    audioAtualizaNoite(0.75f, 240.0f);
    if (!musicaNoiteTocando) ok = 0;
    printf("Posicao=%lu, ganho=%ld -> %ld -> %ld, outro canal=%lu -> %lu, reinicio=%d\n",
           (unsigned long)posicao.u.ms, (long)volumeInicial,
           (long)volumeIntermediario, (long)volumeFinal,
           (unsigned long)volumeOutroAntes, (unsigned long)volumeOutroDepois,
           musicaNoiteTocando);
    if (outroCanal) waveOutClose(outroCanal);
    encerraMusicaNoite();
    return ok ? 0 : 2;
#else
    return 0;
#endif
}
