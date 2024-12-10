#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define DIR_FREE_ENTRY 0xE5

#define DIR_ATTR_READONLY 1 << 0 /* file is read only */
#define DIR_ATTR_HIDDEN 1 << 1 /* file is hidden */
#define DIR_ATTR_SYSTEM 1 << 2 /* system file (also hidden) */
#define DIR_ATTR_VOLUMEID 1 << 3 /* special entry containing disk volume lable */
#define DIR_ATTR_DIRECTORY 1 << 4 /* describes a subdirectory */
#define DIR_ATTR_ARCHIVE 1 << 5 /* archive flag (always set when file is modified */
#define DIR_ATTR_LFN 0xf /* not used */

#define SIG 0xAA55 /* boot sector signature -- sector is executable */

#pragma pack(push, 1)
/* Estrutura do diretório: sem mudanças para FAT32 */
struct fat_dir {
    unsigned char name[11]; /* Short name + file extension */
    uint8_t attr; /* file attributes */
    uint8_t ntres; /* reserved for Windows NT, Set value to 0 when a file is created. */
    uint8_t creation_stamp; /* milisecond timestamp at file creation time */
    uint16_t creation_time; /* time file was created */
    uint16_t ctreation_date; /* date file was created */
    uint16_t last_access_date; /* last access date (last read/written) */
    uint16_t reserved_fat32; /* reserved for fat32 */
    uint16_t last_write_time; /* time of last write */
    uint16_t last_write_date; /* date of last write */
    uint16_t starting_cluster_low; /* FAT32: renamed for clarity */
    uint32_t file_size; /* 32-bit */
};

struct fat_bpb { /* bios Parameter block */
    uint8_t jmp_instruction[3]; /* code to jump to the bootstrap code */
    unsigned char oem_id[8]; /* Oem ID: name of the formatting OS */

    uint16_t bytes_p_sect; /* bytes per sector */
    uint8_t sector_p_clust; /* sector per cluster */
    uint16_t reserved_sect; /* reserved sectors */
    uint8_t n_fat; /* number of FAT copies */
    uint16_t possible_rentries; /* number of possible root entries (FAT16 only) */
    uint16_t snumber_sect; /* small number of sectors */

    uint8_t media_desc; /* media descriptor */
    uint16_t sect_per_fat; /* sector per FAT (FAT16 only) */
    uint16_t sect_per_track; /* sector per track */
    uint16_t number_of_heads; /* number of heads */
    uint32_t hidden_sects; /* hidden sectors */
    uint32_t large_n_sects; /* large number of sectors */

    /* Alterações para FAT32 */
    uint32_t sect_per_fat32; /* FAT32: sectors per FAT (32-bit) */
    uint16_t ext_flags; /* FAT32: extended flags */
    uint16_t fs_version; /* FAT32: filesystem version */
    uint32_t root_cluster; /* FAT32: root directory cluster */
    uint16_t fs_info; /* FAT32: FSInfo sector */
    uint16_t boot_sector_backup; /* FAT32: backup boot sector */
    uint8_t reserved[12]; /* FAT32: reserved for future use */
};
#pragma pack(pop)

int read_bytes(FILE *, unsigned int, void *, unsigned int);
void rfat(FILE *, struct fat_bpb *);

/* prototypes for calculating fat stuff */
uint32_t bpb_faddress(struct fat_bpb *); /* FAT address calculation */
uint32_t bpb_froot_addr(struct fat_bpb *); /* Root directory address (updated for FAT32) */
uint32_t bpb_fdata_addr(struct fat_bpb *); /* Data region address calculation */
uint32_t bpb_fdata_sector_count(struct fat_bpb *); /* Number of data sectors */
uint32_t bpb_fdata_cluster_count(struct fat_bpb *); /* Number of data clusters */

/* Novos protótipos para FAT32 */
uint32_t fat32_first_sector_of_cluster(struct fat_bpb *, uint32_t cluster); /* Cluster to sector conversion */
uint32_t fat32_next_cluster(FILE *, struct fat_bpb *, uint32_t cluster); /* Get next cluster from FAT */

/* Ajustes em EOF para FAT32 */
#define FAT32_EOF_LO 0x0FFFFFF8 /* FAT32: low EOF marker */
#define FAT32_EOF_HI 0x0FFFFFFF /* FAT32: high EOF marker */

/* Strings em FAT32: mantidas as definições originais */
#define FAT16STR_SIZE       11 /* No FAT32, o nome curto também é de 11 bytes */
#define FAT16STR_SIZE_WNULL 12 /* Nome curto com caractere nulo */

/* Códigos de retorno: mantidos */
#define RB_ERROR -1
#define RB_OK     0

#endif
