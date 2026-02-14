# Nome do compilador
CC = gcc

# Flags de compilação:
# -Wall: Mostra todos os avisos (warnings) - Importante para achar erros
# -g: Adiciona informações de debug (útil para usar o valgrind depois)
CFLAGS = -Wall -g

# Nome do executável final (Obrigatório ser 'trab2' segundo o PDF)
TARGET = trab2

# Lista de arquivos fonte (.c)
SRCS = main.c arvore_b.c memoria_binaria.c

# Lista de arquivos objeto (.o) gerados a partir dos fontes
OBJS = $(SRCS:.c=.o)

# Regra principal (o que acontece quando você digita apenas 'make')
all: $(TARGET)

# Regra para criar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

# Regra para compilar cada arquivo .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra de limpeza (digite 'make clean' para apagar os arquivos gerados)
clean:
	rm -f $(OBJS) $(TARGET) arvore.bin

# Regra para rodar (opcional, para testar rápido)
run: $(TARGET)
	./$(TARGET)