CC ?= gcc

COMMON_SOURCE = mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c
UNIX_LIBS = -lGLEW -lGL -lGLU -lglut -lm
WINDOWS_LIBS = -lglew32 -lfreeglut -lopengl32 -lglu32

.PHONY: all clean demos syntax test test-worldgen test-inventory test-crafting

all: mine

mine: $(COMMON_SOURCE)
	$(CC) $(COMMON_SOURCE) -o mine $(UNIX_LIBS)

demos: mine-matriz mine-barra mine-mouse mine-pausa

mine-matriz: $(COMMON_SOURCE)
	$(CC) $(COMMON_SOURCE) -DDEMO_UI_MATRIX_ONLY=1 -o demonstracao/mine-matriz $(UNIX_LIBS)

mine-barra: $(COMMON_SOURCE)
	$(CC) $(COMMON_SOURCE) -DDEMO_UI_FIXED_BAR=1 -o demonstracao/mine-barra $(UNIX_LIBS)

mine-mouse: $(COMMON_SOURCE)
	$(CC) $(COMMON_SOURCE) -DDEMO_UI_RAW_MOUSE_Y=1 -o demonstracao/mine-mouse $(UNIX_LIBS)

mine-pausa: $(COMMON_SOURCE)
	$(CC) $(COMMON_SOURCE) -DDEMO_UI_PAUSE_CLOCK=1 -o demonstracao/mine-pausa $(UNIX_LIBS)

syntax:
	$(CC) -fsyntax-only $(COMMON_SOURCE)

test-worldgen:
	$(CC) tests/worldgen_chunks_test.c src/worldgen.c src/chunks.c src/world.c -o tests/worldgen_chunks_test -lm
	./tests/worldgen_chunks_test

test-inventory:
	$(CC) tests/inventory_test.c src/inventory.c -o tests/inventory_test
	./tests/inventory_test

test-crafting:
	$(CC) tests/crafting_test.c src/crafting.c -o tests/crafting_test
	./tests/crafting_test

test: test-worldgen test-inventory test-crafting

clean:
	$(RM) mine mine.exe
	$(RM) demonstracao/mine-matriz demonstracao/mine-barra demonstracao/mine-mouse demonstracao/mine-pausa
