#ifndef OUTPUT_H
#define OUTPUT_H

#include "fat32.h" /* Alteração: Garantir que estamos incluindo suporte ao FAT32 */

/* Exibe os arquivos do diretório */
void show_files(struct fat_dir *);

/* Exibe informações detalhadas do BPB */
void verbose(struct fat_bpb *);

#endif
