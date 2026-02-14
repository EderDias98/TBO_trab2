#ifndef MEMORIA_BINARIA_H
#define MEMORIA_BINARIA_H

#include <stdio.h>
#include <stdlib.h>

// --- ESTRUTURAS ---

typedef struct {
    int num_chaves;     
    int folha;          
    long int posicao;   
    
    // Ponteiros para arrays dinâmicos
    int *chaves;        
    int *dados;         
    long int *filhos;   
} No;

typedef struct {
    long int raiz_pos;  
    int ordem;          
    long int total_nos; 
} Cabecalho;

// --- GERENCIAMENTO DE MEMÓRIA (RAM) ---
No* criar_no(int ordem);
void liberar_no(No *no);

// --- GERENCIAMENTO DE DISCO (IO) ---
void le_no(FILE *arq, No *no, long int pos, int ordem);
void escreve_no(FILE *arq, No *no, long int pos, int ordem);
void le_cabecalho(FILE *arq, Cabecalho *cab);
void escreve_cabecalho(FILE *arq, Cabecalho *cab);

// --- UTILITÁRIOS DE ARQUIVO ---
void inicializar_arquivo(FILE *arq, int ordem); // Antiga inicializar_arvore (parte de disco)
long int alocar_novo_no(FILE *arq, Cabecalho *cab);

#endif