#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define UNUSED __attribute__((unused))

typedef enum {
    EndRec,
    Unk1,
    BinBlockRec,
    Unk3,
    Unk4,
    Unk5,
    BinBlockTypeRec,
    Unk7,
    BssRec,
    Unk9,
    RelocRec,
    UnkB,
    SymbolRec,
    UnkD,
    ExternRec,
    UnkF,
    SectionNameRec,
    Unk11,
    BssSymbolRec,
    FileNameRec = 0x1C,
    Unk30 = 0x30,
    Unk32 = 0x32,
    Unk34 = 0x34,
    Unk38 = 0x38,
    Unk3A = 0x3A,
    Unk3C = 0x3C,
    Unk4A = 0x4A,
    Unk4C = 0x4C,
    Unk4E = 0x4E,
    Unk50 = 0x50,
    Unk52 = 0x52,
    Unk54 = 0x54,
    Unk6F = 0x6F,
    UnkD7 = 0xD7,
} RecordType;

void processSectionName(FILE *f)
{
    uint16_t recId;
    uint8_t nameLen;
    char name[256];
    fread(&recId, sizeof(recId), 1, f);
    fseek(f, 3, SEEK_CUR); // Skip 3 unknown bytes
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(name, 1, nameLen, f);
    name[nameLen] = 0;
    
    printf("Section Name Record:\n  id: %d\n  name: %.*s\n", recId, nameLen, name);
}

void processFileName(FILE *f)
{
    uint16_t recId;
    uint8_t nameLen;
    char name[256];
    fread(&recId, sizeof(recId), 1, f);
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(name, 1, nameLen, f);
    name[nameLen] = 0;
    
    printf("File Name Record:\n  id: %d\n  name: %.*s\n", recId, nameLen, name);
}

void processBinBlockType(FILE *f)
{
    uint16_t blockType;
    fread(&blockType, sizeof(blockType), 1, f);
    
    printf("Binary Block Type Record:\n  type: %d\n", blockType);
}

void processBinBlock(FILE *f)
{
    uint16_t blockLen;
    fread(&blockLen, sizeof(blockLen), 1, f);

    fseek(f, blockLen, SEEK_CUR); // skip the binary block

    printf("Binary Block Record:\n  length: 0x%X\n", blockLen);
}

void processReloc(FILE *f)
{
    uint8_t relocType;
    uint16_t addr;
    uint8_t format;
    uint16_t referencedId;
    uint32_t offset;

    fread(&relocType, sizeof(relocType), 1, f);
    fread(&addr, sizeof(addr), 1, f);
    fread(&format, sizeof(format), 1, f);

    if (format == 0x02)
    {
        offset = 0;
    }
    else if (format == 0x2C)
    {
        fseek(f, 1, SEEK_CUR); // skip 1 byte
        fread(&offset, sizeof(offset), 1, f);
        fseek(f, 1, SEEK_CUR); // skip 1 byte
    }
    else if (format == 0x00)
    {
        fseek(f, 2, SEEK_CUR); // skip 2 bytes
        // fread(&offset, sizeof(offset), 1, f);
        // fseek(f, 1, SEEK_CUR); // skip 1 byte
    }

    fread(&referencedId, sizeof(referencedId), 1, f);
    printf("Reloc Record:\n  type: 0x%X\n  addr: 0x%X\n  offset: 0x%X\n  symbol id: %d\n  format: 0x%X\n", relocType, addr, offset, referencedId, format);
}

void processExtern(FILE *f)
{
    uint16_t recId;
    uint8_t nameLen;
    char name[256];
    fread(&recId, sizeof(recId), 1, f);
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(name, 1, nameLen, f);
    name[nameLen] = 0;
    
    printf("External Symbol Record:\n  id: %d\n  name: %.*s\n", recId, nameLen, name);
}

void processSymbol(FILE *f)
{
    uint16_t recId;
    uint16_t section;
    uint32_t offset;
    uint8_t nameLen;
    char name[256];
    fread(&recId, sizeof(recId), 1, f);
    fread(&section, sizeof(section), 1, f);
    fread(&offset, sizeof(offset), 1, f);
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(name, 1, nameLen, f);
    name[nameLen] = 0;
    
    printf("Symbol Record:\n  id: %d\n  section: %d\n  offset: 0x%X\n  name: %.*s\n", recId, section, offset, nameLen, name);
}

void processBss(FILE *f)
{
    uint32_t length;
    fread(&length, sizeof(length), 1, f);

    printf("Bss Record:\n  length: %d\n", length);
}

void processBssSymbol(FILE *f)
{
    uint16_t recId;
    uint32_t offset;
    uint8_t nameLen;
    char name[256];
    fread(&recId, sizeof(recId), 1, f);
    fread(&offset, sizeof(offset), 1, f);
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(name, 1, nameLen, f);
    name[nameLen] = 0;
    
    printf("Bss Symbol Record:\n  id: %d\n  offset: 0x%X\n  name: %.*s\n", recId, offset, nameLen, name);
}

int main(UNUSED int argc, UNUSED char **argv)
{
    FILE *f = fopen("test/rrow.obj", "rb");
    int recordType;
    UNUSED long fileLength;
    char *strBuf = calloc(65536, sizeof(char));

    fseek(f, 0, SEEK_SET);
    fgets(strBuf, 4 + 1, f);

    if (strncmp("LNK\x02", strBuf, 4) != 0)
    {
        fprintf(stderr, "Not a valid LNK object file\n");
        free(strBuf);
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    fileLength = ftell(f);

    fseek(f, 6, SEEK_SET);

    recordType = fgetc(f);

    while (recordType != EndRec)
    {
        int skipped = 0;
        switch (recordType)
        {
            case SectionNameRec:
                processSectionName(f);
                break;
            case FileNameRec:
                processFileName(f);
                break;
            case BinBlockTypeRec:
                processBinBlockType(f);
                break;
            case BinBlockRec:
                processBinBlock(f);
                break;
            case RelocRec:
                processReloc(f);
                break;
            case ExternRec:
                processExtern(f);
                break;
            case SymbolRec:
                processSymbol(f);
                break;
            case BssRec:
                processBss(f);
                break;
            case BssSymbolRec:
                processBssSymbol(f);
                break;
            case Unk32:
                skipped = 2;
                break;
            case Unk34:
                skipped = 3;
                // fread(&skipped, 2, 1, f);
                break;
            case Unk38:
                skipped = 6;
                // fread(&skipped, 2, 1, f);
                break;
            case Unk3A:
                skipped = 8;
                // fseek(f, 2, SEEK_CUR);
                // printf("0x%lX\n", ftell(f));
                // fread(&skipped, 4, 1, f);
                // printf("%d\n", skipped);
                break;
            case Unk3C:
                skipped = 2;
                break;
            case Unk4A:
                fseek(f, 28, SEEK_CUR); // ?
                fread(&skipped, 1, 1, f); // Get symbol name length and skip symbol name
                break;
            case Unk4C:
                skipped = 10;
                break;
            case Unk4E:
                skipped = 10;
                break;
            case Unk50:
                skipped = 10;
                break;
            case Unk52: // Symbol type info
                fseek(f, 14, SEEK_CUR); // Skip type info
                fread(&skipped, 1, 1, f); // Get symbol name length and skip symbol name
                break;
            case Unk54: // Bss Symbol type info?
                fseek(f, 21, SEEK_CUR); // Skip type info
                fread(&skipped, 1, 1, f); // Get symbol name length and skip symbol name
                break;
            case Unk6F:
                fread(&skipped, 2, 1, f);
                break;
            case UnkD7:
                skipped = 13;
                break;
            default:
                fprintf(stderr, "Unknown record type: 0x%X (at pos 0x%lX)\n", recordType, ftell(f) - 1);
                free(strBuf);
                exit(EXIT_FAILURE);
                break;
        }
        if (skipped)
        {
            fseek(f, skipped, SEEK_CUR);
            printf("Skipped record (0x%X)\n", recordType);
        }

        recordType = fgetc(f);
    }

    printf("Hit zero\n");

    if (ftell(f) == fileLength)
    {
        printf("Valid file!\n");
    }
    else
    {
        printf("Early termination at 0x%lX\n", ftell(f));
    }

    free(strBuf);
}