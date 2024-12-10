#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define IMG_SIZE (512 * 1024 * 1024)  // Tamanho da imagem: 512 MB
#define SECTOR_SIZE 512              // Tamanho do setor: 512 bytes
#define CLUSTER_SIZE (8 * SECTOR_SIZE) // Cluster: 8 setores por cluster
#define RESERVED_SECTORS 32          // Setores reservados (inclui Boot Sector)
#define FAT_COUNT 2                  // Número de FATs
#define ROOT_CLUSTER 2               // Diretório raiz começa no cluster 2
#define SECTORS_PER_TRACK 32         // Padrão FAT32
#define NUM_HEADS 64                 // Padrão FAT32

// Função para calcular setores por FAT
uint32_t calculate_sect_per_fat(uint32_t total_clusters) {
    return (total_clusters * 4 + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

// Escrever valor no boot_sector de forma segura (little-endian)
void write_uint32_le(uint8_t *dest, uint32_t value) {
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
    dest[2] = (value >> 16) & 0xFF;
    dest[3] = (value >> 24) & 0xFF;
}

// Função para criar e formatar a imagem FAT32
void create_fat32_image(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Erro ao criar a imagem");
        exit(EXIT_FAILURE);
    }

    printf("Criando imagem '%s'...\n", filename);

    // Preencher a imagem com zeros
    uint8_t *empty_sector = calloc(SECTOR_SIZE, sizeof(uint8_t));
    for (size_t i = 0; i < IMG_SIZE / SECTOR_SIZE; i++) {
        fwrite(empty_sector, SECTOR_SIZE, 1, fp);
    }
    free(empty_sector);

    // Calcular total de setores e clusters
    uint32_t total_sectors = IMG_SIZE / SECTOR_SIZE;
    uint32_t data_sectors = total_sectors - RESERVED_SECTORS - FAT_COUNT * calculate_sect_per_fat(total_sectors / CLUSTER_SIZE);
    uint32_t total_clusters = data_sectors / (CLUSTER_SIZE / SECTOR_SIZE);

    printf("Total de setores: %u\n", total_sectors);
    printf("Total de clusters: %u\n", total_clusters);

    // Criar Boot Sector
    uint8_t boot_sector[SECTOR_SIZE] = {0};
    boot_sector[0] = 0xEB; // Jump instruction
    boot_sector[1] = 0x58; // Jump instruction
    boot_sector[2] = 0x90; // NOP
    memcpy(&boot_sector[3], "MSWIN4.1", 8); // OEM Name
    *((uint16_t *)&boot_sector[11]) = SECTOR_SIZE; // Bytes por setor
    boot_sector[13] = 8; // Setores por cluster
    *((uint16_t *)&boot_sector[14]) = RESERVED_SECTORS; // Setores reservados
    boot_sector[16] = FAT_COUNT; // Número de FATs
    *((uint16_t *)&boot_sector[17]) = 0; // Root entries (0 para FAT32)
    *((uint16_t *)&boot_sector[22]) = SECTORS_PER_TRACK; // Sectores por trilha
    *((uint16_t *)&boot_sector[24]) = NUM_HEADS; // Número de cabeças
    *((uint32_t *)&boot_sector[32]) = total_sectors; // Total de setores (32 bits)
    *((uint32_t *)&boot_sector[36]) = calculate_sect_per_fat(total_clusters); // Setores por FAT (32 bits)

    // Gravação do root_cluster manualmente
    write_uint32_le(&boot_sector[44], ROOT_CLUSTER);

    *((uint16_t *)&boot_sector[48]) = 1; // File system info sector
    *((uint16_t *)&boot_sector[50]) = 6; // Backup boot sector
    boot_sector[21] = 0xF8; // Media Descriptor
    boot_sector[510] = 0x55; // Assinatura do boot sector
    boot_sector[511] = 0xAA;

    // Gravar Boot Sector no arquivo
    fseek(fp, 0, SEEK_SET);
    fwrite(boot_sector, SECTOR_SIZE, 1, fp);

    // Verificar se o boot sector foi gravado corretamente
    uint8_t verify_sector[SECTOR_SIZE] = {0};
    fseek(fp, 0, SEEK_SET);
    fread(verify_sector, SECTOR_SIZE, 1, fp);

    printf("Verificando bytes do root_cluster no setor de boot após fwrite:\n");
    printf("Byte 44: %u\n", verify_sector[44]);
    printf("Byte 45: %u\n", verify_sector[45]);
    printf("Byte 46: %u\n", verify_sector[46]);
    printf("Byte 47: %u\n", verify_sector[47]);
    printf("root_cluster no arquivo: %u\n", *((uint32_t *)&verify_sector[44]));

    fclose(fp);
    printf("Imagem FAT32 '%s' criada com sucesso!\n", filename);
}

// Função principal
int main() {
    const char *filename = "diskTest32.img";
    create_fat32_image(filename);
    return 0;
}
