// CSS - Chris Sawyer Sound file create / extract utility

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")

#pragma pack(push, 1)
typedef struct
{
    uint16_t encoding;
    uint16_t channels;
    uint32_t frequency;
    uint32_t byterate;
    uint16_t blockalign;
    uint16_t bitspersample;
} WaveFormat;
assert_struct_size(WaveFormat, 16);

typedef struct
{
    uint16_t encoding;
    uint16_t channels;
    uint32_t frequency;
    uint32_t byterate;
    uint16_t blockalign;
    uint16_t bitspersample;
    uint16_t extrasize;
} WaveFormatEx;
assert_struct_size(WaveFormatEx, 18);

typedef struct
{
    uint32_t RiffMagic;
    uint32_t RiffLength;
    uint32_t WaveMagic;
    uint32_t FmtMagic;
    uint32_t FmtLength;
    WaveFormat Fmt;
    uint32_t DataMagic;
    uint32_t PcmLength;
} WaveFileHeader;
assert_struct_size(WaveFileHeader, 44);
#pragma pack(pop)

enum RESULT
{
    RESULT_OK = 0,
    RESULT_ERROR_ANY,
    RESULT_ERROR_OPEN,
    RESULT_ERROR_NO_INPUT,
    RESULT_ERROR_NO_ARGS,
    RESULT_ERROR_INVALID_ARG,
    RESULT_ERROR_UNKNOWN_ARG,
    RESULT_ERROR_MEMORY_ALLOCATION,
};

void checkalloc(void* mem)
{
    if (mem == NULL)
    {
        fprintf(stderr, "Error allocating memory.\n");
        exit(RESULT_ERROR_MEMORY_ALLOCATION);
    }
}

int exportwave(const char* path, const WaveFormatEx* format, const void* pcm, uint32_t pcmLength)
{
    printf("Extracting %s...\n", path);
    FILE* fp = fopen(path, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error writing file: %s.\n", path);
        return RESULT_ERROR_OPEN;
    }

    uint32_t riffLength = 36 + pcmLength;
    uint32_t fmtLength = 16;

    fwrite("RIFF", 1, 4, fp);
    fwrite(&riffLength, sizeof(riffLength), 1, fp);
    fwrite("WAVE", 1, 4, fp);
    fwrite("fmt ", 1, 4, fp);
    fwrite(&fmtLength, sizeof(fmtLength), 1, fp);
    fwrite(format, sizeof(WaveFormat), 1, fp);
    fwrite("data", 1, 4, fp);
    fwrite(&pcmLength, sizeof(pcmLength), 1, fp);
    fwrite(pcm, 1, pcmLength, fp);
    fclose(fp);
    return RESULT_OK;
}

int extract(const char* path, const char* exportPath)
{
    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Unable to open %s\n", path);
        return RESULT_ERROR_OPEN;
    }

    uint32_t numSounds = 0;
    fread(&numSounds, sizeof(numSounds), 1, fp);

    uint32_t* offsets = (uint32_t*)calloc(numSounds, sizeof(uint32_t));
    checkalloc(offsets);
    fread(offsets, sizeof(uint32_t), numSounds, fp);

    int result = RESULT_OK;
    void* pcmBuffer = NULL;
    size_t pcmBufferCapacity = 0;
    for (uint32_t i = 0; i < numSounds; i++)
    {
        fseek(fp, offsets[i], SEEK_SET);
        uint32_t length = 0;
        fread(&length, sizeof(length), 1, fp);

        WaveFormatEx waveFormat;
        memset(&waveFormat, 0, sizeof(WaveFormatEx));
        fread(&waveFormat, sizeof(waveFormat), 1, fp);

        if (pcmBufferCapacity < length)
        {
            void* newPcmBuffer = realloc(pcmBuffer, length);
            checkalloc(newPcmBuffer);
            pcmBuffer = newPcmBuffer;
            pcmBufferCapacity = length;
        }
        fread(pcmBuffer, 1, length, fp);

        char outPath[2048];
        memset(outPath, 0, sizeof(outPath));
        snprintf(outPath, sizeof(outPath), "%scss1_%02d.wav", exportPath, i);
        int exportResult = exportwave(outPath, &waveFormat, pcmBuffer, (uint32_t)length);
        if (exportResult != RESULT_OK)
        {
            result = RESULT_ERROR_ANY;
        }
    }
    free(pcmBuffer);
    free(offsets);
    fclose(fp);
    return result;
}

int importwave(FILE* fp, const char* inputPath, int* outLength)
{
    FILE* inputFile = fopen(inputPath, "rb");
    if (inputFile == NULL)
    {
        fprintf(stderr, "Failed to open %s\n", inputPath);
        return RESULT_ERROR_OPEN;
    }

    WaveFileHeader waveFileHeader;
    if (fread(&waveFileHeader, sizeof(waveFileHeader), 1, inputFile) != 1)
    {
        fclose(inputFile);
        fprintf(stderr, "Failed to read wave header\n");
        return RESULT_ERROR_OPEN;
    }

    // TODO validate wave header

    fwrite(&waveFileHeader.PcmLength, sizeof(waveFileHeader.PcmLength), 1, fp);
    fwrite(&waveFileHeader.Fmt, sizeof(WaveFileHeader), 1, fp);
    uint16_t extrasize = 0;
    fwrite(&extrasize, sizeof(extrasize), 1, fp);

    // Read in PCM
    char buffer[2048];
    size_t pcmLengthRemaining = waveFileHeader.PcmLength;
    while (pcmLengthRemaining > 0)
    {
        size_t bytesToRead = pcmLengthRemaining;
        if (bytesToRead > sizeof(buffer))
            bytesToRead = sizeof(buffer);

        size_t readBytes = fread(buffer, 1, bytesToRead, inputFile);
        if (readBytes != bytesToRead)
        {
            fclose(inputFile);
            fprintf(stderr, "Failed to read PCM data\n");
            return RESULT_ERROR_OPEN;
        }
        fwrite(buffer, 1, readBytes, fp);
        pcmLengthRemaining -= readBytes;
    }
    fclose(inputFile);

    if (outLength != NULL)
    {
        *outLength += sizeof(WaveFileHeader) + 4 + waveFileHeader.PcmLength;
    }

    return RESULT_OK;
}

int import(const char* datPath, const char** wavePaths, uint32_t numWavePaths)
{
    FILE* fp = fopen(datPath, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to write %s\n", datPath);
        return RESULT_ERROR_OPEN;
    }

    fwrite(&numWavePaths, sizeof(numWavePaths), 1, fp);

    long writeOffset = 4 + (numWavePaths * sizeof(uint32_t));
    for (size_t i = 0; i < numWavePaths; i++)
    {
        printf("Importing %s...\n", wavePaths[i]);
        fseek(fp, (long)(4 + i * sizeof(uint32_t)), SEEK_SET);
        fwrite(&writeOffset, sizeof(writeOffset), 1, fp);
        fseek(fp, writeOffset, SEEK_SET);

        int length = 0;
        int result = importwave(fp, wavePaths[i], &length);
        if (result != RESULT_OK)
        {
            fclose(fp);
            return result;
        }
        writeOffset += length;
    }
    fclose(fp);
    return RESULT_OK;
}

bool direxists(const char* directory)
{
#if _WIN32
    DWORD dwAttrib = GetFileAttributesA(directory);
    return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat sb;
    if (stat(directory, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        return true;
    }
    return false;
#endif
}

char* ensureendslash(const char* path)
{
    char* resultPath;
    size_t pathLen = strlen(path);
    if (pathLen != 0
        && path[pathLen - 1] != '/'
#if _WIN32
        && path[pathLen - 1] != '\\'
#endif
    )
    {
        resultPath = (char*)malloc(pathLen + 2);
        checkalloc(resultPath);
        memcpy(resultPath, path, pathLen);
        resultPath[pathLen] = '/';
        resultPath[pathLen + 1] = '\0';
    }
    else
    {
        resultPath = strdup(path);
        checkalloc(resultPath);
    }
    return resultPath;
}

typedef struct
{
    int argc;
    char** argv;
    int remainingIndex;
} args_t;

bool checkoptions(args_t* args, const char* valid)
{
    for (int i = 0; i < args->argc; i++)
    {
        if (args->argv[i][0] == '-' && args->argv[i][1] != '-')
        {
            const char* ch = &args->argv[i][1];
            while (*ch != '\0')
            {
                if (!strchr(valid, *ch))
                {
                    fprintf(stderr, "Unknown option: %c\n", *ch);
                    return false;
                }
                ch++;
            }
            if (args->remainingIndex < i + 1)
            {
                args->remainingIndex = i + 1;
            }
        }
    }
    return true;
}

bool hasoption(args_t* args, char c)
{
    for (int i = 0; i < args->argc; i++)
    {
        if (args->argv[i][0] == '-' && args->argv[i][1] != '-')
        {
            const char* ch = &args->argv[i][1];
            while (*ch != '\0')
            {
                if (*ch == c)
                {
                    return true;
                }
                ch++;
            }
        }
    }
    return false;
}

char* getoption(args_t* args, char c)
{
    for (int i = 0; i < args->argc; i++)
    {
        if (args->argv[i][0] == '-' && args->argv[i][1] != '-')
        {
            const char* ch = &args->argv[i][1];
            while (*ch != '\0')
            {
                if (*ch == c)
                {
                    i++;
                    if (args->argc > i)
                    {
                        if (args->remainingIndex < i + 1)
                        {
                            args->remainingIndex = i + 1;
                        }
                        return args->argv[i];
                    }
                }
                ch++;
            }
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        printf("CSS create / extract\n");
        printf("usage:   css [options] <dat-path> [<wav-path> ...]\n");
        printf("         css -x [options] <dat-path> [<dat-path> ...]\n");
        printf("\n");
        printf("options: -o <path>        Set extract destination directory.\n");
        printf("         -x               Extract from DAT file.\n");
        return RESULT_ERROR_NO_ARGS;
    }

    args_t args;
    args.argc = argc;
    args.argv = argv;
    args.remainingIndex = 1;

    if (!checkoptions(&args, "ox"))
    {
        return RESULT_ERROR_UNKNOWN_ARG;
    }

    if (hasoption(&args, 'x'))
    {
        char* exportDir;
        if (hasoption(&args, 'o'))
        {
            exportDir = getoption(&args, 'o');
            if (exportDir == NULL)
            {
                fprintf(stderr, "No output directory specified\n");
                return RESULT_ERROR_INVALID_ARG;
            }
            else
            {
                if (!direxists(exportDir))
                {
                    fprintf(stderr, "Invalid directory: %s\n", exportDir);
                    return RESULT_ERROR_INVALID_ARG;
                }
                exportDir = ensureendslash(exportDir);
            }
        }
        else
        {
            exportDir = strdup("");
        }

        int totalResult = RESULT_OK;
        int numFiles = 0;
        for (int i = args.remainingIndex; i < argc; i++)
        {
            int result = extract(argv[i], exportDir);
            if (result != RESULT_OK)
            {
                totalResult = RESULT_ERROR_ANY;
            }
            numFiles++;
        }

        if (numFiles == 0)
        {
            fprintf(stderr, "No input files specified.\n");
            return RESULT_ERROR_NO_INPUT;
        }

        free(exportDir);
        return totalResult;
    }
    else
    {
        if (args.remainingIndex >= argc)
        {
            fprintf(stderr, "No destination file specified.\n");
            return RESULT_ERROR_NO_INPUT;
        }

        int numInputFiles = argc - args.remainingIndex - 1;
        if (numInputFiles <= 0)
        {
            fprintf(stderr, "No input files specified.\n");
            return RESULT_ERROR_NO_INPUT;
        }

        return import(argv[args.remainingIndex], (const char**)&argv[args.remainingIndex + 1], (uint32_t)numInputFiles);
    }
}
