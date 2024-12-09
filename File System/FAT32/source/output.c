#include "output.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static struct pretty_int { int num; char* suff; } pretty_print(int value)
{
    static char* B   = "B";
    static char* KiB = "KiB";
    static char* MiB = "MiB";

    struct pretty_int res =
    {
        .num  = value,
        .suff = B
    };

    if (res.num == 0) return res;

    if (res.num >= 1024)
    {
        res.num  /= 1024;
        res.suff = KiB;
    }

    if (res.num >= 1024)
    {
        res.num  /= 1024;
        res.suff = MiB;
    }

    return res;
}

void show_files(struct fat_dir *dirs)
{
    struct fat_dir *cur;

    fprintf(stdout, "ATTR  NAME           SIZE     CLUSTER\n---------------------------------------\n");

    while ((cur = dirs++) != NULL)
    {
        if (cur->name[0] == 0) // Fim das entradas
            break;

        if ((cur->name[0] == DIR_FREE_ENTRY) || (cur->attr == DIR_FREE_ENTRY)) // Entrada livre
            continue;

        if (cur->attr == DIR_ATTR_LFN) // Entrada de nome longo (ignorar)
            continue;

        uint32_t cluster = cur->starting_cluster_low | (cur->starting_cluster_high << 16);

        struct pretty_int num = pretty_print(cur->file_size);

        fprintf(stdout, "0x%-2x  %-11.11s  %4i %s   0x%x\n",
                cur->attr,
                cur->name,
                num.num,
                num.suff,
                cluster);
    }
}

void verbose(struct fat_bpb *bios_pb)
{
    fprintf(stdout, "Bios Parameter Block (FAT32):\n");
    fprintf(stdout, "Jump Instruction: ");

    for (int i = 0; i < 3; i++)
        fprintf(stdout, "%hhX ", bios_pb->jmp_instruction[i]);

    fprintf(stdout, "\n");

    fprintf(stdout, "OEM ID: %s\n", bios_pb->oem_id);
    fprintf(stdout, "Bytes per Sector: %d\n", bios_pb->bytes_p_sect);
    fprintf(stdout, "Sectors per Cluster: %d\n", bios_pb->sector_p_clust);
    fprintf(stdout, "Reserved Sectors: %d\n", bios_pb->reserved_sect);
    fprintf(stdout, "Number of FATs: %d\n", bios_pb->n_fat);
    fprintf(stdout, "Total Sectors (32-bit): %d\n", bios_pb->large_n_sects);
    fprintf(stdout, "Sectors per FAT (32-bit): %d\n", bios_pb->sect_per_fat_32);
    fprintf(stdout, "Root Directory Starting Cluster: %d\n", bios_pb->root_cluster);
    fprintf(stdout, "File System Info Sector: %d\n", bios_pb->fs_info);
    fprintf(stdout, "Backup Boot Sector: %d\n", bios_pb->backup_boot_sector);
    fprintf(stdout, "Media Descriptor: %hhX\n", bios_pb->media_desc);

    fprintf(stdout, "Sector per Track: %d\n", bios_pb->sect_per_track);
    fprintf(stdout, "Number of Heads: %d\n", bios_pb->number_of_heads);

    fprintf(stdout, "FAT Address: 0x%x\n", bpb_faddress(bios_pb));
    fprintf(stdout, "Root Address: 0x%x\n", bpb_froot_addr(bios_pb));
    fprintf(stdout, "Data Address: 0x%x\n", bpb_fdata_addr(bios_pb));

    return;
}
