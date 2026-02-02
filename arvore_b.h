#ifndef ARVORE_B_H
#define ARVORE_B_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Estrutura do Nó (Em RAM)
typedef struct {
    int num_chaves;                 
    int folha;                      
    long int posicao;               
    
    // Vetores dinâmicos
    int *chaves;       
    int *dados;         // <--- NOVO: Vetor de dados associados às chaves
    long int *filhos;     
} No;

// Cabeçalho do Arquivo
typedef struct {
    long int raiz_pos;              
    int ordem;                      
    long int total_nos;             
} Cabecalho;

// --- Gerenciamento de Memória ---
No* criar_no(int ordem);
void liberar_no(No *no);
long int tamanho_no_bytes(int ordem);

// --- Gerenciamento de Arquivo ---
void inicializar_arvore(FILE *arq, int ordem);
void le_no(FILE *arq, No *no, long int pos, int ordem);
void escreve_no(FILE *arq, No *no, long int pos, int ordem);
void le_cabecalho(FILE *arq, Cabecalho *cab);
void escreve_cabecalho(FILE *arq, Cabecalho *cab);

// --- Operações ---
// Inserir agora pede a chave E o dado
void inserir(FILE *arq, int chave, int dado); 
void remover(FILE *arq, int chave);
void buscar(FILE *arq, int chave);
void imprimir_arvore(FILE *arq);

#endif