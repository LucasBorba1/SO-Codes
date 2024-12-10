#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdbool.h>
#include "fat32.h" /* Alteração: Substituir "fat16.h" por "fat32.h" */

/* Converte strings C para o formato FAT */
bool cstr_to_fat16wnull(char *filename, char output[FAT16STR_SIZE_WNULL]);

#endif
