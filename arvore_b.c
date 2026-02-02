#include "arvore_b.h"
#include <string.h>
#include <math.h>

// ============================================================================
// 1. GERENCIAMENTO DE MEMÓRIA E CÁLCULOS
// ============================================================================

long int tamanho_no_bytes(int ordem) {
    return sizeof(int) * 2          
         + sizeof(long)             
         + sizeof(int) * (ordem - 1)  // chaves
         + sizeof(int) * (ordem - 1)  // dados (NOVO)
         + sizeof(long) * (ordem);    
}

No* criar_no(int ordem) {
    No *no = (No*) malloc(sizeof(No));
    no->chaves = (int*) malloc(sizeof(int) * (ordem - 1));
    no->dados  = (int*) malloc(sizeof(int) * (ordem - 1)); // Aloca dados
    no->filhos = (long*) malloc(sizeof(long) * (ordem));
    return no;
}

void liberar_no(No *no) {
    if (no) {
        if (no->chaves) free(no->chaves);
        if (no->dados)  free(no->dados); // Libera dados
        if (no->filhos) free(no->filhos);
        free(no);
    }
}

// ============================================================================
// 2. DISCO (Leitura/Escrita Serializada)
// ============================================================================

void le_no(FILE *arq, No *no, long int pos, int ordem) {
    if (pos == -1) return;
    fseek(arq, pos, SEEK_SET);
    
    fread(&no->num_chaves, sizeof(int), 1, arq);
    fread(&no->folha, sizeof(int), 1, arq);
    fread(&no->posicao, sizeof(long), 1, arq);
    
    fread(no->chaves, sizeof(int), ordem - 1, arq);
    fread(no->dados,  sizeof(int), ordem - 1, arq); // Lê dados
    fread(no->filhos, sizeof(long), ordem, arq);
}

void escreve_no(FILE *arq, No *no, long int pos, int ordem) {
    if (pos == -1) return;
    fseek(arq, pos, SEEK_SET);
    
    fwrite(&no->num_chaves, sizeof(int), 1, arq);
    fwrite(&no->folha, sizeof(int), 1, arq);
    fwrite(&no->posicao, sizeof(long), 1, arq);
    
    fwrite(no->chaves, sizeof(int), ordem - 1, arq);
    fwrite(no->dados,  sizeof(int), ordem - 1, arq); // Grava dados
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
    long int pos = sizeof(Cabecalho) + (cab->total_nos * tam);
    cab->total_nos++;
    escreve_cabecalho(arq, cab);
    return pos;
}

void inicializar_arvore(FILE *arq, int ordem) {
    Cabecalho cab;
    cab.ordem = ordem;
    cab.raiz_pos = -1;
    cab.total_nos = 0;
    escreve_cabecalho(arq, &cab);
}

// ============================================================================
// 3. INSERÇÃO
// ============================================================================

void split_child(FILE *arq, No *x, int i, No *y, Cabecalho *cab) {
    No *z = criar_no(cab->ordem);
    z->folha = y->folha;
    z->posicao = alocar_novo_no(arq, cab);

    int meio = (cab->ordem - 1) / 2;
    z->num_chaves = y->num_chaves - meio - 1;

    // Copia chaves E DADOS para Z
    for (int j = 0; j < z->num_chaves; j++) {
        z->chaves[j] = y->chaves[j + meio + 1];
        z->dados[j]  = y->dados[j + meio + 1]; // Move dado
    }

    if (!y->folha) {
        for (int j = 0; j <= z->num_chaves; j++) {
            z->filhos[j] = y->filhos[j + meio + 1];
        }
    }

    y->num_chaves = meio;

    // Abre espaço no pai X
    for (int j = x->num_chaves; j >= i + 1; j--) {
        x->filhos[j + 1] = x->filhos[j];
    }
    x->filhos[i + 1] = z->posicao;

    for (int j = x->num_chaves - 1; j >= i; j--) {
        x->chaves[j + 1] = x->chaves[j];
        x->dados[j + 1]  = x->dados[j]; // Move dado
    }
    
    // Sobe a mediana
    x->chaves[i] = y->chaves[meio];
    x->dados[i]  = y->dados[meio];      // Sobe dado da mediana
    x->num_chaves++;

    escreve_no(arq, y, y->posicao, cab->ordem);
    escreve_no(arq, z, z->posicao, cab->ordem);
    escreve_no(arq, x, x->posicao, cab->ordem);

    liberar_no(z);
}

void inserir_nao_cheio(FILE *arq, No *x, int k, int d, Cabecalho *cab) {
    int i = x->num_chaves - 1;

    if (x->folha) {
        while (i >= 0 && k < x->chaves[i]) {
            x->chaves[i + 1] = x->chaves[i];
            x->dados[i + 1]  = x->dados[i]; // Move dado
            i--;
        }
        x->chaves[i + 1] = k;
        x->dados[i + 1]  = d; // Insere dado
        x->num_chaves++;
        escreve_no(arq, x, x->posicao, cab->ordem);
    } else {
        while (i >= 0 && k < x->chaves[i]) i--;
        i++;
        
        No *filho = criar_no(cab->ordem);
        le_no(arq, filho, x->filhos[i], cab->ordem);

        if (filho->num_chaves == cab->ordem - 1) {
            split_child(arq, x, i, filho, cab);
            if (k > x->chaves[i]) {
                i++;
                le_no(arq, filho, x->filhos[i], cab->ordem);
            }
        }
        inserir_nao_cheio(arq, filho, k, d, cab);
        liberar_no(filho);
    }
}

void inserir(FILE *arq, int chave, int dado) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);

    if (cab.raiz_pos == -1) {
        No *raiz = criar_no(cab.ordem);
        raiz->folha = 1;
        raiz->num_chaves = 1;
        raiz->chaves[0] = chave;
        raiz->dados[0]  = dado; // Insere dado na raiz
        raiz->posicao = alocar_novo_no(arq, &cab);
        
        for(int i=0; i<cab.ordem; i++) raiz->filhos[i] = -1;

        cab.raiz_pos = raiz->posicao;
        escreve_cabecalho(arq, &cab);
        escreve_no(arq, raiz, raiz->posicao, cab.ordem);
        liberar_no(raiz);
    } else {
        No *raiz = criar_no(cab.ordem);
        le_no(arq, raiz, cab.raiz_pos, cab.ordem);

        if (raiz->num_chaves == cab.ordem - 1) {
            No *nova_raiz = criar_no(cab.ordem);
            nova_raiz->folha = 0;
            nova_raiz->num_chaves = 0;
            nova_raiz->posicao = alocar_novo_no(arq, &cab);
            nova_raiz->filhos[0] = cab.raiz_pos;
            for(int i=1; i<cab.ordem; i++) nova_raiz->filhos[i] = -1;

            cab.raiz_pos = nova_raiz->posicao;
            escreve_cabecalho(arq, &cab);
            escreve_no(arq, nova_raiz, nova_raiz->posicao, cab.ordem);

            split_child(arq, nova_raiz, 0, raiz, &cab);
            inserir_nao_cheio(arq, nova_raiz, chave, dado, &cab);
            
            liberar_no(nova_raiz);
        } else {
            inserir_nao_cheio(arq, raiz, chave, dado, &cab);
        }
        liberar_no(raiz);
    }
}

// ============================================================================
// 4. REMOÇÃO
// ============================================================================

// MODIFICADO: Agora retorna chave e dado por referência
void pegar_antecessor(FILE *arq, long int pos_no, int ordem, int *chave_out, int *dado_out) {
    No *atual = criar_no(ordem);
    le_no(arq, atual, pos_no, ordem);
    while (!atual->folha) {
        long int prox = atual->filhos[atual->num_chaves];
        le_no(arq, atual, prox, ordem);
    }
    *chave_out = atual->chaves[atual->num_chaves - 1];
    *dado_out  = atual->dados[atual->num_chaves - 1]; // Pega o dado
    liberar_no(atual);
}

void pegar_sucessor(FILE *arq, long int pos_no, int ordem, int *chave_out, int *dado_out) {
    No *atual = criar_no(ordem);
    le_no(arq, atual, pos_no, ordem);
    while (!atual->folha) {
        long int prox = atual->filhos[0];
        le_no(arq, atual, prox, ordem);
    }
    *chave_out = atual->chaves[0];
    *dado_out  = atual->dados[0]; // Pega o dado
    liberar_no(atual);
}

void merge_nodes(FILE *arq, No *pai, int idx, Cabecalho *cab) {
    No *esquerda = criar_no(cab->ordem);
    No *direita = criar_no(cab->ordem);
    long int pos_esq = pai->filhos[idx];
    long int pos_dir = pai->filhos[idx+1];
    
    le_no(arq, esquerda, pos_esq, cab->ordem);
    le_no(arq, direita, pos_dir, cab->ordem);

    // Desce a mediana (chave + dado)
    esquerda->chaves[esquerda->num_chaves] = pai->chaves[idx];
    esquerda->dados[esquerda->num_chaves]  = pai->dados[idx];
    esquerda->num_chaves++;

    // Copia chaves e dados da direita
    for (int i = 0; i < direita->num_chaves; i++) {
        esquerda->chaves[esquerda->num_chaves + i] = direita->chaves[i];
        esquerda->dados[esquerda->num_chaves + i]  = direita->dados[i];
    }

    if (!esquerda->folha) {
        for (int i = 0; i <= direita->num_chaves; i++) {
            esquerda->filhos[esquerda->num_chaves + i] = direita->filhos[i];
        }
    }
    esquerda->num_chaves += direita->num_chaves;

    // Remove do pai
    for (int i = idx; i < pai->num_chaves - 1; i++) {
        pai->chaves[i] = pai->chaves[i+1];
        pai->dados[i]  = pai->dados[i+1];
        pai->filhos[i+1] = pai->filhos[i+2];
    }
    pai->num_chaves--;

    escreve_no(arq, esquerda, pos_esq, cab->ordem);
    escreve_no(arq, pai, pai->posicao, cab->ordem);

    liberar_no(esquerda);
    liberar_no(direita);
}

void borrow_from_prev(FILE *arq, No *pai, int idx, No *filho, int ordem) {
    No *irmao = criar_no(ordem);
    le_no(arq, irmao, pai->filhos[idx-1], ordem);

    // Abre espaço no filho
    for (int i = filho->num_chaves - 1; i >= 0; i--) {
        filho->chaves[i+1] = filho->chaves[i];
        filho->dados[i+1]  = filho->dados[i];
    }
    if (!filho->folha) {
        for (int i = filho->num_chaves; i >= 0; i--) {
            filho->filhos[i+1] = filho->filhos[i];
        }
    }

    // Pai desce para filho (chave + dado)
    filho->chaves[0] = pai->chaves[idx-1];
    filho->dados[0]  = pai->dados[idx-1];
    if (!filho->folha) filho->filhos[0] = irmao->filhos[irmao->num_chaves];
    filho->num_chaves++;

    // Irmão sobe para pai (chave + dado)
    pai->chaves[idx-1] = irmao->chaves[irmao->num_chaves-1];
    pai->dados[idx-1]  = irmao->dados[irmao->num_chaves-1];
    irmao->num_chaves--;

    escreve_no(arq, pai, pai->posicao, ordem);
    escreve_no(arq, filho, filho->posicao, ordem);
    escreve_no(arq, irmao, irmao->posicao, ordem);
    liberar_no(irmao);
}

void borrow_from_next(FILE *arq, No *pai, int idx, No *filho, int ordem) {
    No *irmao = criar_no(ordem);
    le_no(arq, irmao, pai->filhos[idx+1], ordem);

    // Pai desce para final do filho
    filho->chaves[filho->num_chaves] = pai->chaves[idx];
    filho->dados[filho->num_chaves]  = pai->dados[idx];
    if (!filho->folha) filho->filhos[filho->num_chaves+1] = irmao->filhos[0];
    filho->num_chaves++;

    // Irmão sobe para pai
    pai->chaves[idx] = irmao->chaves[0];
    pai->dados[idx]  = irmao->dados[0];

    // Ajusta irmão
    for (int i = 1; i < irmao->num_chaves; i++) {
        irmao->chaves[i-1] = irmao->chaves[i];
        irmao->dados[i-1]  = irmao->dados[i];
    }
    if (!irmao->folha) {
        for (int i = 1; i <= irmao->num_chaves; i++) {
            irmao->filhos[i-1] = irmao->filhos[i];
        }
    }
    irmao->num_chaves--;

    escreve_no(arq, pai, pai->posicao, ordem);
    escreve_no(arq, filho, filho->posicao, ordem);
    escreve_no(arq, irmao, irmao->posicao, ordem);
    liberar_no(irmao);
}

void fill_child(FILE *arq, No *pai, int idx, int t, Cabecalho *cab) {
    if (idx > 0) {
        No *esq = criar_no(cab->ordem);
        le_no(arq, esq, pai->filhos[idx-1], cab->ordem);
        if (esq->num_chaves >= t) {
            No *filho = criar_no(cab->ordem);
            le_no(arq, filho, pai->filhos[idx], cab->ordem);
            borrow_from_prev(arq, pai, idx, filho, cab->ordem);
            liberar_no(esq);
            liberar_no(filho);
            return;
        }
        liberar_no(esq);
    }

    if (idx < pai->num_chaves) {
        No *dir = criar_no(cab->ordem);
        le_no(arq, dir, pai->filhos[idx+1], cab->ordem);
        if (dir->num_chaves >= t) {
            No *filho = criar_no(cab->ordem);
            le_no(arq, filho, pai->filhos[idx], cab->ordem);
            borrow_from_next(arq, pai, idx, filho, cab->ordem);
            liberar_no(dir);
            liberar_no(filho);
            return;
        }
        liberar_no(dir);
    }

    if (idx < pai->num_chaves) {
        merge_nodes(arq, pai, idx, cab);
    } else {
        merge_nodes(arq, pai, idx-1, cab);
    }
}

void remover_rec(FILE *arq, No *no, int k, int t, Cabecalho *cab) {
    int idx = 0;
    while (idx < no->num_chaves && no->chaves[idx] < k) idx++;

    if (idx < no->num_chaves && no->chaves[idx] == k) {
        if (no->folha) {
            for (int i = idx; i < no->num_chaves - 1; i++) {
                no->chaves[i] = no->chaves[i+1];
                no->dados[i]  = no->dados[i+1]; // Shift dados
            }
            no->num_chaves--;
            escreve_no(arq, no, no->posicao, cab->ordem);
        } else {
            No *filho_esq = criar_no(cab->ordem);
            No *filho_dir = criar_no(cab->ordem);
            le_no(arq, filho_esq, no->filhos[idx], cab->ordem);
            le_no(arq, filho_dir, no->filhos[idx+1], cab->ordem);

            if (filho_esq->num_chaves >= t) {
                int pred_k, pred_d;
                pegar_antecessor(arq, no->filhos[idx], cab->ordem, &pred_k, &pred_d);
                no->chaves[idx] = pred_k;
                no->dados[idx]  = pred_d; // Substitui pelo dado do antecessor
                escreve_no(arq, no, no->posicao, cab->ordem);
                remover_rec(arq, filho_esq, pred_k, t, cab);
            } else if (filho_dir->num_chaves >= t) {
                int suc_k, suc_d;
                pegar_sucessor(arq, no->filhos[idx+1], cab->ordem, &suc_k, &suc_d);
                no->chaves[idx] = suc_k;
                no->dados[idx]  = suc_d; // Substitui pelo dado do sucessor
                escreve_no(arq, no, no->posicao, cab->ordem);
                remover_rec(arq, filho_dir, suc_k, t, cab);
            } else {
                merge_nodes(arq, no, idx, cab);
                No *filho_merged = criar_no(cab->ordem);
                le_no(arq, filho_merged, no->filhos[idx], cab->ordem);
                remover_rec(arq, filho_merged, k, t, cab);
                liberar_no(filho_merged);
            }
            liberar_no(filho_esq);
            liberar_no(filho_dir);
        }
    } else {
        if (no->folha) return;

        No *filho = criar_no(cab->ordem);
        le_no(arq, filho, no->filhos[idx], cab->ordem);
        
        if (filho->num_chaves < t) {
            fill_child(arq, no, idx, t, cab);
            if (idx > no->num_chaves) idx--; 
            le_no(arq, filho, no->filhos[idx], cab->ordem);
        }
        
        remover_rec(arq, filho, k, t, cab);
        liberar_no(filho);
    }
}

void remover(FILE *arq, int chave) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);
    if (cab.raiz_pos == -1) return;

    No *raiz = criar_no(cab.ordem);
    le_no(arq, raiz, cab.raiz_pos, cab.ordem);
    
    int t = (int)ceil(cab.ordem / 2.0);

    remover_rec(arq, raiz, chave, t, &cab);

    le_no(arq, raiz, cab.raiz_pos, cab.ordem); 
    if (raiz->num_chaves == 0 && !raiz->folha) {
        cab.raiz_pos = raiz->filhos[0];
        escreve_cabecalho(arq, &cab);
    } else if (raiz->num_chaves == 0 && raiz->folha) {
        cab.raiz_pos = -1;
        escreve_cabecalho(arq, &cab);
    }
    liberar_no(raiz);
}

// ============================================================================
// 5. BUSCA E IMPRESSÃO
// ============================================================================

void buscar_rec(FILE *arq, No *no, int k, int ordem) {
    int i = 0;
    while (i < no->num_chaves && k > no->chaves[i]) i++;

    if (i < no->num_chaves && k == no->chaves[i]) {
        printf("O REGISTRO ESTA NA ARVORE! (Dado: %d)\n", no->dados[i]);
        return;
    }

    if (no->folha) {
        printf("O REGISTRO NAO ESTA NA ARVORE!\n");
        return;
    }

    No *filho = criar_no(ordem);
    le_no(arq, filho, no->filhos[i], ordem);
    buscar_rec(arq, filho, k, ordem);
    liberar_no(filho);
}

void buscar(FILE *arq, int chave) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);
    if (cab.raiz_pos == -1) {
        printf("O REGISTRO NAO ESTA NA ARVORE!\n");
        return;
    }
    No *raiz = criar_no(cab.ordem);
    le_no(arq, raiz, cab.raiz_pos, cab.ordem);
    buscar_rec(arq, raiz, chave, cab.ordem);
    liberar_no(raiz);
}

void imprimir_arvore(FILE *arq) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);
    if (cab.raiz_pos == -1) return;

    long int fila[1000];
    int inicio = 0, fim = 0;
    
    int nos_no_nivel_atual = 1;
    int nos_no_prox_nivel = 0;

    fila[fim++] = cab.raiz_pos;

    printf("ARVORE B\n");
    
    while (inicio < fim) {
        if (nos_no_nivel_atual == 0) {
            printf("\n");
            nos_no_nivel_atual = nos_no_prox_nivel;
            nos_no_prox_nivel = 0;
        }

        long int pos = fila[inicio++];
        No *no = criar_no(cab.ordem);
        le_no(arq, no, pos, cab.ordem);
        nos_no_nivel_atual--;

        printf("[");
        for (int i = 0; i < no->num_chaves; i++) {
            // Imprime chave e dado
            printf("k:%d(d:%d), ", no->chaves[i], no->dados[i]);
        }
        printf("] ");

        if (!no->folha) {
            for (int i = 0; i <= no->num_chaves; i++) {
                if (no->filhos[i] != -1) {
                    fila[fim++] = no->filhos[i];
                    nos_no_prox_nivel++;
                }
            }
        }
        liberar_no(no);
    }
    printf("\n");
}