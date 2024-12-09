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
 * Função de busca na pasta raíz. Codigo original do professor,
 * altamente modificado.
 *
 * Ela itera sobre todas as bpb->possible_rentries do struct fat_dir* dirs, e
 * retorna a primeira entrada com nome igual à filename.
 */
struct far_dir_searchres find_in_root(FILE *fp, char *filename, struct fat_bpb *bpb)
{
    struct far_dir_searchres res = { .found = false };

    uint32_t root_cluster = bpb->root_cluster;
    uint32_t cluster_size = bpb->bytes_p_sect * bpb->sector_p_clust;

    while (root_cluster < FAT32_EOF_LO)
    {
        uint32_t cluster_address = bpb_fdata_addr(bpb) + (root_cluster - 2) * cluster_size;
        struct fat_dir dirs[cluster_size / sizeof(struct fat_dir)];

        read_bytes(fp, cluster_address, dirs, cluster_size);

        for (size_t i = 0; i < cluster_size / sizeof(struct fat_dir); i++)
        {
            if (dirs[i].name[0] == '\0') continue;

            if (memcmp((char *)dirs[i].name, filename, FAT32STR_SIZE) == 0)
            {
                res.found = true;
                res.fdir = dirs[i];
                res.idx = i;
                return res;
            }
        }

        // Passa para o próximo cluster
        uint32_t next_cluster_address = bpb_faddress(bpb) + root_cluster * sizeof(uint32_t);
        read_bytes(fp, next_cluster_address, &root_cluster, sizeof(uint32_t));
    }

    return res;
}


/*
 * Função de ls
 *
 * Ela itéra todas as bpb->possible_rentries do diretório raiz
 * e as lê via read_bytes().
 */
struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
    uint32_t root_cluster = bpb->root_cluster;
    uint32_t cluster_size = bpb->bytes_p_sect * bpb->sector_p_clust;

    struct fat_dir *dirs = malloc(cluster_size);

    uint32_t cluster_address = bpb_fdata_addr(bpb) + (root_cluster - 2) * cluster_size;

    read_bytes(fp, cluster_address, dirs, cluster_size);

    return dirs;
}

void mv(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
    // Converte os nomes de arquivo para o formato FAT32
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    if (cstr_to_fat32wnull(source, source_rname) || cstr_to_fat32wnull(dest, dest_rname)) {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Procura o arquivo fonte no diretório raiz
    struct far_dir_searchres src_res = find_in_root(fp, source_rname, bpb);
    if (!src_res.found) {
        fprintf(stderr, "Arquivo %s não encontrado.\n", source);
        exit(EXIT_FAILURE);
    }

    // Procura se o destino já existe
    struct far_dir_searchres dest_res = find_in_root(fp, dest_rname, bpb);
    if (dest_res.found) {
        fprintf(stderr, "Arquivo de destino %s já existe.\n", dest);
        exit(EXIT_FAILURE);
    }

    // Renomeia o arquivo
    memcpy(src_res.fdir.name, dest_rname, FAT32STR_SIZE);

    // Calcula o endereço físico da entrada no diretório
    uint32_t cluster_size = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster_address = bpb_fdata_addr(bpb) + (bpb->root_cluster - 2) * cluster_size;
    uint32_t entry_address = cluster_address + src_res.idx * sizeof(struct fat_dir);

    fseek(fp, entry_address, SEEK_SET);
    fwrite(&src_res.fdir, sizeof(struct fat_dir), 1, fp);

    printf("mv %s → %s concluído.\n", source, dest);
}


void rm(FILE *fp, char *filename, struct fat_bpb *bpb)
{
    // Converte o nome do arquivo para o formato FAT32
    char fat32_rname[FAT32STR_SIZE_WNULL];
    if (cstr_to_fat32wnull(filename, fat32_rname)) {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Procura o arquivo no diretório raiz
    struct far_dir_searchres file_res = find_in_root(fp, fat32_rname, bpb);
    if (!file_res.found) {
        fprintf(stderr, "Arquivo %s não encontrado.\n", filename);
        exit(EXIT_FAILURE);
    }

    // Marca a entrada como livre
    file_res.fdir.name[0] = DIR_FREE_ENTRY;

    uint32_t cluster_size = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster_address = bpb_fdata_addr(bpb) + (bpb->root_cluster - 2) * cluster_size;
    uint32_t entry_address = cluster_address + file_res.idx * sizeof(struct fat_dir);

    fseek(fp, entry_address, SEEK_SET);
    fwrite(&file_res.fdir, sizeof(struct fat_dir), 1, fp);

    // Libera os clusters associados ao arquivo
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t cluster = file_res.fdir.starting_cluster_low | (file_res.fdir.starting_cluster_high << 16);

    while (cluster < FAT32_EOF_LO) {
        uint32_t next_cluster;
        uint32_t entry_address = fat_address + cluster * sizeof(uint32_t);

        read_bytes(fp, entry_address, &next_cluster, sizeof(uint32_t));

        // Marca o cluster atual como livre
        uint32_t free_cluster = 0x0;
        fseek(fp, entry_address, SEEK_SET);
        fwrite(&free_cluster, sizeof(uint32_t), 1, fp);

        cluster = next_cluster & 0x0FFFFFFF;
    }

    printf("rm %s concluído.\n", filename);
}


struct fat32_newcluster_info fat32_find_free_cluster(FILE* fp, struct fat_bpb* bpb)
{
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t total_clusters = bpb_fdata_cluster_count(bpb);

    for (uint32_t cluster = 2; cluster < total_clusters; cluster++)
    {
        uint32_t entry;
        uint32_t entry_address = fat_address + cluster * sizeof(uint32_t);

        read_bytes(fp, entry_address, &entry, sizeof(uint32_t));

        if ((entry & 0x0FFFFFFF) == 0x0)
        {
            return (struct fat32_newcluster_info) { .cluster = cluster, .address = entry_address };
        }
    }

    return (struct fat32_newcluster_info) { .cluster = 0 };
}


void cp(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
    // Converte os nomes de arquivo para o formato FAT32
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];
    if (cstr_to_fat32wnull(source, source_rname) || cstr_to_fat32wnull(dest, dest_rname)) {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Procura o arquivo fonte no diretório raiz
    struct far_dir_searchres src_res = find_in_root(fp, source_rname, bpb);
    if (!src_res.found) {
        fprintf(stderr, "Arquivo %s não encontrado.\n", source);
        exit(EXIT_FAILURE);
    }

    // Verifica se o arquivo de destino já existe
    struct far_dir_searchres dest_res = find_in_root(fp, dest_rname, bpb);
    if (dest_res.found) {
        fprintf(stderr, "Arquivo de destino %s já existe.\n", dest);
        exit(EXIT_FAILURE);
    }

    // Aloca clusters para o novo arquivo
    struct fat32_newcluster_info new_cluster = fat32_find_free_cluster(fp, bpb);
    uint32_t cluster = new_cluster.cluster;

    if (cluster == 0) {
        fprintf(stderr, "Disco cheio.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t src_cluster = src_res.fdir.starting_cluster_low | (src_res.fdir.starting_cluster_high << 16);
    uint32_t bytes_to_copy = src_res.fdir.file_size;

    while (bytes_to_copy > 0) {
        // Copia os dados do cluster
        uint32_t cluster_size = bpb->bytes_p_sect * bpb->sector_p_clust;
        uint32_t src_address = bpb_fdata_addr(bpb) + (src_cluster - 2) * cluster_size;
        uint32_t dest_address = bpb_fdata_addr(bpb) + (cluster - 2) * cluster_size;

        char buffer[cluster_size];
        size_t bytes_in_this_cluster = (bytes_to_copy > cluster_size) ? cluster_size : bytes_to_copy;

        read_bytes(fp, src_address, buffer, bytes_in_this_cluster);
        fseek(fp, dest_address, SEEK_SET);
        fwrite(buffer, 1, bytes_in_this_cluster, fp);

        bytes_to_copy -= bytes_in_this_cluster;

        // Atualiza FAT
        uint32_t next_cluster;
        uint32_t src_fat_address = fat_address + src_cluster * sizeof(uint32_t);
        read_bytes(fp, src_fat_address, &next_cluster, sizeof(uint32_t));

        uint32_t new_next_cluster = 0x0FFFFFFF; // EOF
        fseek(fp, fat_address + cluster * sizeof(uint32_t), SEEK_SET);
        fwrite(&new_next_cluster, sizeof(uint32_t), 1, fp);

        src_cluster = next_cluster & 0x0FFFFFFF;
        if (bytes_to_copy > 0) {
            new_cluster = fat32_find_free_cluster(fp, bpb);
            cluster = new_cluster.cluster;
        }
    }

    printf("cp %s → %s concluído.\n", source, dest);
}


void cat(FILE *fp, char *filename, struct fat_bpb *bpb)
{
    // Converte o nome do arquivo para o formato FAT32
    char fat32_rname[FAT32STR_SIZE_WNULL];
    if (cstr_to_fat32wnull(filename, fat32_rname)) {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Procura o arquivo no diretório raiz
    struct far_dir_searchres file_res = find_in_root(fp, fat32_rname, bpb);
    if (!file_res.found) {
        fprintf(stderr, "Arquivo %s não encontrado.\n", filename);
        exit(EXIT_FAILURE);
    }

    uint32_t cluster_size = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster = file_res.fdir.starting_cluster_low | (file_res.fdir.starting_cluster_high << 16);
    uint32_t bytes_to_read = file_res.fdir.file_size;

    while (bytes_to_read > 0) {
        uint32_t cluster_address = bpb_fdata_addr(bpb) + (cluster - 2) * cluster_size;
        char buffer[cluster_size];
        size_t bytes_in_this_cluster = (bytes_to_read > cluster_size) ? cluster_size : bytes_to_read;

        read_bytes(fp, cluster_address, buffer, bytes_in_this_cluster);
        printf("%.*s", (int)bytes_in_this_cluster, buffer);

        bytes_to_read -= bytes_in_this_cluster;

        // Passa para o próximo cluster
        uint32_t fat_address = bpb_faddress(bpb) + cluster * sizeof(uint32_t);
        read_bytes(fp, fat_address, &cluster, sizeof(uint32_t));
        cluster &= 0x0FFFFFFF;
    }
}
