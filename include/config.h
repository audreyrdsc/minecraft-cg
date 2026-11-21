#ifndef MINECRAFT_CG_CONFIG_H
#define MINECRAFT_CG_CONFIG_H

/* Dimensões estruturais compartilhadas por todos os módulos. */
#define LADO 128
#define ALTURA_MUNDO 16
#define CHUNK_LADO 16
#define NUM_CHUNKS_X ((LADO + CHUNK_LADO - 1) / CHUNK_LADO)
#define NUM_CHUNKS_Z ((LADO + CHUNK_LADO - 1) / CHUNK_LADO)

#endif
