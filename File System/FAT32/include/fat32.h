#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define DIR_FREE_ENTRY 0xE5

#define DIR_ATTR_READONLY 1 << 0 /* file is read only */
#define DIR_ATTR_HIDDEN 1 << 1 /* file is hidden */
#define DIR_ATTR_SYSTEM 1 << 2 /* system file (also hidden) */
#define DIR_ATTR_VOLUMEID 1 << 3 /* special entry containing disk volume label */
#define DIR_ATTR_DIRECTORY 1 << 4 /* describes a subdirectory */
#define DIR_ATTR_ARCHIVE 1 << 5 /* archive flag (always set when file is modified */
#define DIR_ATTR_LFN 0xf /* not used */

#define SIG 0xAA55 /* boot sector signature -- sector is executable */

#pragma pack(push, 1)
/* Directory entry structure for FAT32 */
struct fat_dir {
    unsigned char name[11]; /* Short name + file extension */
    uint8_t attr; /* File attributes */
    uint8_t ntres; /* Reserved for Windows NT, set value to 0 when a file is created */
    uint8_t creation_stamp; /* Millisecond timestamp at file creation time */
    uint16_t creation_time; /* Time file was created */
    uint16_t creation_date; /* Date file was created */
    uint16_t last_access_date; /* Last access date (last read/written) */
    uint16_t starting_cluster_high; /* High 16 bits of the starting cluster */
    uint16_t last_write_time; /* Time of last write */
    uint16_t last_write_date; /* Date of last write */
    uint16_t starting_cluster_low; /* Low 16 bits of the starting cluster */
    uint32_t file_size; /* File size in bytes */
};

/* Boot Sector and BPB structure for FAT32 */
struct fat_bpb {
    uint8_t jmp_instruction[3]; /* Code to jump to bootstrap */
    unsigned char oem_id[8]; /* OEM ID: name of the formatting OS */

    uint16_t bytes_p_sect; /* Bytes per sector */
    uint8_t sector_p_clust; /* Sectors per cluster */
    uint16_t reserved_sect; /* Reserved sectors */
    uint8_t n_fat; /* Number of FAT copies */
    uint16_t root_entry_count; /* Always 0 in FAT32 */
    uint16_t snumber_sect; /* Small number of sectors (used in FAT12/16) */

    uint8_t media_desc; /* Media descriptor */
    uint16_t sect_per_track; /* Sectors per track */
    uint16_t number_of_heads; /* Number of heads */
    uint32_t hidden_sects; /* Hidden sectors */
    uint32_t large_n_sects; /* Total number of sectors */

    uint32_t sect_per_fat_32; /* Sectors per FAT in FAT32 */
    uint32_t root_cluster; /* Initial cluster of the root directory */
    uint16_t fs_info; /* File system information sector */
    uint16_t backup_boot_sector; /* Backup boot sector location */
    uint8_t reserved[12]; /* Reserved for future use */

    uint8_t drive_number; /* Drive number (logical) */
    uint8_t reserved1; /* Reserved */
    uint8_t boot_signature; /* Boot signature (0x29 for valid FAT32) */
    uint32_t volume_id; /* Volume ID */
    unsigned char volume_label[11]; /* Volume label */
    unsigned char fs_type[8]; /* File system type ("FAT32") */

    uint8_t boot_code[420]; /* Bootstrap code */
    uint16_t signature; /* Signature (0xAA55) */
};
#pragma pack(pop)

int read_bytes(FILE *, unsigned int, void *, unsigned int);
void rfat(FILE *, struct fat_bpb *);

/* Prototypes for calculating FAT32 addresses */
uint32_t bpb_faddress(struct fat_bpb *);
uint32_t bpb_froot_addr(struct fat_bpb *);
uint32_t bpb_fdata_addr(struct fat_bpb *);
uint32_t bpb_fdata_sector_count(struct fat_bpb *);
uint32_t bpb_fdata_cluster_count(struct fat_bpb *);

#define FAT32STR_SIZE       11
#define FAT32STR_SIZE_WNULL 12

#define RB_ERROR -1
#define RB_OK     0

#define FAT32_EOF_LO 0x0FFFFFF8
#define FAT32_EOF_HI 0x0FFFFFFF

#endif
