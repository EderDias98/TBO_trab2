#ifndef ARVORE_B_H
#define ARVORE_B_H

#include <stdio.h>
#include "memoria_binaria.h" 

// --- OPERAÇÕES DA ÁRVORE ---
// O main só precisa conhecer essas funções.
// Elas encapsulam toda a lógica de splits, merges e recursão.

void inserir(FILE *arq, int chave, int dado); 
void remover(FILE *arq, int chave);
void buscar(FILE *arq, int chave);
void imprimir_arvore(FILE *arq);

#endif

