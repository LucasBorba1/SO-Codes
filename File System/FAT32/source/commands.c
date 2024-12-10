#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "commands.h"
#include "fat32.h"
#include "support.h"

#include <errno.h>
#include <err.h>
#include <error.h>
#include <assert.h>

#include <sys/types.h>

/*
 * Função de busca na pasta raíz.
 * Alteração para FAT32: não depende mais de `bpb->possible_rentries`,
 * pois o diretório raiz agora está em clusters.
 */
struct far_dir_searchres find_in_root(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb)
{
	struct far_dir_searchres res = { .found = false };

	for (size_t i = 0; i < bpb->bytes_p_sect / sizeof(struct fat_dir) * bpb->sector_p_clust; i++)
	{
		if (dirs[i].name[0] == '\0') continue;

		if (memcmp((char *)dirs[i].name, filename, FAT16STR_SIZE) == 0)
		{
			res.found = true;
			res.fdir  = dirs[i];
			res.idx   = i;
			break;
		}
	}

	return res;
}

/*
 * Função de ls
 * Alteração para FAT32: leitura agora considera clusters da raiz.
 */
struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
	uint32_t cluster = bpb->root_cluster; /* Começa no cluster raiz */
	uint32_t root_size = bpb->bytes_p_sect * bpb->sector_p_clust;

	struct fat_dir *dirs = malloc(root_size);
	if (!dirs)
	{
		perror("Erro de alocação de memória");
		exit(EXIT_FAILURE);
	}

	/* Calcula o endereço do primeiro setor do cluster */
	uint32_t root_sector = fat32_first_sector_of_cluster(bpb, cluster);

	/* Lê os dados do diretório raiz */
	if (read_bytes(fp, root_sector, dirs, root_size) != RB_OK)
	{
		perror("Erro ao ler o diretório raiz");
		free(dirs);
		exit(EXIT_FAILURE);
	}

	return dirs;
}

/*
 * Função de mv
 * Alteração para FAT32: ajustado para usar clusters de 32 bits.
 */
void mv(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
	char source_rname[FAT16STR_SIZE_WNULL], dest_rname[FAT16STR_SIZE_WNULL];

	bool badname = cstr_to_fat16wnull(source, source_rname)
	            || cstr_to_fat16wnull(dest, dest_rname);

	if (badname)
	{
		fprintf(stderr, "Nome de arquivo inválido.\n");
		exit(EXIT_FAILURE);
	}

	uint32_t cluster = bpb->root_cluster;
	uint32_t root_sector = fat32_first_sector_of_cluster(bpb, cluster);
	uint32_t root_size = bpb->bytes_p_sect * bpb->sector_p_clust;

	struct fat_dir root[root_size];
	if (read_bytes(fp, root_sector, root, root_size) != RB_OK)
		error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler diretório raiz");

	struct far_dir_searchres dir1 = find_in_root(root, source_rname, bpb);
	struct far_dir_searchres dir2 = find_in_root(root, dest_rname, bpb);

	if (dir2.found)
		error(EXIT_FAILURE, 0, "Arquivo %s já existe no destino.", dest);

	if (!dir1.found)
		error(EXIT_FAILURE, 0, "Arquivo %s não encontrado.", source);

	memcpy(dir1.fdir.name, dest_rname, FAT16STR_SIZE);

	uint32_t source_address = root_sector + dir1.idx * sizeof(struct fat_dir);

	(void)fseek(fp, source_address, SEEK_SET);
	(void)fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp);

	printf("mv %s → %s.\n", source, dest);

	return;
}

/*
 * Função de rm
 * Alteração para FAT32: usa clusters de 32 bits e `fat32_next_cluster`.
 */
void rm(FILE *fp, char *filename, struct fat_bpb *bpb)
{
	char fat16_rname[FAT16STR_SIZE_WNULL];

	if (cstr_to_fat16wnull(filename, fat16_rname))
	{
		fprintf(stderr, "Nome de arquivo inválido.\n");
		exit(EXIT_FAILURE);
	}

	uint32_t cluster = bpb->root_cluster;
	uint32_t root_sector = fat32_first_sector_of_cluster(bpb, cluster);
	uint32_t root_size = bpb->bytes_p_sect * bpb->sector_p_clust;

	struct fat_dir root[root_size];
	if (read_bytes(fp, root_sector, root, root_size) != RB_OK)
		error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler diretório raiz");

	struct far_dir_searchres dir = find_in_root(root, fat16_rname, bpb);

	if (!dir.found)
		error(EXIT_FAILURE, 0, "Arquivo %s não encontrado.", filename);

	dir.fdir.name[0] = DIR_FREE_ENTRY;

	uint32_t file_address = root_sector + dir.idx * sizeof(struct fat_dir);
	(void)fseek(fp, file_address, SEEK_SET);
	(void)fwrite(&dir.fdir, sizeof(struct fat_dir), 1, fp);

	uint32_t fat_address = bpb_faddress(bpb);
	uint32_t cluster_number = (dir.fdir.starting_cluster_low | (dir.fdir.reserved_fat32 << 16));
	uint32_t null = 0x0;

	while (cluster_number < FAT32_EOF_LO)
	{
		uint32_t cluster_entry_address = fat_address + cluster_number * 4;
		read_bytes(fp, cluster_entry_address, &cluster_number, sizeof(cluster_number));
		cluster_number &= 0x0FFFFFFF;

		(void)fseek(fp, cluster_entry_address, SEEK_SET);
		(void)fwrite(&null, sizeof(uint32_t), 1, fp);
	}

	printf("rm %s concluído.\n", filename);
	return;
}

void cp(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
    /* Manipulação de diretório explicado em mv() */
    char source_rname[FAT16STR_SIZE_WNULL], dest_rname[FAT16STR_SIZE_WNULL];

    bool badname = cstr_to_fat16wnull(source, source_rname)
                || cstr_to_fat16wnull(dest, dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t cluster = bpb->root_cluster; /* Alteração: Usa cluster raiz do FAT32 */
    uint32_t root_sector = fat32_first_sector_of_cluster(bpb, cluster); /* Alteração: Calcula o setor inicial do cluster raiz */
    uint32_t root_size = bpb->bytes_p_sect * bpb->sector_p_clust;

    struct fat_dir root[root_size];
    if (read_bytes(fp, root_sector, root, root_size) != RB_OK)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler diretório raiz");

    struct far_dir_searchres dir1 = find_in_root(root, source_rname, bpb);

    if (!dir1.found)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", source);

    if (find_in_root(root, dest_rname, bpb).found)
        error(EXIT_FAILURE, 0, "Arquivo %s já existe no destino.", dest);

    struct fat_dir new_dir = dir1.fdir;
    memcpy(new_dir.name, dest_rname, FAT16STR_SIZE);

    /* Dentry */

    bool dentry_failure = true;

    /* Procura-se uma entrada livre no diretório raíz */
    for (size_t i = 0; i < bpb->bytes_p_sect / sizeof(struct fat_dir) * bpb->sector_p_clust; i++)
    {
        if (root[i].name[0] == DIR_FREE_ENTRY || root[i].name[0] == '\0')
        {
            uint32_t dest_address = root_sector + i * sizeof(struct fat_dir);

            /* Aplica new_dir ao diretório raiz */
            (void)fseek(fp, dest_address, SEEK_SET);
            (void)fwrite(&new_dir, sizeof(struct fat_dir), 1, fp);

            dentry_failure = false;
            break;
        }
    }

    if (dentry_failure)
        error_at_line(EXIT_FAILURE, ENOSPC, __FILE__, __LINE__, "Não foi possível alocar uma entrada no diretório raiz.");

    /* Agora é necessário alocar os clusters para o novo arquivo. */

    int count = 0;

    /* Clusters */
    {
        /*
         * Informações de novo cluster
         *
         * É alocado os clusters de trás para frente; o último é alocado primeiro,
         * primariamente devido à necessidade de seu valor ser FAT32_EOF_HI.
         */
        uint32_t source_cluster = (dir1.fdir.starting_cluster_low | (dir1.fdir.reserved_fat32 << 16)); /* Alteração: Cluster de 32 bits */
        uint32_t prev_cluster = FAT32_EOF_HI;

        /* Quantos clusters o arquivo necessita */
        uint32_t cluster_count = dir1.fdir.file_size / (bpb->bytes_p_sect * bpb->sector_p_clust) + 1;

        /* Aloca-se os clusters, gravando-os na FAT */
        while (cluster_count--)
        {
            uint32_t free_cluster = fat32_find_free_cluster(fp, bpb).cluster; /* Alteração: Usa fat32_find_free_cluster */
            if (free_cluster == 0x0)
                error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Disco cheio (imagem foi corrompida)");

            if (prev_cluster != FAT32_EOF_HI)
            {
                uint32_t fat_entry_address = bpb_faddress(bpb) + prev_cluster * 4; /* Alteração: FAT32 usa 4 bytes por entrada */
                (void)fseek(fp, fat_entry_address, SEEK_SET);
                (void)fwrite(&free_cluster, sizeof(uint32_t), 1, fp);
            }

            prev_cluster = free_cluster;

            /* Copiar os dados */
            uint32_t source_address = fat32_first_sector_of_cluster(bpb, source_cluster);
            uint32_t dest_address = fat32_first_sector_of_cluster(bpb, free_cluster);

            size_t bytes_in_sector = MIN(new_dir.file_size, bpb->bytes_p_sect * bpb->sector_p_clust);

            char buffer[bpb->bytes_p_sect * bpb->sector_p_clust];
            read_bytes(fp, source_address, buffer, bytes_in_sector);
            (void)fseek(fp, dest_address, SEEK_SET);
            (void)fwrite(buffer, bytes_in_sector, 1, fp);

            new_dir.file_size -= bytes_in_sector;
            source_cluster = fat32_next_cluster(fp, bpb, source_cluster); /* Alteração: Usa fat32_next_cluster */

            count++;
        }

        /* Marca o último cluster como EOF */
        uint32_t fat_entry_address = bpb_faddress(bpb) + prev_cluster * 4; /* Alteração: FAT32 usa 4 bytes por entrada */
        uint32_t eof_marker = FAT32_EOF_HI; /* Armazena o valor de FAT32_EOF_HI em uma variável */
        (void)fseek(fp, fat_entry_address, SEEK_SET);
        (void)fwrite(&eof_marker, sizeof(uint32_t), 1, fp); /* Passa o endereço da variável para fwrite */
    }

    printf("cp %s → %s, %i clusters copiados.\n", source, dest, count);

    return;
}

void cat(FILE *fp, char *filename, struct fat_bpb *bpb)
{
    /*
     * Leitura do diretório raiz explicado em mv().
     */

    char rname[FAT16STR_SIZE_WNULL];

    bool badname = cstr_to_fat16wnull(filename, rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t cluster = bpb->root_cluster; /* Alteração: Usa o cluster raiz do FAT32 */
    uint32_t root_sector = fat32_first_sector_of_cluster(bpb, cluster); /* Alteração: Calcula o setor inicial do cluster raiz */
    uint32_t root_size = bpb->bytes_p_sect * bpb->sector_p_clust;

    struct fat_dir root[root_size];
    if (read_bytes(fp, root_sector, root, root_size) != RB_OK)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler diretório raiz");

    struct far_dir_searchres dir = find_in_root(root, rname, bpb);

    if (!dir.found)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o %s.", filename);

    /*
     * Descobre-se quantos bytes o arquivo tem
     */
    size_t bytes_to_read = dir.fdir.file_size;

    /*
     * O primeiro cluster do arquivo está guardado na struct fat_dir.
     */
    uint32_t cluster_number = (dir.fdir.starting_cluster_low | (dir.fdir.reserved_fat32 << 16)); /* Alteração: Cluster de 32 bits */

    const uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;

    while (bytes_to_read != 0)
    {
        /* Onde em disco está o cluster atual */
        uint32_t cluster_address = fat32_first_sector_of_cluster(bpb, cluster_number); /* Alteração: Usa fat32_first_sector_of_cluster */

        /* Devemos ler no máximo cluster_width. */
        size_t read_in_this_sector = MIN(bytes_to_read, cluster_width);

        char buffer[cluster_width];

        /* Lemos o cluster atual */
        read_bytes(fp, cluster_address, buffer, read_in_this_sector);
        printf("%.*s", (signed)read_in_this_sector, buffer);

        bytes_to_read -= read_in_this_sector;

        /* Calcula o próximo cluster */
        cluster_number = fat32_next_cluster(fp, bpb, cluster_number); /* Alteração: Usa fat32_next_cluster */
    }

    return;
}


