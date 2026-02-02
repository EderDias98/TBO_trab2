#ifndef ARVORE_B_H
#define ARVORE_B_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Tamanho máximo fixo para alocação da struct no arquivo.
// Suporta árvores de até ordem 500. Se precisar de mais, aumente.
#define MAX_ORDEM 512 

// Estrutura do Nó (Página de Disco)
typedef struct {
    int num_chaves;                 // Quantidade atual de chaves no nó
    int folha;                      // 1 se for folha, 0 se for interno
    long int posicao;               // Endereço deste nó no arquivo (byte offset)
    
    int chaves[MAX_ORDEM - 1];      // Vetor de chaves
    long int filhos[MAX_ORDEM];     // Vetor de "ponteiros" (offsets) para filhos
} No;

// Cabeçalho do Arquivo (Metadados)
typedef struct {
    long int raiz_pos;              // Onde está a raiz (-1 se vazia)
    int ordem;                      // A ordem "m" da árvore
    long int total_nos;             // Contador para saber onde gravar novos nós
} Cabecalho;

// --- Protótipos das Funções ---

// Gerenciamento de Arquivo
FILE* abrir_arquivo(char *nome, char *modo);
void inicializar_arvore(FILE *arq, int ordem);
void le_no(FILE *arq, No *no, long int pos);
void escreve_no(FILE *arq, No *no, long int pos);
void le_cabecalho(FILE *arq, Cabecalho *cab);
void escreve_cabecalho(FILE *arq, Cabecalho *cab);
long int alocar_novo_no(FILE *arq, Cabecalho *cab);

// Operações da Árvore B
void inserir(FILE *arq, int chave);
void remover(FILE *arq, int chave);
void buscar(FILE *arq, int chave); // Imprime mensagem conforme PDF
void imprimir_arvore(FILE *arq);   // Imprime em largura conforme PDF

#endif