#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define UNUSED __attribute__((unused))

typedef enum {
    Text,
    Data,
    Bss,
    Rodata,
    None,
} SectionType;

const char *const sectionNames[] = {
    ".text",
    ".data",
    ".bss",
    ".rodata",
};

const int numSections = sizeof(sectionNames) / sizeof(sectionNames[0]);

size_t sectionNameLengths[sizeof(sectionNames) / sizeof(sectionNames[0])];
size_t maxSectionNameLength;

_Static_assert(None == sizeof(sectionNames) / sizeof(sectionNames[0]),
    "SectionType enum does not have the same number of values as the length of the Section Name array!");

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
    LocalSymbolRec = 0x30,
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

SectionType getSectionFromName(const char *const sectionName)
{
    int sectionIndex;
    for (sectionIndex = 0; sectionIndex < numSections; sectionIndex++)
    {
        if (strncmp(sectionNames[sectionIndex], sectionName, sectionNameLengths[sectionIndex]) == 0)
        {
            return sectionIndex;
        }
    }
    return None;
}

void processSectionName(FILE *f, SectionType *sectionOut, uint16_t *recordIdOut)
{
    uint8_t nameLen;
    char name[256];
    fread(recordIdOut, sizeof(*recordIdOut), 1, f);
    fseek(f, 3, SEEK_CUR); // Skip 3 unknown bytes
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(name, 1, nameLen, f);
    name[nameLen] = 0;

    *sectionOut = getSectionFromName(name);
    
    printf("Section Name Record:\n  id: %d\n  name: %.*s\n", *recordIdOut, nameLen, name);
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

void processBinBlockType(FILE *f, uint16_t *blockSectionIdOut)
{
    fread(blockSectionIdOut, sizeof(*blockSectionIdOut), 1, f);
    
    printf("Binary Block Type Record:\n  type: %d\n", *blockSectionIdOut);
}

uint32_t processBinBlock(FILE *f, FILE *outFile)
{
    uint16_t blockLen;
    fread(&blockLen, sizeof(blockLen), 1, f);

    if (outFile != NULL)
    {
        uint8_t *dataBuffer = malloc(blockLen);
        fread(dataBuffer, blockLen, 1, f);
        fwrite(dataBuffer, blockLen, 1, outFile);
        free(dataBuffer);
    }
    else
    {
        fseek(f, blockLen, SEEK_CUR); // skip the binary block
    }

    printf("Binary Block Record:\n  length: 0x%X\n", blockLen);

    return blockLen;
}

void processReloc(FILE *f)
{
    uint8_t relocType;
    uint16_t addr;
    uint8_t format;
    uint16_t referencedId;
    uint32_t offset;
    uint32_t fileAddr;

    fileAddr = ftell(f) - 1;

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

    if (format == 0x2C)
    {}
    printf("Reloc Record (at 0x%X):\n  type: 0x%X\n  addr: 0x%X\n  offset: 0x%X\n  symbol id: 0x%X\n  format: 0x%X\n", fileAddr, relocType, addr, offset, referencedId, format);
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

void processSymbol(FILE *f, char nameOut[256], uint16_t *sectionOut, uint32_t *offsetOut)
{
    uint16_t recId;
    uint8_t nameLen;
    fread(&recId, sizeof(recId), 1, f);
    fread(sectionOut, sizeof(*sectionOut), 1, f);
    fread(offsetOut, sizeof(*offsetOut), 1, f);
    fread(&nameLen, sizeof(nameLen), 1, f);
    fread(nameOut, 1, nameLen, f);
    nameOut[nameLen] = 0;
    
    printf("Symbol Record:\n  id: %d\n  section: %d\n  offset: 0x%X\n  name: %.*s\n", recId, *sectionOut, *offsetOut, nameLen, nameOut);
}

uint32_t processBss(FILE *f)
{
    uint32_t length;
    fread(&length, sizeof(length), 1, f);

    printf("Bss Record:\n  length: %d\n", length);

    return length;
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

void setupSectionNameLengths()
{
    int sectionIndex;
    maxSectionNameLength = 0;
    for (sectionIndex = 0; sectionIndex < numSections; sectionIndex++)
    {
        size_t curSectionNameLength = strlen(sectionNames[sectionIndex]);
        sectionNameLengths[sectionIndex] = curSectionNameLength;
        if (curSectionNameLength > maxSectionNameLength)
        {
            maxSectionNameLength = curSectionNameLength;
        }
    }
}

SectionType getSectionTypeFromRecordId(uint16_t curBlockSectionId, uint16_t sectionRecordIds[numSections])
{
    SectionType curSectionIndex;
    for (curSectionIndex = 0; curSectionIndex < numSections; curSectionIndex++)
    {
        if (sectionRecordIds[curSectionIndex] == curBlockSectionId)
        {
            return curSectionIndex;
        }
    }
    return None;
}

const char usage[] = "Usage: %s <Input> [Text Out] [Symbols Out]\n";

const char functionsFilename[] = "functions.s";
const char bssLengthFilename[] = "bss_length.s";

int main(UNUSED int argc, UNUSED char **argv)
{
    FILE *f;
    int recordType;
    long fileLength;
    char headerBuffer[4 + 1]; // LNK\x02

    FILE *sectionBinOutFiles[numSections];
    FILE *sectionSymbolsOutFiles[numSections];
    FILE *functionsOutFile;
    FILE *bssLengthOutFile;
    uint16_t sectionRecordIds[numSections];
    uint32_t bssLength = 0;

    SectionType curBlockSectionType = None;
    
    if (argc < 2)
    {
        printf(usage, argv[0]);
        return EXIT_FAILURE;
    }

    f = fopen(argv[1], "rb");

    fseek(f, 0, SEEK_SET);
    fgets(headerBuffer, sizeof(headerBuffer) / sizeof(char), f);

    if (strncmp("LNK\x02", headerBuffer, sizeof(headerBuffer) / sizeof(char)) != 0)
    {
        fprintf(stderr, "Not a valid LNK object file\n");
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    fileLength = ftell(f);
    fseek(f, 6, SEEK_SET);

    setupSectionNameLengths();

    int sectionIndex;
    char sectionBinFilenameBuffer[maxSectionNameLength + 32];
    for (sectionIndex = 0; sectionIndex < numSections; sectionIndex++)
    {
        if (sectionIndex != Bss)
        {
            sprintf(sectionBinFilenameBuffer, "out_%s.bin", sectionNames[sectionIndex] + 1);
            sectionBinOutFiles[sectionIndex] = fopen(sectionBinFilenameBuffer, "wb");
        }
        else
        {
            sectionBinOutFiles[sectionIndex] = NULL;
        }

        sprintf(sectionBinFilenameBuffer, "out_%s_syms.txt", sectionNames[sectionIndex] + 1);
        sectionSymbolsOutFiles[sectionIndex] = fopen(sectionBinFilenameBuffer, "w");
        fprintf(sectionSymbolsOutFiles[sectionIndex], "SECTIONS { %s : {\n", sectionNames[sectionIndex]);

        sectionRecordIds[sectionIndex] = 0;
    }

    functionsOutFile = fopen(functionsFilename, "w");
    bssLengthOutFile = fopen(bssLengthFilename, "w");

    recordType = fgetc(f);

    while (recordType != EndRec)
    {
        int skipped = 0;
        switch (recordType)
        {
            case SectionNameRec:
                {
                    SectionType curSectionType = None;
                    uint16_t curSectionRecordId;
                    processSectionName(f, &curSectionType, &curSectionRecordId);
                    if (curSectionType != None)
                    {
                        sectionRecordIds[curSectionType] = curSectionRecordId;
                    }
                }
                break;
            case FileNameRec:
                processFileName(f);
                break;
            case BinBlockTypeRec:
                {
                    uint16_t curBlockSectionId;
                    processBinBlockType(f, &curBlockSectionId);
                    curBlockSectionType = getSectionTypeFromRecordId(curBlockSectionId, sectionRecordIds);
                }
                break;
            case BinBlockRec:
                if (curBlockSectionType != None)
                {
                    processBinBlock(f, sectionBinOutFiles[curBlockSectionType]);
                }
                else
                {
                    processBinBlock(f, NULL);
                }
                break;
            case RelocRec:
                processReloc(f);
                break;
            case ExternRec:
                processExtern(f);
                break;
            case SymbolRec:
            case LocalSymbolRec:
                {
                    char curSymbolName[256];
                    uint16_t curSymbolSectionId;
                    uint32_t curSymbolOffset;
                    SectionType curSymbolSectionType;
                    processSymbol(f, curSymbolName, &curSymbolSectionId, &curSymbolOffset);
                    curSymbolSectionType = getSectionTypeFromRecordId(curSymbolSectionId, sectionRecordIds);
                    
                    if (curSymbolSectionType != None)
                    {
                        fprintf(sectionSymbolsOutFiles[curSymbolSectionType], "%s = 0x%08x;\n", curSymbolName, curSymbolOffset);
                    }
                    if (curSymbolSectionType == Text)
                    {
                        fprintf(functionsOutFile, ".global %s\n", curSymbolName);
                        fprintf(functionsOutFile, ".type %s, @function\n", curSymbolName);
                    }
                }
                break;
            case BssRec:
                bssLength += processBss(f);
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
    
    for (sectionIndex = 0; sectionIndex < numSections; sectionIndex++)
    {
        if (sectionBinOutFiles[sectionIndex] != NULL)
        {
            fclose(sectionBinOutFiles[sectionIndex]);
        }
        fputs("}}\n", sectionSymbolsOutFiles[sectionIndex]);
        fclose(sectionSymbolsOutFiles[sectionIndex]);
    }

    fclose(functionsOutFile);

    fprintf(bssLengthOutFile, ".skip %d\n", bssLength);
    fclose(bssLengthOutFile);
}