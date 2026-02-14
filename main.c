#include "arvore_b.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // 1. Verificação de Argumentos (Conforme Seção 6.3)
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        return 1;
    }

    // 2. Abrir arquivo de entrada
    FILE *fin = fopen(argv[1], "r");
    if (!fin) {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }

    // 3. Redirecionar STDOUT para o arquivo de saída (Conforme Seção 6.2)
    // Isso garante que os printfs de "buscar" e "imprimir_arvore" vão para o arquivo.
    if (!freopen(argv[2], "w", stdout)) {
        perror("Erro ao abrir arquivo de saida");
        fclose(fin);
        return 1;
    }

    // 4. Criar Arquivo Binário Temporário (Conforme Seção 4)
    // "O arquivo binário deve ser criado na criação do primeiro nó... e mantido..."
    char *nome_bin = "arvore.bin";
    FILE *fbin = fopen(nome_bin, "wb+");
    if (!fbin) {
        perror("Erro ao criar arquivo binario");
        fclose(fin);
        return 1;
    }
    
    // 5. Leitura da Ordem e Inicialização
    int ordem, n_ops;
    
    // Lê a ordem (primeira linha)
    if (fscanf(fin, "%d", &ordem) != 1) {
        fprintf(stderr, "Erro ao ler a ordem da arvore.\n");
        return 1;
    }
    
    inicializar_arquivo(fbin, ordem);

    // Lê o número de operações (segunda linha)
    if (fscanf(fin, "%d", &n_ops) != 1) {
        fprintf(stderr, "Erro ao ler numero de operacoes.\n");
        return 1;
    }

    // 6. Loop de Processamento das Operações
    char operacao;
    int chave, dado;
    
    // O loop deve rodar exatamente n_ops vezes
    for (int i = 0; i < n_ops; i++) {
        // " %c" com espaço antes pula quebras de linha/espaços em branco
        if (fscanf(fin, " %c", &operacao) != 1) break;

        if (operacao == 'I') {
            // Formato PDF: "I chave, registro" (Ex: I 20, 20)
            // O formato "%d , %d" lê o inteiro, consome a vírgula (se houver) e lê o próximo.
            if (fscanf(fin, "%d , %d", &chave, &dado) == 2) {
                inserir(fbin, chave, dado);
            } else {
                fprintf(stderr, "Erro de formato na insercao.\n");
            }
            
        } else if (operacao == 'R') {
            // Formato PDF: "R chave"
            fscanf(fin, "%d", &chave);
            remover(fbin, chave);
            
        } else if (operacao == 'B') {
            // Formato PDF: "B chave"
            fscanf(fin, "%d", &chave);
            buscar(fbin, chave);
        }
    }

    // 7. Impressão Final da Árvore (Conforme Seção 5, item 4 e Seção 6.2)
    // A árvore deve ser impressa ao final de todas as operações.
    imprimir_arvore(fbin);

    // 8. Limpeza e Remoção do Binário (Conforme Seção 4)
    // "e deve ser apagado na finalização da execução do programa."
    fclose(fin);
    fclose(fbin);
    remove(nome_bin); // Deleta o arquivo arvore.bin do disco

    return 0;
}