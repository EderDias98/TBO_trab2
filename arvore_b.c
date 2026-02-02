#include "arvore_b.h"
#include <string.h>
#include <math.h>

// ============================================================================
// 1. FUNÇÕES AUXILIARES DE DISCO ("Low Level")
// ============================================================================

void le_no(FILE *arq, No *no, long int pos) {
    if (pos == -1) return;
    fseek(arq, pos, SEEK_SET);
    fread(no, sizeof(No), 1, arq);
}

void escreve_no(FILE *arq, No *no, long int pos) {
    if (pos == -1) return;
    fseek(arq, pos, SEEK_SET);
    fwrite(no, sizeof(No), 1, arq);
}

void le_cabecalho(FILE *arq, Cabecalho *cab) {
    fseek(arq, 0, SEEK_SET);
    fread(cab, sizeof(Cabecalho), 1, arq);
}

void escreve_cabecalho(FILE *arq, Cabecalho *cab) {
    fseek(arq, 0, SEEK_SET);
    fwrite(cab, sizeof(Cabecalho), 1, arq);
}

// "Aloca" espaço no final do arquivo para um novo nó
long int alocar_novo_no(FILE *arq, Cabecalho *cab) {
    long int pos = sizeof(Cabecalho) + (cab->total_nos * sizeof(No));
    cab->total_nos++;
    escreve_cabecalho(arq, cab); // Atualiza contador
    return pos;
}

void inicializar_arvore(FILE *arq, int ordem) {
    Cabecalho cab;
    cab.ordem = ordem;
    cab.raiz_pos = -1; // Árvore vazia
    cab.total_nos = 0;
    escreve_cabecalho(arq, &cab);
}

// ============================================================================
// 2. LÓGICA DE INSERÇÃO (Baseada no código Python/CLRS)
// ============================================================================

// Divide um nó filho cheio (y) em dois
void split_child(FILE *arq, No *x, int i, No *y, Cabecalho *cab) {
    No z; // Novo nó
    z.folha = y->folha;
    z.posicao = alocar_novo_no(arq, cab);

    // T (grau minimo) aproximado para lógica de split baseada em Ordem
    // Ordem 4: split em 1 (sobe), 1 (fica), 1 (novo) -> Mínimo é 1
    // A logica aqui usa a mediana exata da Ordem M.
    int meio = (cab->ordem - 1) / 2; 

    // O novo nó (z) recebe as chaves da metade superior de y
    // Quantas chaves z vai ter? (Total - meio - 1 que sobe)
    z.num_chaves = y->num_chaves - meio - 1;

    for (int j = 0; j < z.num_chaves; j++) {
        z.chaves[j] = y->chaves[j + meio + 1];
    }

    // Se não for folha, z recebe também os filhos correspondentes
    if (!y->folha) {
        for (int j = 0; j <= z.num_chaves; j++) {
            z.filhos[j] = y->filhos[j + meio + 1];
        }
    }

    // Atualiza número de chaves de y
    y->num_chaves = meio;

    // Move os filhos de x para abrir espaço para z
    for (int j = x->num_chaves; j >= i + 1; j--) {
        x->filhos[j + 1] = x->filhos[j];
    }
    x->filhos[i + 1] = z.posicao;

    // Move as chaves de x para abrir espaço para a mediana de y
    for (int j = x->num_chaves - 1; j >= i; j--) {
        x->chaves[j + 1] = x->chaves[j];
    }
    x->chaves[i] = y->chaves[meio];
    x->num_chaves++;

    // Grava as alterações no disco
    escreve_no(arq, y, y->posicao);
    escreve_no(arq, &z, z.posicao);
    escreve_no(arq, x, x->posicao);
}

void inserir_nao_cheio(FILE *arq, No *x, int k, Cabecalho *cab) {
    int i = x->num_chaves - 1;

    if (x->folha) {
        // Insere ordenado no vetor
        while (i >= 0 && k < x->chaves[i]) {
            x->chaves[i + 1] = x->chaves[i];
            i--;
        }
        x->chaves[i + 1] = k;
        x->num_chaves++;
        escreve_no(arq, x, x->posicao);
    } else {
        // Encontra o filho correto para descer
        while (i >= 0 && k < x->chaves[i]) {
            i--;
        }
        i++;
        
        No filho;
        le_no(arq, &filho, x->filhos[i]);

        // Se o filho estiver cheio, divide antes de entrar
        if (filho.num_chaves == cab->ordem - 1) {
            split_child(arq, x, i, &filho, cab);
            // Decide se desce para o novo filho ou o antigo
            if (k > x->chaves[i]) {
                i++;
                le_no(arq, &filho, x->filhos[i]); // Recarrega o filho correto
            }
        }
        inserir_nao_cheio(arq, &filho, k, cab);
    }
}

void inserir(FILE *arq, int chave) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);

    if (cab.raiz_pos == -1) {
        // Cria raiz
        No raiz;
        raiz.folha = 1;
        raiz.num_chaves = 1;
        raiz.chaves[0] = chave;
        raiz.posicao = alocar_novo_no(arq, &cab);
        for(int i=0; i<MAX_ORDEM; i++) raiz.filhos[i] = -1;

        cab.raiz_pos = raiz.posicao;
        escreve_cabecalho(arq, &cab);
        escreve_no(arq, &raiz, raiz.posicao);
    } else {
        No raiz;
        le_no(arq, &raiz, cab.raiz_pos);

        if (raiz.num_chaves == cab.ordem - 1) {
            // Raiz cheia: cria nova raiz e faz split da antiga
            No nova_raiz;
            nova_raiz.folha = 0;
            nova_raiz.num_chaves = 0;
            nova_raiz.posicao = alocar_novo_no(arq, &cab);
            nova_raiz.filhos[0] = cab.raiz_pos;
            for(int i=1; i<MAX_ORDEM; i++) nova_raiz.filhos[i] = -1;

            cab.raiz_pos = nova_raiz.posicao;
            escreve_cabecalho(arq, &cab);
            
            // Grava nova raiz antes do split para garantir integridade
            escreve_no(arq, &nova_raiz, nova_raiz.posicao); 

            split_child(arq, &nova_raiz, 0, &raiz, &cab);
            inserir_nao_cheio(arq, &nova_raiz, chave, &cab);
        } else {
            inserir_nao_cheio(arq, &raiz, chave, &cab);
        }
    }
}

// ============================================================================
// 3. LÓGICA DE REMOÇÃO (Traduzido do Python/CLRS para C com Disco)
// ============================================================================

// Funções auxiliares para remoção

// Pega o antecessor (maior chave da subárvore esquerda)
int pegar_antecessor(FILE *arq, long int pos_no) {
    No atual;
    le_no(arq, &atual, pos_no);
    while (!atual.folha) {
        le_no(arq, &atual, atual.filhos[atual.num_chaves]); // Vai tudo para direita
    }
    return atual.chaves[atual.num_chaves - 1];
}

// Pega o sucessor (menor chave da subárvore direita)
int pegar_sucessor(FILE *arq, long int pos_no) {
    No atual;
    le_no(arq, &atual, pos_no);
    while (!atual.folha) {
        le_no(arq, &atual, atual.filhos[0]); // Vai tudo para esquerda
    }
    return atual.chaves[0];
}

// Funde o filho[idx] com filho[idx+1]
void merge_nodes(FILE *arq, No *pai, int idx, Cabecalho *cab) {
    No esquerda, direita;
    long int pos_esq = pai->filhos[idx];
    long int pos_dir = pai->filhos[idx+1];
    
    le_no(arq, &esquerda, pos_esq);
    le_no(arq, &direita, pos_dir);

    // 1. Desce a chave mediana do pai para o final da esquerda
    esquerda.chaves[esquerda.num_chaves] = pai->chaves[idx];
    esquerda.num_chaves++;

    // 2. Copia chaves da direita para esquerda
    for (int i = 0; i < direita.num_chaves; i++) {
        esquerda.chaves[esquerda.num_chaves + i] = direita.chaves[i];
    }

    // 3. Copia filhos da direita para esquerda (se não for folha)
    if (!esquerda.folha) {
        for (int i = 0; i <= direita.num_chaves; i++) {
            esquerda.filhos[esquerda.num_chaves + i] = direita.filhos[i];
        }
    }
    esquerda.num_chaves += direita.num_chaves;

    // 4. Ajusta o pai (remove a chave que desceu e o ponteiro para direita)
    for (int i = idx; i < pai->num_chaves - 1; i++) {
        pai->chaves[i] = pai->chaves[i+1];
        pai->filhos[i+1] = pai->filhos[i+2];
    }
    pai->num_chaves--;

    escreve_no(arq, &esquerda, pos_esq);
    escreve_no(arq, pai, pai->posicao);
    // Nota: O nó 'direita' fica "perdido" no arquivo (leak de disco permitido pelo PDF)
}

// Pega emprestado do irmão anterior
void borrow_from_prev(FILE *arq, No *pai, int idx, No *filho) {
    No irmao;
    le_no(arq, &irmao, pai->filhos[idx-1]);

    // Abre espaço no filho (shift direita)
    for (int i = filho->num_chaves - 1; i >= 0; i--) {
        filho->chaves[i+1] = filho->chaves[i];
    }
    if (!filho->folha) {
        for (int i = filho->num_chaves; i >= 0; i--) {
            filho->filhos[i+1] = filho->filhos[i];
        }
    }

    // Chave do pai desce para filho
    filho->chaves[0] = pai->chaves[idx-1];
    if (!filho->folha) filho->filhos[0] = irmao.filhos[irmao.num_chaves];
    filho->num_chaves++;

    // Chave do irmão sobe para pai
    pai->chaves[idx-1] = irmao.chaves[irmao.num_chaves-1];
    irmao.num_chaves--;

    escreve_no(arq, pai, pai->posicao);
    escreve_no(arq, filho, filho->posicao);
    escreve_no(arq, &irmao, irmao.posicao);
}

// Pega emprestado do irmão seguinte
void borrow_from_next(FILE *arq, No *pai, int idx, No *filho) {
    No irmao;
    le_no(arq, &irmao, pai->filhos[idx+1]);

    // Chave do pai desce para filho (no fim)
    filho->chaves[filho->num_chaves] = pai->chaves[idx];
    if (!filho->folha) filho->filhos[filho->num_chaves+1] = irmao.filhos[0];
    filho->num_chaves++;

    // Chave do irmão sobe para pai
    pai->chaves[idx] = irmao.chaves[0];

    // Ajusta irmão (shift esquerda)
    for (int i = 1; i < irmao.num_chaves; i++) {
        irmao.chaves[i-1] = irmao.chaves[i];
    }
    if (!irmao.folha) {
        for (int i = 1; i <= irmao.num_chaves; i++) {
            irmao.filhos[i-1] = irmao.filhos[i];
        }
    }
    irmao.num_chaves--;

    escreve_no(arq, pai, pai->posicao);
    escreve_no(arq, filho, filho->posicao);
    escreve_no(arq, &irmao, irmao.posicao);
}

// Garante que o filho em pai->filhos[idx] tenha pelo menos t chaves
// t aqui é calculado como ceil(m/2)
void fill_child(FILE *arq, No *pai, int idx, int t, Cabecalho *cab) {
    // Verifica irmão esquerdo
    if (idx > 0) {
        No esq;
        le_no(arq, &esq, pai->filhos[idx-1]);
        if (esq.num_chaves >= t) {
            No filho; le_no(arq, &filho, pai->filhos[idx]);
            borrow_from_prev(arq, pai, idx, &filho);
            return;
        }
    }

    // Verifica irmão direito
    if (idx < pai->num_chaves) {
        No dir;
        le_no(arq, &dir, pai->filhos[idx+1]);
        if (dir.num_chaves >= t) {
            No filho; le_no(arq, &filho, pai->filhos[idx]);
            borrow_from_next(arq, pai, idx, &filho);
            return;
        }
    }

    // Se ninguém pode emprestar, MERGE
    if (idx < pai->num_chaves) {
        merge_nodes(arq, pai, idx, cab); // Merge com direito
    } else {
        merge_nodes(arq, pai, idx-1, cab); // Merge com esquerdo
    }
}

void remover_rec(FILE *arq, No *no, int k, int t, Cabecalho *cab) {
    int idx = 0;
    while (idx < no->num_chaves && no->chaves[idx] < k) idx++;

    // Caso 1: A chave k está neste nó
    if (idx < no->num_chaves && no->chaves[idx] == k) {
        if (no->folha) {
            // Caso 1: Folha, apenas remove
            for (int i = idx; i < no->num_chaves - 1; i++) {
                no->chaves[i] = no->chaves[i+1];
            }
            no->num_chaves--;
            escreve_no(arq, no, no->posicao);
        } else {
            // Caso 2: Nó interno
            No filho_esq, filho_dir;
            le_no(arq, &filho_esq, no->filhos[idx]);
            le_no(arq, &filho_dir, no->filhos[idx+1]);

            if (filho_esq.num_chaves >= t) {
                // 2a: Antecessor
                int pred = pegar_antecessor(arq, no->filhos[idx]);
                no->chaves[idx] = pred;
                escreve_no(arq, no, no->posicao);
                remover_rec(arq, &filho_esq, pred, t, cab);
            } else if (filho_dir.num_chaves >= t) {
                // 2b: Sucessor
                int suc = pegar_sucessor(arq, no->filhos[idx+1]);
                no->chaves[idx] = suc;
                escreve_no(arq, no, no->posicao);
                remover_rec(arq, &filho_dir, suc, t, cab);
            } else {
                // 2c: Merge
                merge_nodes(arq, no, idx, cab);
                // Depois do merge, a chave k desceu para o filho (agora merged)
                No filho_merged;
                le_no(arq, &filho_merged, no->filhos[idx]);
                remover_rec(arq, &filho_merged, k, t, cab);
            }
        }
    } else {
        // Caso 3: A chave não está aqui
        if (no->folha) {
            // Chave não existe na árvore
            return; 
        }

        // Verifica se o filho onde a chave deveria estar tem poucas chaves
        // t_min = ceil(m/2)
        // Precisamos garantir que ele tenha pelo menos t_min chaves antes de descer (regra proativa B-Tree)
        // No CLRS 't' é o grau mínimo. Em ordem 'm', o mínimo é ceil(m/2)-1. 
        // A lógica de preenchimento deve garantir t chaves onde t = ceil(m/2).
        
        // Antes de descer, verifica se precisa encher o filho
        No filho;
        le_no(arq, &filho, no->filhos[idx]);
        
        // Mínimo seguro para descer e poder remover é t chaves.
        if (filho.num_chaves < t) {
            fill_child(arq, no, idx, t, cab);
            // Se fez merge, o índice pode ter mudado ou o nó pai ter perdido chaves
            if (idx > no->num_chaves) idx--; 
            le_no(arq, &filho, no->filhos[idx]); // Recarrega filho correto
        }
        
        remover_rec(arq, &filho, k, t, cab);
    }
}

void remover(FILE *arq, int chave) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);
    if (cab.raiz_pos == -1) return;

    No raiz;
    le_no(arq, &raiz, cab.raiz_pos);
    
    // Calcula o "t" (mínimo de chaves + 1) para a lógica de B-Tree
    // Ex: Ordem 4 -> Min chaves 1. t deve ser 2 para a lógica funcionar (ter margem).
    int t = (int)ceil(cab.ordem / 2.0);

    remover_rec(arq, &raiz, chave, t, &cab);

    // Se a raiz ficar vazia após remoção
    le_no(arq, &raiz, cab.raiz_pos); // Recarrega para verificar
    if (raiz.num_chaves == 0 && !raiz.folha) {
        cab.raiz_pos = raiz.filhos[0]; // Filho único vira nova raiz
        escreve_cabecalho(arq, &cab);
    } else if (raiz.num_chaves == 0 && raiz.folha) {
        cab.raiz_pos = -1; // Árvore vazia
        escreve_cabecalho(arq, &cab);
    }
}

// ============================================================================
// 4. BUSCA E IMPRESSÃO
// ============================================================================

void buscar_rec(FILE *arq, No *no, int k) {
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

    No filho;
    le_no(arq, &filho, no->filhos[i]);
    buscar_rec(arq, &filho, k);
}

void buscar(FILE *arq, int chave) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);
    if (cab.raiz_pos == -1) {
        printf("O REGISTRO NAO ESTA NA ARVORE!\n");
        return;
    }
    No raiz;
    le_no(arq, &raiz, cab.raiz_pos);
    buscar_rec(arq, &raiz, chave);
}

void imprimir_arvore(FILE *arq) {
    Cabecalho cab;
    le_cabecalho(arq, &cab);
    if (cab.raiz_pos == -1) return;

    // BFS (Largura) simples usando array como fila (para fins didáticos)
    // Para uma árvore muito grande, precisaria de fila dinâmica
    long int fila[1000];
    int inicio = 0, fim = 0;
    
    // Controladores de nível
    int nos_no_nivel_atual = 1;
    int nos_no_prox_nivel = 0;

    fila[fim++] = cab.raiz_pos;

    printf("ARVORE B\n");
    
    while (inicio < fim) {
        if (nos_no_nivel_atual == 0) {
            printf("\n"); // Quebra de linha ao mudar de nível
            nos_no_nivel_atual = nos_no_prox_nivel;
            nos_no_prox_nivel = 0;
        }

        long int pos = fila[inicio++];
        No no;
        le_no(arq, &no, pos);
        nos_no_nivel_atual--;

        // Impressão no formato: [key: X, key: Y, ]
        printf("[");
        for (int i = 0; i < no.num_chaves; i++) {
            printf("key: %d, ", no.chaves[i]);
        }
        printf("] ");

        if (!no.folha) {
            for (int i = 0; i <= no.num_chaves; i++) {
                if (no.filhos[i] != -1) {
                    fila[fim++] = no.filhos[i];
                    nos_no_prox_nivel++;
                }
            }
        }
    }
    printf("\n"); // Quebra final
}