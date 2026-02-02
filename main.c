#include "arvore_b.h"
#include <stdio.h>
#include <stdlib.h>
int main() {
    char *arquivo = "arvore.bin";
    int ordem = 4; // Ordem 4 (igual ao exemplo do PDF)
    FILE* arq = fopen(arquivo,"w+");
    // 1. Cria a árvore
    printf("Criando arvore B de ordem %d...\n", ordem);
    inicializar_arvore(arq, ordem);

    // 2. Inserções do Exemplo do PDF [cite: 124-136]
    int chaves[] = {20, 75, 77, 78, 55, 62, 51, 40, 60, 45};
    int n = 10;

    for (int i = 0; i < n; i++) {
        printf("Inserindo chave: %d\n", chaves[i]);
        inserir(arq, chaves[i]);
        
        // Debug: Mostra como ficou após cada inserção
        // imprimir_arvore(arquivo); 
        // printf("\n---\n");
    }

    // 3. Resultado Final
    printf("\n--- ESTADO FINAL DA ARVORE ---\n");
    imprimir_arvore(arq);

    return 0;
}