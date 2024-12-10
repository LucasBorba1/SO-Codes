# Diretórios
INCLUDE = include
SOURCE  = source
BUILD   = build

# Compilador e argumentos
CC    = cc
CARGS = -Wall -Wextra -g -O0 -I$(INCLUDE) -pedantic -std=c11

# Nome do executável
NAME = fat32_fs

# Listagem de objetos e cabeçalhos
OBJS    = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))
HEADERS = $(wildcard $(INCLUDE)/*.h)

# Nome do arquivo de disco
DISK_IMG = diskTest32.img

# Alvos principais
.PHONY: all builddir clean resetimg

all: $(NAME)

# Reset da imagem de disco
resetimg:
	@cp -v backup.img $(DISK_IMG)
	@echo "Imagem de disco restaurada para o estado original."

# Criação do diretório de build
builddir:
	@mkdir -p $(BUILD)

# Compilação de objetos
$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS)
	$(CC) -c $(CARGS) $< -o $@
	@echo 'CC   ' $<

# Limpeza de arquivos gerados
clean:
	@rm -vf $(NAME) $(OBJS)
	@rm -vf $(DISK_IMG)
	@echo 'Cleaned build files.'

# Linkagem final
$(NAME): builddir $(OBJS)
	$(CC) $(CARGS) $(OBJS) -o $@
	@echo 'CCLD ' $(NAME)
