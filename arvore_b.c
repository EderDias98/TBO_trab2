#include "arvore_b.h"
#include <string.h>
#include <math.h>

// ============================================================================
// FUNÇÕES AUXILIARES (PRIVADAS / STATIC)
// ============================================================================

// Insere ordenado em arrays na memória
static void inserir_em_array(int *chaves, int *dados, long *filhos, int n, int k, int d, long filho_dir) {
    int i = n - 1;
    while (i >= 0 && k < chaves[i]) {
        chaves[i+1] = chaves[i];
        dados[i+1] = dados[i];
        filhos[i+2] = filhos[i+1];
        i--;
    }
    chaves[i+1] = k;
    dados[i+1] = d;
    filhos[i+2] = filho_dir;
}

// ============================================================================
// INSERÇÃO (BOTTOM-UP)
// ============================================================================

static int inserir_recursivo(FILE *arq, long pos_atual, int k, int d, Cabecalho *cab, 
                      int *k_prom, int *d_prom, long *pos_filho_dir_prom) {
    
    No *no = criar_no(cab->ordem);
    le_no(arq, no, pos_atual, cab->ordem);
    int flag_overflow = 0;

    if (no->folha) {
        if (no->num_chaves < cab->ordem - 1) {
            inserir_em_array(no->chaves, no->dados, no->filhos, no->num_chaves, k, d, -1);
            no->num_chaves++;
            escreve_no(arq, no, pos_atual, cab->ordem);
            flag_overflow = 0;
        } else {
            // Split na Folha
            int *temp_k = malloc(sizeof(int) * cab->ordem);
            int *temp_d = malloc(sizeof(int) * cab->ordem);
            long *temp_f = malloc(sizeof(long) * (cab->ordem + 1));

            for(int i=0; i<no->num_chaves; i++) {
                temp_k[i] = no->chaves[i];
                temp_d[i] = no->dados[i];
                temp_f[i] = no->filhos[i];
            }
            temp_f[no->num_chaves] = no->filhos[no->num_chaves];

            inserir_em_array(temp_k, temp_d, temp_f, no->num_chaves, k, d, -1);
            
            int idx_meio = (cab->ordem - 1) / 2;
            *k_prom = temp_k[idx_meio];
            *d_prom = temp_d[idx_meio];

            No *novo = criar_no(cab->ordem);
            novo->folha = 1;
            novo->posicao = alocar_novo_no(arq, cab);
            novo->num_chaves = 0;

            int j = 0;
            for (int i = idx_meio + 1; i < cab->ordem; i++) {
                novo->chaves[j] = temp_k[i];
                novo->dados[j] = temp_d[i];
                novo->filhos[j] = temp_f[i];
                novo->num_chaves++;
                j++;
            }
            novo->filhos[novo->num_chaves] = temp_f[cab->ordem]; 

            no->num_chaves = idx_meio; 
            *pos_filho_dir_prom = novo->posicao;
            flag_overflow = 1;

            escreve_no(arq, no, pos_atual, cab->ordem);
            escreve_no(arq, novo, novo->posicao, cab->ordem);

            free(temp_k); free(temp_d); free(temp_f);
            liberar_no(novo);
        }
    } else {
        // Não é folha
        int i = 0;
        while (i < no->num_chaves && k > no->chaves[i]) i++;
        
        int k_up, d_up;
        long pos_up;
        
        int overflow_filho = inserir_recursivo(arq, no->filhos[i], k, d, cab, &k_up, &d_up, &pos_up);

        if (overflow_filho) {
            if (no->num_chaves < cab->ordem - 1) {
                inserir_em_array(no->chaves, no->dados, no->filhos, no->num_chaves, k_up, d_up, pos_up);
                no->num_chaves++;
                escreve_no(arq, no, pos_atual, cab->ordem);
                flag_overflow = 0;
            } else {
                // Split no Nó Interno
                int *temp_k = malloc(sizeof(int) * cab->ordem);
                int *temp_d = malloc(sizeof(int) * cab->ordem);
                long *temp_f = malloc(sizeof(long) * (cab->ordem + 1));

                for(int x=0; x<no->num_chaves; x++) {
                    temp_k[x] = no->chaves[x];
                    temp_d[x] = no->dados[x];
                    temp_f[x] = no->filhos[x];
                }
                temp_f[no->num_chaves] = no->filhos[no->num_chaves];

                inserir_em_array(temp_k, temp_d, temp_f, no->num_chaves, k_up, d_up, pos_up);

                int idx_meio = (cab->ordem - 1) / 2;
                *k_prom = temp_k[idx_meio];
                *d_prom = temp_d[idx_meio];

                No *novo = criar_no(cab->ordem);
                novo->folha = 0;
                novo->posicao = alocar_novo_no(arq, cab);
                novo->num_chaves = 0;

                int j = 0;
                for (int x = idx_meio + 1; x < cab->ordem; x++) {
                    novo->chaves[j] = temp_k[x];
                    novo->dados[j] = temp_d[x];
                    novo->filhos[j] = temp_f[x];
                    novo->num_chaves++;
                    j++;
                }
                novo->filhos[novo->num_chaves] = temp_f[cab->ordem];

                no->num_chaves = idx_meio;
                *pos_filho_dir_prom = novo->posicao;
                flag_overflow = 1;

                escreve_no(arq, no, pos_atual, cab->ordem);
                escreve_no(arq, novo, novo->posicao, cab->ordem);

                free(temp_k); free(temp_d); free(temp_f);
                liberar_no(novo);
            }
        }
    }
    liberar_no(no);
    return flag_overflow;
}

void inserir(FILE *arq, int chave, int dado) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);

    if (cab.raiz_pos == -1) {
        No *raiz = criar_no(cab.ordem);
        raiz->folha = 1;
        raiz->num_chaves = 1;
        raiz->chaves[0] = chave;
        raiz->dados[0] = dado;
        raiz->posicao = alocar_novo_no(arq, &cab);
        for(int i=0; i<cab.ordem; i++) raiz->filhos[i] = -1;
        
        cab.raiz_pos = raiz->posicao;
        escreve_cabecalho(arq, &cab);
        escreve_no(arq, raiz, raiz->posicao, cab.ordem);
        liberar_no(raiz);
    } else {
        int k_prom, d_prom;
        long pos_filho_dir;
        int overflow_raiz = inserir_recursivo(arq, cab.raiz_pos, chave, dado, &cab, &k_prom, &d_prom, &pos_filho_dir);
        
        if (overflow_raiz) {
            No *nova_raiz = criar_no(cab.ordem);
            nova_raiz->folha = 0;
            nova_raiz->num_chaves = 1;
            nova_raiz->posicao = alocar_novo_no(arq, &cab);
            
            nova_raiz->chaves[0] = k_prom;
            nova_raiz->dados[0] = d_prom;
            nova_raiz->filhos[0] = cab.raiz_pos;
            nova_raiz->filhos[1] = pos_filho_dir;
            
            for(int i=2; i<cab.ordem; i++) nova_raiz->filhos[i] = -1;

            cab.raiz_pos = nova_raiz->posicao;
            escreve_cabecalho(arq, &cab);
            escreve_no(arq, nova_raiz, nova_raiz->posicao, cab.ordem);
            liberar_no(nova_raiz);
        }
    }
}

// ============================================================================
// REMOÇÃO (Helpers e Recursão)
// ============================================================================

static void pegar_antecessor(FILE *arq, long int pos_no, int ordem, int *chave_out, int *dado_out) {
    No *atual = criar_no(ordem);
    le_no(arq, atual, pos_no, ordem);
    while (!atual->folha) {
        long int prox = atual->filhos[atual->num_chaves];
        le_no(arq, atual, prox, ordem);
    }
    *chave_out = atual->chaves[atual->num_chaves - 1];
    *dado_out  = atual->dados[atual->num_chaves - 1];
    liberar_no(atual);
}

static void pegar_sucessor(FILE *arq, long int pos_no, int ordem, int *chave_out, int *dado_out) {
    No *atual = criar_no(ordem);
    le_no(arq, atual, pos_no, ordem);
    while (!atual->folha) {
        long int prox = atual->filhos[0];
        le_no(arq, atual, prox, ordem);
    }
    *chave_out = atual->chaves[0];
    *dado_out  = atual->dados[0];
    liberar_no(atual);
}

static void merge_nodes(FILE *arq, No *pai, int idx, Cabecalho *cab) {
    No *esquerda = criar_no(cab->ordem);
    No *direita = criar_no(cab->ordem);
    long int pos_esq = pai->filhos[idx];
    long int pos_dir = pai->filhos[idx+1];
    
    le_no(arq, esquerda, pos_esq, cab->ordem);
    le_no(arq, direita, pos_dir, cab->ordem);

    esquerda->chaves[esquerda->num_chaves] = pai->chaves[idx];
    esquerda->dados[esquerda->num_chaves]  = pai->dados[idx];
    esquerda->num_chaves++;

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

static void borrow_from_prev(FILE *arq, No *pai, int idx, No *filho, int ordem) {
    No *irmao = criar_no(ordem);
    le_no(arq, irmao, pai->filhos[idx-1], ordem);

    for (int i = filho->num_chaves - 1; i >= 0; i--) {
        filho->chaves[i+1] = filho->chaves[i];
        filho->dados[i+1]  = filho->dados[i];
    }
    if (!filho->folha) {
        for (int i = filho->num_chaves; i >= 0; i--) {
            filho->filhos[i+1] = filho->filhos[i];
        }
    }

    filho->chaves[0] = pai->chaves[idx-1];
    filho->dados[0]  = pai->dados[idx-1];
    if (!filho->folha) filho->filhos[0] = irmao->filhos[irmao->num_chaves];
    filho->num_chaves++;

    pai->chaves[idx-1] = irmao->chaves[irmao->num_chaves-1];
    pai->dados[idx-1]  = irmao->dados[irmao->num_chaves-1];
    irmao->num_chaves--;

    escreve_no(arq, pai, pai->posicao, ordem);
    escreve_no(arq, filho, filho->posicao, ordem);
    escreve_no(arq, irmao, irmao->posicao, ordem);
    liberar_no(irmao);
}

static void borrow_from_next(FILE *arq, No *pai, int idx, No *filho, int ordem) {
    No *irmao = criar_no(ordem);
    le_no(arq, irmao, pai->filhos[idx+1], ordem);

    filho->chaves[filho->num_chaves] = pai->chaves[idx];
    filho->dados[filho->num_chaves]  = pai->dados[idx];
    if (!filho->folha) filho->filhos[filho->num_chaves+1] = irmao->filhos[0];
    filho->num_chaves++;

    pai->chaves[idx] = irmao->chaves[0];
    pai->dados[idx]  = irmao->dados[0];

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

static void fill_child(FILE *arq, No *pai, int idx, int t, Cabecalho *cab) {
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
    if (idx < pai->num_chaves) merge_nodes(arq, pai, idx, cab);
    else merge_nodes(arq, pai, idx-1, cab);
}

static void remover_rec(FILE *arq, No *no, int k, int t, Cabecalho *cab) {
    int idx = 0;
    while (idx < no->num_chaves && no->chaves[idx] < k) idx++;

    if (idx < no->num_chaves && no->chaves[idx] == k) {
        if (no->folha) {
            for (int i = idx; i < no->num_chaves - 1; i++) {
                no->chaves[i] = no->chaves[i+1];
                no->dados[i]  = no->dados[i+1];
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
                no->dados[idx]  = pred_d;
                escreve_no(arq, no, no->posicao, cab->ordem);
                remover_rec(arq, filho_esq, pred_k, t, cab);
            } else if (filho_dir->num_chaves >= t) {
                int suc_k, suc_d;
                pegar_sucessor(arq, no->filhos[idx+1], cab->ordem, &suc_k, &suc_d);
                no->chaves[idx] = suc_k;
                no->dados[idx]  = suc_d;
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
// BUSCA E IMPRESSÃO
// ============================================================================

static void buscar_rec(FILE *arq, No *no, int k, int ordem) {
    int i = 0;
    while (i < no->num_chaves && k > no->chaves[i]) i++;

    if (i < no->num_chaves && k == no->chaves[i]) {
        printf("O REGISTRO ESTA NA ARVORE!\n");
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
            printf("key: %d, ", no->chaves[i]);
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