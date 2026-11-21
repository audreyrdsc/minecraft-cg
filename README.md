# minecraft-cg

![Demonstração: o personagem passa pela casinha de vidro, olha as folhas da árvore em primeira pessoa, atravessa o lago e a câmera volta a mostrar a ilha](midia/demo.webp)

Um cenário no estilo Minecraft feito em C com OpenGL no pipeline fixo, como material da disciplina de Computação Gráfica.

O núcleo executável continua em [`mine.c`](mine.c), comentado para ser lido do começo ao fim, e os tipos compartilhados começaram a ser extraídos para [`include/game_types.h`](include/game_types.h). Essa migração incremental preserva o ponto de entrada enquanto prepara a separação dos sistemas em módulos.

## Estado atual do projeto

O código-fonte abaixo deve ser recompilado em `mine.exe` após alterações. O jogo inicia às 6h, no
nascer do sol (`tempoMundo = 0.25`), chega ao meio-dia às 12h, tem pôr do sol
às 18h e meia-noite às 0h. O spawn de inimigos e a música noturna ocorrem no
intervalo das 18h às 6h.

- O céu usa shader GLSL 1.20 em um domo 3D centrado na câmera, com transição
  suave de cores, brilho de horizonte, sol e lua com halo, estrelas com
  cintilação, nuvens procedurais em múltiplas formações pseudoaleatórias e uma
  passagem eventual de cometa à noite. As nuvens têm escalas e velocidades
  diferentes, e os astros desaparecem suavemente antes de cruzar o horizonte.
  Os elementos acompanham a orientação vertical do olhar e não ficam presos à
  tela. Nuvens e estrelas usam tempo contínuo, e estrelas cadentes e cometas
  têm trajetórias e intervalos distintos durante a noite.
- As nuvens estáticas de blocos foram desativadas para não duplicar a camada
  procedural do céu.
- A água usa shader GLSL 1.20 com ondulação leve, transparência, reflexos e
  brilho animado.
- O mundo tem 128 × 128 colunas (quatro vezes a área de 64 × 64), com
  árvores de maçã, cerejeiras, planícies, lagos, rio sinuoso e vegetação baixa.
  No mundo novo, procure o bosque próximo de (26, 7), o rio ao redor de
  z=28–32 e a planície aproximadamente em x=20–55, z=37–55 (coordenadas x, z).
- Porco e cavalo caminham pelo cenário; o cachorro segue Steve. Perto do
  cavalo, `M` monta ou desmonta, e os controles de movimento e salto conduzem
  a montaria. Em mundos novos o cavalo surge em (12, 3, 2.5), se livre.
  O nome Steve aparece acima do personagem em terceira pessoa.
- A HUD possui corações, fome e saturação em ícones pixelados, barra de itens,
  vida sobre inimigos e feedback de combate.
- Os sons ficam em `assets/audio/` no formato WAV: dano do jogador, comer,
  passos, quebra/coleta de blocos, vidro, água, ataque e morte de zumbi,
  spawn de zumbi, disparo e música noturna em repetição. O cachorro late duas
  vezes quando cada inimigo entra a até cinco blocos de Steve; novos alertas
  aguardam o anterior terminar. Ao se aproximar a dois blocos do porco, Steve
  ouve o som dele uma vez por aproximação. Ao se aproximar a um bloco do
  cavalo sem estar montado, Steve ouve outro som uma vez por aproximação.
  Montar/desmontar toca outro som; montado e em movimento, o galope reproduz
  os quatro sons em sequência. O
  som de fogo se repete enquanto houver pelo menos um zumbi queimando.
- No Windows, a música noturna é reproduzida em um canal próprio e sua
  intensidade diminui suavemente durante os 60 segundos reais anteriores às
  6h. O esmaecimento é aplicado apenas à música, preservando o volume dos
  efeitos sonoros. Passos na grama usam `walking.wav`; em outras superfícies,
  o som anterior permanece em `walking-other.wav`. Os efeitos usam buffers
  independentes, inclusive quando o caminho do projeto é longo, e podem tocar
  junto com a música.

Limitação conhecida: a câmera em terceira pessoa ainda não verifica colisão
com o terreno. Perto de paredes ou sob blocos, ela pode entrar no cenário e
mostrar faces internas ou vazios; mover Steve ou girar a câmera pode contornar
o problema temporariamente.

### Compilação atual no Windows

No PowerShell, a partir da pasta do projeto, o executável principal é sempre
gerado em `./mine.exe`:

```powershell
$files=@('mine.c','src/inventory.c','src/crafting.c','src/entities.c','src/world.c','src/chunks.c','src/worldgen.c','src/save.c','src/effects.c','src/audio.c','src/player.c')
& 'C:\msys64\mingw64\bin\gcc.exe' $files -o mine.exe -lglew32 -lfreeglut -lopengl32 -lglu32 -lwinmm
```

O `-lwinmm` é necessário para a reprodução WAV pelo Windows MCI/PlaySound.
O jogo deve ser executado a partir da pasta do projeto para encontrar o atlas
e os arquivos de áudio.

### Organização atual

- `mine.c`: ponto de entrada e implementação legada dos sistemas do protótipo.
- `include/game_types.h`: contratos de dados para inventário, drops, raycast e iluminação.
- `include/config.h`: dimensões compartilhadas do mundo e dos chunks.
- `include/inventory.h` e `src/inventory.c`: API e operações de pilhas, hotbar e inventário.
- `include/crafting.h` e `src/crafting.c`: receitas, grade de crafting e consumo de ingredientes.
- `include/entities.h` e `src/entities.c`: drops físicos, coleta, zumbis e slimes com comportamento e desenho distintos.
- `src/entities.c` também contém o primeiro zumbi e a API de combate básico.
- `include/world.h` e `src/world.c`: matriz 3D do mundo e operações seguras de leitura/escrita.
- `include/chunks.h` e `src/chunks.c`: divisão lógica em chunks de 16 × 16, marcação de regiões alteradas, carregamento por distância do jogador, cache invalidável de faces visíveis e listas de exibição reconstruídas sob demanda.
- `include/worldgen.h` e `src/worldgen.c`: seed e geração procedural determinística da extensão do terreno.
- `include/save.h` e `src/save.c`: salvamento/carregamento binário do mundo, jogador, clima e entidades.
- `include/effects.h` e `src/effects.c`: chuva/neve alternável por bioma e partículas de quebra de blocos.
- `include/audio.h` e `src/audio.c`: feedback sonoro portátil para quebra, coleta, dano e alimentação.
- `include/blocks.h`: catálogo centralizado de blocos, itens, ferramentas e passadas de renderização.
- `include/player.h`: contrato público de estado, colisão, movimentação e reaparecimento do jogador.
- `src/player.c`: estado, colisão AABB, movimento e reaparecimento do jogador.
- `CMakeLists.txt` e `Makefile`: construção reproduzível nas plataformas suportadas.

O arquivo principal é organizado em seções, nesta ordem:

| Seção | Conteúdo |
|---|---|
| Constantes | Tipos de bloco, tamanho do mundo, medidas do personagem e da câmera |
| Declarações antecipadas | Funções e variáveis usadas antes de serem definidas; servem de índice |
| TEXTURAS E TRANSPARÊNCIA | Tudo o que a aula de texturas trouxe, num bloco só: atlas, célula de cada face, faces ocultas, casinha de vidro e três passadas |
| MUNDO | Mapa, matriz do mundo, construção e desenho dos blocos |
| COLISÃO | Posição do personagem e colisão com o mundo |
| CÂMERA | Projeção, direção do olhar, primeira e terceira pessoa, desenho do personagem |
| ENTRADA | Teclado, mouse, roda e atualização do movimento |
| INTERFACE | Menu principal, pausa, instruções, mira, barra de itens e informações em tela |
| QUADRO E PROGRAMA PRINCIPAL | Desenho do quadro, FPS e `main` |

As funções centrais têm um comentário antes delas explicando o que fazem e por quê.

## O que tem no cenário

- Um terreno de 128 × 128 colunas de blocos: a área original de 16 × 16 foi preservada e recebeu uma grande extensão procedural.
- Uma montanha irregular com neve no topo.
- Um gramado com um laguinho.
- Uma praia e o mar.
- Uma árvore no formato do Minecraft, com as folhas vazadas.
- Árvores com maçãs, bosques de cerejeiras rosas e vegetação em planos cruzados.
- Um porco, um cachorro que segue o jogador e um cavalo montável com `M`.
- Uma casinha de vidro ao lado de onde o personagem começa.
- Nuvens procedurais animadas pelo shader do céu, com posições pseudoaleatórias,
  ruído em múltiplas escalas e deslocamento lento.
- Um personagem no estilo do Minecraft, com braços e pernas que balançam ao andar.
- Gravidade, salto e colisão vertical com o terreno e o teto.
- Quebra de blocos com o botão esquerdo e colocação do item selecionado com o botão direito.
- Com um bloco sólido selecionado, olhe para baixo e clique com o botão direito
  para colocá-lo sob os pés e subir, desde que haja apoio e espaço acima.
- Drops físicos simples que caem no mundo e são coletados pelo jogador.
- Inventário interno de 36 slots, com abertura pela tecla `E`.
- Movimentação de pilhas no inventário por clique esquerdo e unitária por clique direito.
- Tela de crafting 2 × 2 com receitas de vidro, tochas e picareta (2 areias = 1 vidro; 1 tronco + 1 folha = 4 tochas; 2 pedras + 1 tronco = 1 picareta).
- Para fabricar a picareta: abra `E`, entre em `C`, clique duas vezes em uma pilha de pedra e uma vez em um tronco; clique no resultado da picareta e selecione-a pela roda do mouse.
- Barra de vida e dano de queda acima de três blocos, com reaparecimento.
- Sistema de fome e saturação: o consumo aumenta ao andar/correr, a saturação é consumida primeiro, a fome causa dano quando chega a zero, pode ser recuperada com maçãs e permite regeneração gradual com fome alta.
- A corrida é bloqueada quando a fome está em nível crítico.
- Maçãs na hotbar: selecione o item e pressione `F` para comer.
- Folhas quebradas podem soltar maçãs, criando uma fonte renovável de alimento.
- Inimigos hostis: zumbi e slime, com perseguição, spawn noturno, desaparecimento
  ao amanhecer, animação simples, dano por proximidade, invulnerabilidade curta,
  barra de vida e drops. Os braços dos zumbis ficam à frente do corpo e balançam
  alternadamente durante a caminhada. Zumbis naturais comuns pegam fogo durante
  cerca de dez segundos antes de desaparecer ao amanhecer, enquanto continuam perseguindo
  e atacando o jogador; o fogo usa shader com chamas animadas e ampliadas e não
  é aplicado aos slimes.
- Extensão procedural com biomas de planície, deserto, tundra, floresta e cerejeiras, cavernas simples e minérios de carvão e ferro.
- Picareta e espada no inventário: minérios exigem picareta para serem extraídos; carvão e ferro viram drops coletáveis.
- Ferramentas possuem durabilidade de 100 usos, não se empilham e desaparecem quando quebram.
- Salvamento manual com `F6` e carregamento com `F7`, incluindo mundo, seed,
  posição, inventário, vida, fome, horário, chuva, zumbis, drops, animais e
  montaria. Saves v5 são expandidos e têm backup `.v5.bak`. Em saves v6,
  regiões idênticas à geração antiga recebem o novo terreno; chunks alterados
  e a área ocupada pelo jogador são preservados e o original fica em `.v6.bak`.
  A posição já salva do cavalo não é substituída. Salve com F6 após carregar.
- A seed pode ser definida ao iniciar com `mine.exe --seed 12345` ou `mine.exe --seed=12345`, permitindo reproduzir o mesmo mundo.
- Chuva alternável com `T` e partículas visuais ao quebrar blocos.
- Cristais luminosos coletáveis em campos de tundra, uma mecânica visual original do projeto; depois de minerados, podem ser recolocados como iluminação.
- Ciclo contínuo de dia e noite com shader de céu, sol, lua, estrelas cintilantes,
  nuvens móveis, cometa eventual e horário no HUD.
- Iluminação direcional dinâmica calculada pela posição do sol.
- Tochas emissoras no cenário e colocáveis pelo jogador, com iluminação artificial por distância reconstruída após cada alteração.
- Um menu principal com os botões Jogar e Sair.
- Um menu de pausa (ESC) com os botões Continuar, Instruções e Sair.
- Uma mira fixa no centro, barra de 13 itens com miniaturas tridimensionais dos blocos, quantidades e item selecionado; o HUD exibe FPS, seed, bioma, horário e posição.
- A mira usa travessia DDA 3D, visitando apenas as células realmente atravessadas pelo raio.
- Informações de FPS e posição do personagem no canto superior, sem alterar o título da janela.
- Uma tela de instruções acessível pelo menu de pausa, com retorno por ESC, Enter ou Espaço.

Os blocos e membros dos personagens são construídos a partir de cubos. O terreno usa um atlas pixelado; a água e o vidro são semitransparentes, as folhas e a vegetação usam recorte de alfa e o céu usa shader. As texturas das cerejeiras são derivadas do atlas em memória.

## Conteúdos por aula

| Semana | Conteúdo | Onde aparece no código |
|---|---|---|
| 1 | Janela, laço principal, tempo de quadro, FPS em tela, ESC | `main`, `desenha`, `atualiza` |
| 2 | Primitivas (`GL_QUADS`) e cor por face | `desenhaBloco`, `corDaFace` |
| 3 | Cubo 3D, buffer de profundidade, descarte de faces, instâncias com `glPushMatrix`/`glPopMatrix` | `desenhaBloco`, `blocoEm` |
| 4 | Perspectiva, correção de proporção, câmera com `gluLookAt`, navegação pelo teclado | `aplicaProjecao`, `redimensiona`, `posicionaCamera` |
| 5 | Teclado por estado e tempo de quadro; mouse com yaw e pitch e captura do cursor; matriz do mundo e colisão por caixa alinhada aos eixos; primeira e terceira pessoa; zoom pela roda | `teclado`, `tecladoSolto`, `atualiza`, `mundo`, `posicaoLivre`, `moveJogador`, `direcaoDoOlhar`, `movimentoMouse`, `posicionaCamera`, `desenhaJogador`, `rodaMouse` |
| Texturas e transparência | Atlas lido de um PPM e enviado com `glTexImage2D`; células por face e `glTexCoord2f`; filtro `GL_NEAREST` (visual pixelado) e `GL_MODULATE`; alfa por cor-chave; recorte com `GL_ALPHA_TEST` nas folhas; mistura com `GL_BLEND` e `glDepthMask` no vidro e na água, em três passadas, com os semitransparentes ordenados; faces ocultas por vizinho opaco | `lerPPM`, `converteRGBA`, `carregaAtlas`, `celulaDaFace`, `coordenadasDaCelula`, `desenhaBloco`, `faceVisivel`, `facesVisiveis`, `desenhaOpacos`, `listaTransparentes`, `desenhaTransparentes`, `casinhaEm` |
| Interface e estados | Passada 2D com as duas matrizes salvas; projeção em pixels; menu principal, jogo, pausa e instruções; cursor visível nos menus e capturado no jogo; botões com hover e clique; mira, barra de 13 itens com miniaturas 3D, quantidades, seleção numérica/roda, FPS e posição | `iniciaInterface`, `terminaInterface`, `desenhaInterface`, `desenhaInstrucoes`, `movimentoInterface`, `cliqueMouse`, `estadoJogo`, `botaoSelecionado` |

## Dependências

- Compilador C (`gcc`)
- OpenGL, GLU, GLUT (freeglut) e GLEW

É preciso usar a **freeglut**, não outra GLUT: a roda do mouse usa `glutMouseWheelFunc`, que é extensão da freeglut.

**Ubuntu / Debian**
```bash
sudo apt install build-essential freeglut3-dev libglew-dev
```

**Arch / Manjaro**
```bash
sudo pacman -S gcc freeglut glew glu
```

**Windows**: use o MinGW com as bibliotecas freeglut e GLEW instaladas.

**macOS**: não precisa instalar nada; o código usa a GLUT e o OpenGL que já vêm no sistema (blocos `#ifdef __APPLE__`). Essa GLUT não tem a roda do mouse, então no Mac o zoom é só por `+`/`-`. O olhar pelo mouse também é tratado à parte: a GLUT da Apple trava o mouse por 0,25 s a cada reposicionamento do cursor, então no Mac o cursor é desacoplado do mouse e o deslocamento é lido direto do sistema (por isso o `-framework ApplicationServices`).

## Como compilar e rodar

O projeto também possui `CMakeLists.txt` e `Makefile` para reproduzir a
compilação da versão principal e das quatro demonstrações. A compilação
manual abaixo continua válida e é útil para conferir as bibliotecas da
plataforma.

Com CMake, a partir da pasta do projeto:

```bash
cmake -S . -B build
cmake --build build
```

Os executáveis das demonstrações ficam em `build/demonstracao/`.

**Linux**
```bash
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -o mine -lGLEW -lGL -lGLU -lglut -lm
./mine
```

**Windows (MinGW)**
```bash
windres mine.rc -O coff -o mine.res.o
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c mine.res.o -o mine.exe -lglew32 -lfreeglut -lopengl32 -lglu32 -lwinmm
mine.exe
```

Teste independente de geração procedural e chunks:

```bash
make test-worldgen
```

Teste de pilhas, ferramentas, durabilidade e cristal:

```bash
make test-inventory
```

Para executar toda a suíte independente da janela:

```bash
make test
```

**macOS**
```bash
clang mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -o mine -framework OpenGL -framework GLUT -framework ApplicationServices -lm
./mine
```

Rode o jogo **de dentro da pasta do projeto**: ele lê o `atlas.ppm` da pasta atual e, se não achar o arquivo, avisa no terminal e fecha.

### Linux/macOS

Use as mesmas macros e substitua o comando de compilação pelas bibliotecas da
plataforma:

```sh
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_MATRIX_ONLY=1 -o demonstracao/mine-matriz -lGLEW -lGL -lGLU -lglut -lm
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_FIXED_BAR=1 -o demonstracao/mine-barra -lGLEW -lGL -lGLU -lglut -lm
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_RAW_MOUSE_Y=1 -o demonstracao/mine-mouse -lGLEW -lGL -lGLU -lglut -lm
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_PAUSE_CLOCK=1 -o demonstracao/mine-pausa -lGLEW -lGL -lGLU -lglut -lm
```

Depois, execute o binário correspondente em `demonstracao/`. Para mostrar a
versão correta, compile sem nenhuma das quatro macros.

## Atlas de texturas

As faces texturizadas dos blocos usam `atlas.ppm`: 64 × 64 pixels divididos em células de 16 × 16, quatro por linha. As células de cerejeira são produzidas em memória a partir das texturas existentes; a vegetação baixa tem uma textura própria gerada pelo programa.

Para mudar uma textura, edite a imagem num editor qualquer (por exemplo, salvando como PNG) e converta de volta para PPM binário com o ImageMagick:

```bash
magick atlas.png atlas.ppm
```

O leitor do jogo não aceita linhas de comentário (`#`) no cabeçalho do PPM. Alguns editores, como o GIMP, gravam uma; por isso a conversão final pelo ImageMagick.

| Célula | Aparência | | Célula | Aparência |
|---|---|---|---|---|
| 0 | grama, topo | | 6 | folha (vazados em magenta) |
| 1 | grama, lateral | | 7 | vidro |
| 2 | terra | | 8 | água |
| 3 | pedra | | 9 | areia |
| 4 | anéis da madeira | | 10 | neve |
| 5 | casca | | 12 e 13 | folhas e casca de cerejeira, geradas em memória |

O formato PPM (`P6`) não tem canal alfa. Na leitura, o jogo transforma o magenta (255, 0, 255) em transparente e dá opacidade parcial às células de vidro (0,4) e de água (0,6).

## Controles

| Entrada | Ação | Leitura |
|---|---|---|
| W / S | Andar para frente / para trás | estado |
| A / D | Andar para a esquerda / para a direita | estado |
| R (segurando) | Correr (dobra a velocidade) | estado |
| ← / → | Girar o olhar | estado |
| Espaço ou ↑ | Saltar | estado |
| F | Comer a maçã selecionada e recuperar fome | evento |
| G | Atacar o inimigo à frente | evento |
| M | Montar ou desmontar perto do cavalo | evento |
| F6 | Salvar o mundo em `savegame.dat` | evento |
| F7 | Carregar o mundo salvo | evento |
| T | Alternar chuva | evento |
| Mouse | Direção do olhar (já começa capturado) | evento de movimento |
| Clique esquerdo | Quebrar o bloco observado | evento |
| Clique direito | Colocar o bloco; olhando para baixo, construir sob os pés e subir | evento |
| E | Abrir ou fechar o inventário | evento |
| C (inventário) | Abrir a tela de crafting | evento |
| Roda | Selecionar o próximo/anterior item | evento |
| + / - | Aproximar / afastar (campo de visão) | evento |
| Tab | Liberar / capturar o cursor (clicar na janela também captura) | evento |
| F5 ou V | Alternar primeira / terceira pessoa | evento |
| P | Alternar perspectiva / ortográfica | evento |
| ESC | Abrir / fechar a pausa ou voltar das instruções | evento |
| 1 a 9 | Selecionar espaço da barra de itens | evento |
| 0 | Selecionar a décima posição da barra (maçã) | evento |
| Q (em um menu) | Sair | evento |

A coluna "Leitura" diz como cada entrada é tratada: ações contínuas, que duram enquanto a tecla está pressionada, são lidas **por estado** a cada quadro; ações discretas, que acontecem uma vez por toque, são tratadas **por evento**.

O jogo começa no menu principal, com o cursor livre. Ao clicar em Jogar, o personagem aparece no gramado, olhando para a montanha, com a câmera em terceira pessoa e o cursor capturado. Há gravidade e o personagem salta com Espaço ou ↑. Durante o jogo, o mouse controla o olhar; Tab libera o cursor, e Tab ou um clique na janela o capturam de novo. ESC abre a pausa, que congela a atualização do jogo; nela, **INSTRUÇÕES** mostra todos os comandos e **CONTINUAR** retoma a partida.

## Como o cenário é montado

Os eixos do mundo são:

- **x**: da esquerda (0) para a direita (63)
- **y**: altura, a partir de 0, crescendo para cima
- **z**: do fundo do mapa (0) para a frente (63), onde fica o mar

### O mapa

O terreno vem da matriz `mapa`, que mostra o mundo **visto de cima**. Cada linha é um valor de `z`, cada número na linha é um valor de `x`, e o número em si é a **altura** daquela coluna de blocos:

| Valor | Significado |
|---|---|
| 0 | Mar (areia no fundo, água por cima) |
| 1 | Praia |
| 2 ou mais | Terra firme com grama no topo |
| 6 ou mais | Topo coberto de neve (`NIVEL_NEVE`) |

O acesso é `mapa[z][x]`, com a linha antes da coluna. Assim a tabela aparece no código do mesmo jeito que o mundo visto de cima.

### Do mapa aos blocos

1. `construirMundo`, chamada no início e na migração de saves antigos, percorre os 128 × 128 quadrados do mapa e chama `colunaEm(x, z)` para cada um.
2. `colunaEm` lê a altura `h` no mapa e empilha blocos de `y = 0` até o topo. Para cada andar, uma sequência de `if` escolhe o tipo comparando `y` com `h`:
   - acima do chão e até o nível da água: **água**
   - coluna baixa: **areia**
   - topo de coluna alta: **neve**
   - topo: **grama**
   - dois andares abaixo do topo: **terra**
   - o resto: **pedra**
3. Cada bloco é gravado com `colocaBloco(tipo, x, y, z)` na matriz `mundo`.
4. As árvores, cerejeiras e casas são montadas com `colocaBloco`; as nuvens são geradas pelo shader do céu.
5. A cada quadro, a matriz é percorrida e `blocoEm(tipo, x, y, z)` é chamada para cada posição ocupada: ela desloca a origem com `glTranslatef` e desenha ali o cubo de sempre, só com as faces que nenhum vizinho opaco esconde (`facesVisiveis`).

### Desenho em três passadas

A mistura do vidro e da água usa a cor que já está na tela, então a ordem de desenho importa:

1. **Opacos** (terra, pedra, grama, tronco, areia e neve), em `desenhaOpacos`.
2. **Recorte**: folhas e vegetação, com o teste de alfa ligado; os vazados são descartados. Depois vêm os personagens e animais.
3. **Semitransparentes**: vidro e água, em `desenhaTransparentes`, ordenados do mais distante ao mais próximo da câmera, com a mistura ligada e a escrita de profundidade desligada.

### A matriz do mundo

`mundo[x][y][z]` guarda o tipo de cada bloco (ou `VAZIO`) numa grade de 64 × 16 × 64. Os primeiros 16 × 16 quadrados usam a matriz didática original; a extensão usa a seed `sementeMundo` e uma função procedural determinística. **Atenção à ordem**: o `mapa` é `[z][x]`, para parecer o mundo visto de cima, mas o `mundo` segue a ordem dos eixos, `[x][y][z]`. Todo acesso passa por `colocaBloco` e `blocoNoMundo`, que também testam os limites.

A matriz é o que permite a colisão: a célula que contém um ponto `(px, py, pz)` é `(floor(px), floor(py), floor(pz))`, então saber se o personagem bate num bloco é consultar as poucas células que a caixa dele ocupa (`posicaoLivre`). Fora do mundo conta como sólido, e a água não é sólida.

## Experimente

- **Mudar o terreno**: edite os números da matriz `mapa`. Um `0` no meio do gramado vira um lago; um `9` vira uma torre.
- **Mais ou menos neve**: altere `NIVEL_NEVE`.
- **Outra árvore**: acrescente `arvoreEm(x, z);` em `construirMundo`, num quadrado de grama.
- **Nuvens**: ajuste os parâmetros procedurais no shader do céu em `inicializaShaderCeu`.
- **Um tipo de bloco novo**: crie um `#define` (por exemplo, `#define MADEIRA 20`; de 10 a 16 são as cores do personagem e 17 é o vidro), desenhe a célula no `atlas.ppm` (há 5 livres) e acrescente o `case` em `celulaDaFace`. Para cor sólida, sem textura, o `case` vai em `corDaFace`.
- **Quebrar um bloco**: `colocaBloco(VAZIO, x, y, z)` depois de `construirMundo`; o desenho e a colisão passam a ver o buraco.


## Demonstrações dos erros da Semana 10

As quatro demonstrações são modos temporários de compilação. Sem nenhuma macro, o código permanece na versão correta. Execute os comandos a partir da pasta do projeto, onde está o `atlas.ppm`.

### Windows MinGW

```powershell
# 1. Guardar somente GL_PROJECTION: a mira acompanha a câmera
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_MATRIX_ONLY=1 -o demonstracao\mine-matriz.exe -lglew32 -lfreeglut -lopengl32 -lglu32
.\demonstracao\mine-matriz.exe

# 2. Barra em posição fixa: ela sai do centro ao redimensionar
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_FIXED_BAR=1 -o demonstracao\mine-barra.exe -lglew32 -lfreeglut -lopengl32 -lglu32
.\demonstracao\mine-barra.exe

# 3. Mouse sem converter a altura: clique e hover ficam espelhados
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_RAW_MOUSE_Y=1 -o demonstracao\mine-mouse.exe -lglew32 -lfreeglut -lopengl32 -lglu32
.\demonstracao\mine-mouse.exe

# 4. Pausa sem reiniciar o tempo: o personagem avança de uma vez ao voltar
gcc mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c -DDEMO_UI_PAUSE_CLOCK=1 -o demonstracao\mine-pausa.exe -lglew32 -lfreeglut -lopengl32 -lglu32
.\demonstracao\mine-pausa.exe
```

**Para a primeira demonstração**, clique em **JOGAR**, alterne para primeira pessoa com `V` ou `F5` e gire a câmera. A mira deverá sair do centro.

**Para a segunda**, clique em **JOGAR** e redimensione a janela para uma largura maior e depois menor. A barra permanecerá na posição inicial.

**Para a terceira**, permaneça no menu, mova o cursor sobre **JOGAR** e tente clicar. O destaque e o clique serão avaliados na posição verticalmente espelhada; clique nessa posição para revelar o erro.

**Para a quarta**, durante o jogo mantenha `W` pressionado, pressione `ESC`,
aguarde alguns segundos e pressione `ESC` novamente sem soltar `W`. O personagem avançará de uma só vez porque o tempo da pausa foi usado como um único quadro. O relógio é reiniciado normalmente ao sair do menu principal; o defeito é demonstrado somente no retorno da pausa.
