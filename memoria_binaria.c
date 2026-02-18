#include "memoria_binaria.h"
#include <string.h>

// Calcula tamanho fixo do nó no disco para usar no fseek
long int tamanho_no_bytes(int ordem) {
    return sizeof(int) * 2          
         + sizeof(long)             
         + sizeof(int) * (ordem - 1)
         + sizeof(int) * (ordem - 1)
         + sizeof(long) * (ordem);    
}

No* criar_no(int ordem) {
    No *no = (No*) malloc(sizeof(No));
    // Alloca espaco extra para operacao de merge
    int max_keys_for_merge = 2 * ordem - 1;
    no->chaves = (int*) calloc(max_keys_for_merge, sizeof(int));
    no->dados  = (int*) calloc(max_keys_for_merge, sizeof(int));
    no->filhos = (long*) malloc(sizeof(long) * (max_keys_for_merge + 1));
    // Inicializa todos filhos com -1
    for (int i = 0; i <= max_keys_for_merge; i++) {
        no->filhos[i] = -1;
    }
    no->num_chaves = 0;
    no->folha = 1;
    no->posicao = -1;
    return no;
}

void liberar_no(No *no) {
    if (no) {
        if (no->chaves) free(no->chaves);
        if (no->dados)  free(no->dados);
        if (no->filhos) free(no->filhos);
        free(no);
    }
}

void le_no(FILE *arq, No *no, long int pos, int ordem) {
    if (pos == -1) return;
    fseek(arq, pos, SEEK_SET);
    fread(&no->num_chaves, sizeof(int), 1, arq);
    fread(&no->folha, sizeof(int), 1, arq);
    fread(&no->posicao, sizeof(long), 1, arq);
    fread(no->chaves, sizeof(int), ordem - 1, arq);
    fread(no->dados,  sizeof(int), ordem - 1, arq);
    fread(no->filhos, sizeof(long), ordem, arq);
}

void escreve_no(FILE *arq, No *no, long int pos, int ordem) {
    if (pos == -1) return;
    fseek(arq, pos, SEEK_SET);
    fwrite(&no->num_chaves, sizeof(int), 1, arq);
    fwrite(&no->folha, sizeof(int), 1, arq);
    fwrite(&no->posicao, sizeof(long), 1, arq);
    fwrite(no->chaves, sizeof(int), ordem - 1, arq);
    fwrite(no->dados,  sizeof(int), ordem - 1, arq);
    fwrite(no->filhos, sizeof(long), ordem, arq);
}

void le_cabecalho(FILE *arq, Cabecalho *cab) {
    fseek(arq, 0, SEEK_SET);
    fread(cab, sizeof(Cabecalho), 1, arq);
}

void escreve_cabecalho(FILE *arq, Cabecalho *cab) {
    fseek(arq, 0, SEEK_SET);
    fwrite(cab, sizeof(Cabecalho), 1, arq);
}

long int alocar_novo_no(FILE *arq, Cabecalho *cab) {
    long int tam = tamanho_no_bytes(cab->ordem);
    // Pula o cabeçalho + (número de nós já existentes * tamanho do nó)
    long int pos = sizeof(Cabecalho) + (cab->total_nos * tam);
    cab->total_nos++;
    escreve_cabecalho(arq, cab); // Atualiza contador no disco
    return pos;
}

void inicializar_arquivo(FILE *arq, int ordem) {
    Cabecalho cab;
    memset(&cab, 0, sizeof(Cabecalho));  // Inicializa os bytes para evitar erro de valgrind
    cab.ordem = ordem;
    cab.raiz_pos = -1;
    cab.total_nos = 0;
    escreve_cabecalho(arq, &cab);
}