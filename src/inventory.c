#include "../include/inventory.h"

#ifndef VAZIO
#define VAZIO 0
#endif
#ifndef MINERIO_CARVAO
#define MINERIO_CARVAO 20
#endif
#ifndef MINERIO_FERRO
#define MINERIO_FERRO 21
#endif
#ifndef PICARETA
#define PICARETA 22
#endif
#ifndef ESPADA
#define ESPADA 23
#endif
#ifndef CRISTAL
#define CRISTAL 24
#endif
#define DURABILIDADE_FERRAMENTA 100

static int ehFerramenta(int tipo)
{
    return tipo == PICARETA || tipo == ESPADA;
}

int indiceDoItem(int tipo)
{
    int i;
    for (i = 0; i < NUM_ITENS; i++)
        if (tiposItens[i] == tipo) return i;
    if (tipo == MINERIO_CARVAO || tipo == MINERIO_FERRO || tipo == CRISTAL)
        return NUM_ITENS; /* coletável, mas reservado fora da hotbar */
    return -1;
}

void sincronizaHotbar(void)
{
    int i;

    /* A hotbar é organizada por tipo de item, enquanto o inventário é
       organizado por slots físicos. Somar por tipo evita que uma segunda
       picareta ou um drop coletado em um slot posterior desapareça da
       barra ou apareça com o ícone errado. */
    for (i = 0; i < NUM_ITENS; i++) quantidadesItens[i] = 0;
    for (i = 0; i < INVENTORY_SLOTS; i++) {
        int indice = indiceDoItem(inventario.slots[i].tipo);
        if (indice >= 0 && indice < NUM_ITENS)
            quantidadesItens[indice] += inventario.slots[i].quantidade;
    }
}

int adicionaAoInventario(int tipo, int quantidade)
{
    int i, restante = quantidade;

    for (i = 0; i < INVENTORY_SLOTS && restante > 0; i++) {
        ItemStack *slot = &inventario.slots[i];
        if (!ehFerramenta(tipo) && slot->tipo == tipo &&
            slot->quantidade < MAX_ITEM_STACK) {
            int espaco = MAX_ITEM_STACK - slot->quantidade;
            int adicionado = restante < espaco ? restante : espaco;
            slot->quantidade += adicionado;
            restante -= adicionado;
        }
    }

    for (i = 0; i < INVENTORY_SLOTS && restante > 0; i++) {
        ItemStack *slot = &inventario.slots[i];
        if (slot->quantidade == 0) {
            int limite = ehFerramenta(tipo) ? 1 : MAX_ITEM_STACK;
            int adicionado = restante < limite ? restante : limite;
            slot->tipo = tipo;
            slot->quantidade = adicionado;
            slot->durabilidade = ehFerramenta(tipo) ? DURABILIDADE_FERRAMENTA : 0;
            restante -= adicionado;
        }
    }

    sincronizaHotbar();
    return quantidade - restante;
}

int removeDoInventario(int tipo, int quantidade)
{
    int i, disponivel = 0;

    for (i = 0; i < INVENTORY_SLOTS; i++)
        if (inventario.slots[i].tipo == tipo)
            disponivel += inventario.slots[i].quantidade;
    if (disponivel < quantidade) return 0;

    for (i = 0; i < INVENTORY_SLOTS && quantidade > 0; i++) {
        ItemStack *slot = &inventario.slots[i];
        if (slot->tipo == tipo && slot->quantidade > 0) {
            int removido = quantidade < slot->quantidade
                         ? quantidade : slot->quantidade;
            slot->quantidade -= removido;
            quantidade -= removido;
            if (slot->quantidade == 0) {
                slot->tipo = VAZIO;
                slot->durabilidade = 0;
            }
        }
    }

    sincronizaHotbar();
    return 1;
}

int durabilidadeDoItem(int tipo)
{
    int i;
    for (i = 0; i < INVENTORY_SLOTS; i++)
        if (inventario.slots[i].tipo == tipo &&
            inventario.slots[i].quantidade > 0)
            return inventario.slots[i].durabilidade;
    return 0;
}

int gastaDurabilidade(int tipo)
{
    int i;
    if (!ehFerramenta(tipo)) return 1;
    for (i = 0; i < INVENTORY_SLOTS; i++)
        if (inventario.slots[i].tipo == tipo &&
            inventario.slots[i].quantidade > 0) {
            inventario.slots[i].durabilidade--;
            if (inventario.slots[i].durabilidade <= 0) {
                inventario.slots[i].tipo = VAZIO;
                inventario.slots[i].quantidade = 0;
                inventario.slots[i].durabilidade = 0;
            }
            sincronizaHotbar();
            return 1;
        }
    return 0;
}
