#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Definição de estrutura para o Boot Sector
#pragma pack(push, 1)
typedef struct {
    uint8_t jumpCode[3];
    char oemName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t numFATs;
    uint16_t rootEntries;
    uint16_t totalSectorsSmall;
    uint8_t mediaDescriptor;
    uint16_t sectorsPerFATSmall;
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    uint32_t hiddenSectors;
    uint32_t totalSectorsLarge;
    uint32_t sectorsPerFAT;
    uint16_t flags;
    uint16_t version;
    uint32_t rootCluster;
    uint16_t fsInfo;
    uint16_t bootSectorBackup;
    uint8_t reserved[12];
    uint8_t driveNumber;
    uint8_t reserved1;
    uint8_t bootSignature;
    uint32_t volumeID;
    char volumeLabel[11];
    char fsType[8];
} BootSector;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t ntReserved;
    uint8_t timeTenth;
    uint16_t createTime;
    uint16_t createDate;
    uint16_t lastAccessDate;
    uint16_t firstClusterHigh;
    uint16_t writeTime;
    uint16_t writeDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} DirectoryEntry;
#pragma pack(pop)

// Função para listar arquivos no diretório
void listFiles(FILE *imgFile, BootSector *bootSector, uint32_t cluster) {
    uint32_t rootDirSector = 
        bootSector->reservedSectors + 
        bootSector->numFATs * bootSector->sectorsPerFAT;

    uint32_t clusterSector = rootDirSector + (cluster - 2) * bootSector->sectorsPerCluster;
    uint8_t *buffer = malloc(bootSector->bytesPerSector * bootSector->sectorsPerCluster);

    fseek(imgFile, clusterSector * bootSector->bytesPerSector, SEEK_SET);
    fread(buffer, bootSector->bytesPerSector, bootSector->sectorsPerCluster, imgFile);

    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    for (int i = 0; i < (bootSector->bytesPerSector * bootSector->sectorsPerCluster) / sizeof(DirectoryEntry); i++) {
        if (entry[i].name[0] == 0x00) break; // Sem mais entradas
        if (entry[i].name[0] == 0xE5) continue; // Entrada deletada
        if (entry[i].attr & 0x08) continue; // Volume label

        char filename[12] = {0};
        strncpy(filename, entry[i].name, 11);
        filename[11] = '\0';

        // Limpar espaços
        for (int j = 10; j >= 0 && filename[j] == ' '; j--)
            filename[j] = '\0';

        printf("%s%s\n", filename, (entry[i].attr & 0x10) ? "/" : ""); // '/' para diretórios
    }

    free(buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo.img>\n", argv[0]);
        return 1;
    }

    FILE *imgFile = fopen(argv[1], "rb");
    if (!imgFile) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    BootSector bootSector;
    fread(&bootSector, sizeof(BootSector), 1, imgFile);

    printf("Sistema de Arquivos: %.8s\n", bootSector.fsType);
    if (strncmp(bootSector.fsType, "FAT32   ", 8) != 0) {
        printf("O arquivo não está formatado em FAT32.\n");
        fclose(imgFile);
        return 1;
    }

    printf("Listando arquivos do diretório raiz:\n");
    listFiles(imgFile, &bootSector, bootSector.rootCluster);

    fclose(imgFile);
    return 0;
}
