#include "arvore_b.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Verifica argumentos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        return 1;
    }

    FILE *fin = fopen(argv[1], "r");
    if (!fin) {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }

    freopen(argv[2], "w", stdout);

    char *nome_bin = "arvore.bin";
    FILE *fbin = fopen(nome_bin, "wb+");
    if (!fbin) {
        perror("Erro ao criar arquivo binario");
        return 1;
    }
    
    int ordem, n_ops;
    
    if (fscanf(fin, "%d", &ordem) != 1) return 1;
    
    inicializar_arvore(fbin, ordem);

    if (fscanf(fin, "%d", &n_ops) != 1) return 1;

    char operacao;
    int chave, dado;
    
    for (int i = 0; i < n_ops; i++) {
        if (fscanf(fin, " %c", &operacao) != 1) break;

        if (operacao == 'I') {
            // Tenta ler "I Chave, Dado"
            // Se o arquivo tiver só "I Chave", precisamos tratar.
            // O PDF diz "I chave, registro". Vamos assumir que são dois inteiros.
            // Tratamento robusto para vírgula opcional:
            fscanf(fin, "%d", &chave);
            
            // Verifica o próximo char para ver se tem vírgula ou dado
            char c = fgetc(fin);
            // Pula espaços e vírgulas
            while(c == ' ' || c == ',') c = fgetc(fin);
            ungetc(c, fin); // Devolve o numero (ou newline) pro buffer
            
            if (fscanf(fin, "%d", &dado) != 1) {
                // Se falhar em ler o dado (ex: fim de linha), usa um dummy
                dado = chave * 10; 
            }
            
            inserir(fbin, chave, dado);
            
        } else if (operacao == 'R') {
            fscanf(fin, "%d", &chave);
            remover(fbin, chave);
            
        } else if (operacao == 'B') {
            fscanf(fin, "%d", &chave);
            buscar(fbin, chave);
        }
    }

    imprimir_arvore(fbin);

    fclose(fin);
    fclose(fbin);
    return 0;
}