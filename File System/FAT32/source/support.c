#include "support.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "fat32.h"

/* Converte nomes para o formato FAT32 (8.3 ou LFN) */
bool cstr_to_fat32wnull(char *filename, char output[FAT32STR_SIZE_WNULL])
{
    char* strptr = filename;
    char* dot = strchr(filename, '.'); // Localiza o ponto no nome do arquivo

    if (dot == NULL) {
        // Nome de arquivo inválido (sem extensão ou muito longo)
        fprintf(stderr, "Erro: Nome de arquivo inválido (sem extensão ou muito longo).\n");
        return true;
    }

    int i;

    // Copia a parte do nome antes do ponto (máximo 8 caracteres)
    for (i = 0; strptr != dot && i < 8; strptr++, i++) {
        output[i] = toupper(*strptr); // Converte para maiúsculas
    }

    // Preenche com espaços até completar 8 caracteres
    while (i < 8) {
        output[i++] = ' ';
    }

    // Copia a extensão após o ponto (máximo 3 caracteres)
    strptr = dot + 1; // Avança para o caractere após o ponto
    for (; i < 11 && *strptr != '\0'; strptr++, i++) {
        output[i] = toupper(*strptr); // Converte para maiúsculas
    }

    // Preenche com espaços até completar 11 caracteres
    while (i < 11) {
        output[i++] = ' ';
    }

    output[11] = '\0'; // Adiciona o terminador nulo

    return false;
}
