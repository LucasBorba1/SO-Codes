#include "fat32.h"
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <err.h>

/* Calculate FAT address */
uint32_t bpb_faddress(struct fat_bpb *bpb) {
    return bpb->reserved_sect * bpb->bytes_p_sect;
}

/* Calculate root directory address */
uint32_t bpb_froot_addr(struct fat_bpb *bpb) {
    uint32_t root_sector = (bpb->root_cluster - 2) * bpb->sector_p_clust;
    return bpb_fdata_addr(bpb) + root_sector * bpb->bytes_p_sect;
}

/* Calculate data region address */
uint32_t bpb_fdata_addr(struct fat_bpb *bpb) {
    return bpb_faddress(bpb) + bpb->n_fat * bpb->sect_per_fat_32 * bpb->bytes_p_sect;
}

/* Calculate total data sector count */
uint32_t bpb_fdata_sector_count(struct fat_bpb *bpb) {
    return bpb->large_n_sects - bpb_fdata_addr(bpb) / bpb->bytes_p_sect;
}

/* Calculate total data cluster count */
uint32_t bpb_fdata_cluster_count(struct fat_bpb *bpb) {
    uint32_t sectors = bpb_fdata_sector_count(bpb);
    return sectors / bpb->sector_p_clust;
}

/*
 * Allows reading from a specific offset and writing the data to buff
 * Returns RB_ERROR if seeking or reading failed, and RB_OK on success
 */
int read_bytes(FILE *fp, unsigned int offset, void *buff, unsigned int len) {
    if (fseek(fp, offset, SEEK_SET) != 0) {
        error_at_line(0, errno, __FILE__, __LINE__, "warning: error when seeking to %u", offset);
        return RB_ERROR;
    }
    if (fread(buff, 1, len, fp) != len) {
        error_at_line(0, errno, __FILE__, __LINE__, "warning: error reading file");
        return RB_ERROR;
    }

    return RB_OK;
}

/* Read FAT32's BIOS parameter block */
void rfat(FILE *fp, struct fat_bpb *bpb) {
    read_bytes(fp, 0x0, bpb, sizeof(struct fat_bpb));
    return;
}
