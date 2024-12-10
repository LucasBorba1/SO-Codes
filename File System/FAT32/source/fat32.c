#include "fat32.h"
#include "commands.h"
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <err.h>

/* calculate FAT address */
uint32_t bpb_faddress(struct fat_bpb *bpb)
{
    /* Sem alterações: o cálculo do início da FAT permanece o mesmo */
    return bpb->reserved_sect * bpb->bytes_p_sect;
}

/* calculate FAT root address */
uint32_t bpb_froot_addr(struct fat_bpb *bpb)
{
    /*
     * Alteração para FAT32:
     * O diretório raiz não está mais em uma área separada.
     * Agora começa no cluster indicado por `root_cluster`.
     * O cálculo abaixo considera o endereço do cluster raiz.
     */
    return bpb_fdata_addr(bpb) + (bpb->root_cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
}

/* calculate data address */
uint32_t bpb_fdata_addr(struct fat_bpb *bpb)
{
    /*
     * Alteração para FAT32:
     * O endereço da região de dados agora ignora `possible_rentries`
     * e considera a FAT estendida (sect_per_fat32).
     */
    return bpb_faddress(bpb) + bpb->n_fat * bpb->sect_per_fat32 * bpb->bytes_p_sect;
}

/* calculate data sector count */
uint32_t bpb_fdata_sector_count(struct fat_bpb *bpb)
{
    /* Sem alterações significativas, mas agora usamos `large_n_sects` */
    return bpb->large_n_sects - bpb_fdata_addr(bpb) / bpb->bytes_p_sect;
}

/* calculate data sector count for small sector counts */
static uint32_t bpb_fdata_sector_count_s(struct fat_bpb *bpb)
{
    /* Sem alterações significativas, mas agora usamos `snumber_sect` */
    return bpb->snumber_sect - bpb_fdata_addr(bpb) / bpb->bytes_p_sect;
}

/* calculate data cluster count */
uint32_t bpb_fdata_cluster_count(struct fat_bpb *bpb)
{
    /* Sem alterações: continua dividindo os setores pela cluster size */
    uint32_t sectors = bpb_fdata_sector_count_s(bpb);
    return sectors / bpb->sector_p_clust;
}

/* allows reading from a specific offset and writing the data to buffer */
int read_bytes(FILE *fp, unsigned int offset, void *buff, unsigned int len)
{
    /* Sem alterações: lógica de leitura permanece inalterada */
    if (fseek(fp, offset, SEEK_SET) != 0)
    {
        error_at_line(0, errno, __FILE__, __LINE__, "warning: error when seeking to %u", offset);
        return RB_ERROR;
    }
    if (fread(buff, 1, len, fp) != len)
    {
        error_at_line(0, errno, __FILE__, __LINE__, "warning: error reading file");
        return RB_ERROR;
    }

    return RB_OK;
}

/* read fat32's BIOS Parameter Block */
void rfat(FILE *fp, struct fat_bpb *bpb)
{
    /* Sem alterações: ainda lemos o BPB da mesma forma */
    read_bytes(fp, 0x0, bpb, sizeof(struct fat_bpb));
    return;
}

/* get the first sector of a cluster */
uint32_t fat32_first_sector_of_cluster(struct fat_bpb *bpb, uint32_t cluster)
{
    /*
     * Função adicionada para FAT32:
     * Converte um número de cluster em um setor absoluto.
     */
    return bpb_fdata_addr(bpb) + (cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
}

/* get the next cluster in the FAT chain */
uint32_t fat32_next_cluster(FILE *fp, struct fat_bpb *bpb, uint32_t cluster)
{
    /*
     * Função adicionada para FAT32:
     * Lê a FAT para encontrar o próximo cluster no encadeamento.
     */
    uint32_t fat_offset = cluster * 4; /* FAT32 usa entradas de 4 bytes */
    uint32_t fat_sector = bpb->reserved_sect + (fat_offset / bpb->bytes_p_sect);
    uint32_t fat_entry_offset = fat_offset % bpb->bytes_p_sect;

    uint32_t next_cluster;
    read_bytes(fp, fat_sector * bpb->bytes_p_sect + fat_entry_offset, &next_cluster, sizeof(next_cluster));

    return next_cluster & 0x0FFFFFFF; /* Máscara para obter os 28 bits do cluster */
}

struct fat32_newcluster_info fat32_find_free_cluster(FILE* fp, struct fat_bpb* bpb)
{
    uint32_t cluster = 2; /* FAT32 começa no cluster 2 */
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t total_clusters = bpb_fdata_cluster_count(bpb);

    for (; cluster < total_clusters; cluster++)
    {
        uint32_t entry;
        uint32_t entry_address = fat_address + cluster * 4;

        read_bytes(fp, entry_address, &entry, sizeof(uint32_t));
        entry &= 0x0FFFFFFF; /* Máscara para FAT32 */

        if (entry == 0x0) /* Cluster livre */
        {
            /* Usar uma variável local para construir o resultado */
            struct fat32_newcluster_info result;
            result.cluster = cluster;
            result.address = entry_address;
            return result;
        }
    }

    /* Nenhum cluster livre encontrado */
    struct fat32_newcluster_info result = {0};
    return result;
}
