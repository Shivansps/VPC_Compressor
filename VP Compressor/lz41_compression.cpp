#include <iostream>
#include <string>
#include <filesystem>
#include "lz41_compression.h"
#include "lib/vp/vp.h"
#include "lib/pof/aligner.h"
#include "lib/lz4/lz4.h"
#include "lib/lz4/lz4hc.h"
namespace fs = std::filesystem;
#pragma warning(disable:4996)

/*LZ4 implementation*/
//Needs Block size
int lz41_stream_compress(FILE* file_in, FILE* file_out, const LZ41CONFIG *config);
//Needs Block size and original size
int lz41_compress_memory(char* bytes_in, char* bytes_out, int original_size, const LZ41CONFIG *config);
//Needs compressed size
int lz41_decompress_memory(char* bytes_in, char* bytes_out, int compressed_size);
int lz41_stream_decompress(FILE* file_in, FILE* file_out);
/*LZ4-HC implementation, better ratio but slower, Compression level: 5 to 12 (MAX) */
//Needs block size and compression level
int lz41_stream_compress_HC(FILE* file_in, FILE* file_out, const LZ41CONFIG *config);
//Needs block size, original size and compression level
int lz41_compress_memory_HC(char* bytes_in, char* bytes_out, int original_size, const LZ41CONFIG *config);

int lz41_compress_memory(char* bytes_in, char* bytes_out,int original_size, const LZ41CONFIG *config)
{
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;

    char* inpBuf = (char*)malloc(config->block_size);
    int max_blocks = LZ4_compressBound(original_size) / config->block_size;
    max_blocks += config->block_size;
    int* offsets = (int*)malloc(max_blocks * sizeof(int));
    int* offsetsEnd = offsets;
    int in_offset = 0;
    int written_bytes = 0;

    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    memcpy(bytes_out, LZ41_FILE_HEADER, sizeof(LZ41_FILE_HEADER));
    bytes_out += sizeof(LZ41_FILE_HEADER);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    char* cmpBuf = (char*)malloc(LZ4_compressBound(config->block_size));
    while(in_offset < original_size)
    {
        int amountToRead = config->block_size;

        if (original_size < in_offset+amountToRead)
            amountToRead -= (in_offset + amountToRead) - original_size;

        memcpy(inpBuf, bytes_in, amountToRead);
        bytes_in += amountToRead;
        in_offset += amountToRead;

        LZ4_resetStream(lz4Stream);

        int cmpBytes = LZ4_compress_fast_continue(lz4Stream, inpBuf, cmpBuf, amountToRead, LZ4_compressBound(config->block_size), 5-config->compression_level);
        if (cmpBytes <= 0)
            return LZ41_COMPRESSION_ERROR;

        memcpy(bytes_out, cmpBuf, cmpBytes);
        bytes_out += cmpBytes;
        written_bytes += cmpBytes;
        
        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }
    free(cmpBuf);
    free(inpBuf);
    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd)
    {
        memcpy(bytes_out, ptr++, sizeof(int));
        bytes_out += sizeof(int);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    memcpy(bytes_out, &i, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Filesize */
    memcpy(bytes_out, &original_size, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Block Size */
    memcpy(bytes_out, &config->block_size, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_compress_memory_HC(char* bytes_in, char* bytes_out, int original_size, const LZ41CONFIG *config)
{
    LZ4_streamHC_t lz4Stream_body;
    LZ4_streamHC_t* lz4Stream = &lz4Stream_body;

    char *inpBuf=(char*)malloc(config->block_size);
    int max_blocks = LZ4_compressBound(original_size) / config->block_size;
    max_blocks += config->block_size;
    int* offsets = (int*)malloc(max_blocks*sizeof(int));
    int* offsetsEnd = offsets;
    int in_offset = 0;
    int written_bytes = 0;

    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    memcpy(bytes_out, LZ41_FILE_HEADER, sizeof(LZ41_FILE_HEADER));
    bytes_out += sizeof(LZ41_FILE_HEADER);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    char* cmpBuf = (char*)malloc(LZ4_compressBound(config->block_size));
    while (in_offset < original_size)
    {
        int amountToRead = config->block_size;

        if (original_size < in_offset + amountToRead)
            amountToRead -= (in_offset + amountToRead) - original_size;

        memcpy(inpBuf, bytes_in, amountToRead);
        bytes_in += amountToRead;
        in_offset += amountToRead;

        LZ4_resetStreamHC(lz4Stream,config->compression_level);

        int cmpBytes = LZ4_compress_HC_continue(lz4Stream, inpBuf, cmpBuf, amountToRead, LZ4_compressBound(config->block_size));
        if (cmpBytes <= 0)
            return LZ41_COMPRESSION_ERROR;

        memcpy(bytes_out, cmpBuf, (size_t)cmpBytes);
        bytes_out += (size_t)cmpBytes;
        written_bytes += (size_t)cmpBytes;

        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
        
    }
    free(cmpBuf);
    free(inpBuf);

    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd)
    {
        memcpy(bytes_out, ptr++, sizeof(int));
        bytes_out += sizeof(int);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    memcpy(bytes_out, &i, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Filesize */
    memcpy(bytes_out, &original_size, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Block Size */
    memcpy(bytes_out, &config->block_size, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_decompress_memory(char* bytes_in, char* bytes_out, int compressed_size)
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
    int offset = 0, length = 0, numOffsets, written_bytes = 0,block_size, block;

    memcpy(&numOffsets, bytes_in + compressed_size - 12, sizeof(int));
    memcpy(&length, bytes_in + compressed_size - 8, sizeof(int));
    memcpy(&block_size, bytes_in + compressed_size - 4, sizeof(int));

    char *decBuf=(char*)malloc(block_size);
    int* offsets = (int*)malloc(numOffsets*sizeof(int));

    /*Read Header*/
    char header[sizeof(LZ41_FILE_HEADER)];
    memcpy(header, bytes_in, sizeof(LZ41_FILE_HEADER));
    if (memcmp(LZ41_FILE_HEADER, header, sizeof(LZ41_FILE_HEADER)))
        return LZ41_HEADER_MISMATCH;

    /* The blocks [currentBlock, endBlock) contain the data we want */
    int currentBlock = offset / block_size;
    int endBlock = ((offset + length - 1) / block_size) + 1;

    /* Read the offsets tail */
    int* offsetsPtr = offsets;
    if (numOffsets <= endBlock)
        return LZ41_OFFSETS_MISMATCH;
    
    char* firstOffset = bytes_in + compressed_size + (-4 * (numOffsets + 1)) - (sizeof(int)*2);
    for (block = 0; block <= endBlock; ++block)
    {
        memcpy(offsetsPtr++, firstOffset, sizeof(int));
        firstOffset += sizeof(int);
    }

    /* Seek to the first block to read */
    bytes_in +=offsets[currentBlock];
    offset = offset % block_size;

    /* Start decoding */
    char* cmpBuf = (char*)malloc(LZ4_compressBound(block_size));
    for (; currentBlock < endBlock; ++currentBlock)
    {
        /* The difference in offsets is the size of the block */
        int  cmpBytes = offsets[currentBlock + 1] - offsets[currentBlock];
        memcpy(cmpBuf, bytes_in, cmpBytes);
        bytes_in += cmpBytes;

        const int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decBuf, cmpBytes, block_size);
        if (decBytes <= 0)
            return LZ41_DECOMPRESSION_ERROR;

        /* Write out the part of the data we care about */
        int blockLength = ((length) < ((decBytes - offset)) ? (length) : ((decBytes - offset)));
        memcpy(bytes_out + written_bytes, decBuf + offset, (size_t)blockLength);
        written_bytes += (size_t)blockLength;
        offset = 0;
        length -= blockLength;
    }
    free(cmpBuf);
    free(decBuf);
    free(offsets);
    return written_bytes;
}

int lz41_stream_compress( FILE* file_in, FILE* file_out, const LZ41CONFIG *config )
{
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    int written_bytes = 0;

    char *inpBuf=(char*)malloc(config->block_size);
    fseek(file_in, 0, SEEK_END);
    int file_size = ftell(file_in);
    int max_blocks = LZ4_compressBound(file_size) / config->block_size;
    max_blocks += config->block_size;
    int* offsets = (int*)malloc(max_blocks * sizeof(int));
    int* offsetsEnd = offsets;
    fseek(file_in, 0, SEEK_SET);

    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    fwrite(LZ41_FILE_HEADER, 1, sizeof(LZ41_FILE_HEADER), file_out);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    char* cmpBuf = (char*)malloc(LZ4_compressBound(config->block_size));
    for (;;) 
    {
        const int inpBytes = fread(inpBuf, 1, config->block_size,file_in);
        if (0 == inpBytes) {
            break;
        }

        LZ4_resetStream(lz4Stream);

        int cmpBytes = LZ4_compress_fast_continue(lz4Stream, inpBuf, cmpBuf, inpBytes, LZ4_compressBound(config->block_size), 5-config->compression_level);
        if (cmpBytes <= 0) 
           return LZ41_COMPRESSION_ERROR;

        fwrite(cmpBuf, (size_t)cmpBytes, 1, file_out);
        written_bytes += (size_t)cmpBytes;

        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }
    free(inpBuf);
    free(cmpBuf);
    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd) 
    {
        fwrite(ptr++, sizeof(int), 1, file_out);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    fwrite(&i, sizeof(int), 1, file_out);
    written_bytes += sizeof(int);

    /* Write Filesize */
    fwrite(&file_size, 4, 1, file_out);
    written_bytes += sizeof(int);

    /* Write Block Size */
    fwrite(&config->block_size, 4, 1, file_out);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_stream_compress_HC(FILE* file_in, FILE* file_out, const LZ41CONFIG *config)
{
    LZ4_streamHC_t lz4Stream_body;
    LZ4_streamHC_t* lz4Stream = &lz4Stream_body;
    int written_bytes = 0;

    char *inpBuf=(char*)malloc(config->block_size);
    fseek(file_in, 0, SEEK_END);
    int file_size = ftell(file_in);
    int max_blocks = LZ4_compressBound(file_size) / config->block_size;
    max_blocks += config->block_size;
    int* offsets = (int*)malloc(max_blocks * sizeof(int));
    int* offsetsEnd = offsets;
    fseek(file_in, 0, SEEK_SET);

    LZ4_initStreamHC(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    fwrite(LZ41_FILE_HEADER, 1, sizeof(LZ41_FILE_HEADER), file_out);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    char* cmpBuf = (char*)malloc(LZ4_compressBound(config->block_size));
    for (;;)
    {
        const int inpBytes = fread(inpBuf, 1, config->block_size, file_in);
        if (0 == inpBytes) {
            break;
        }

        LZ4_resetStreamHC(lz4Stream, config->compression_level);

        int cmpBytes = LZ4_compress_HC_continue(lz4Stream, inpBuf, cmpBuf, inpBytes, LZ4_compressBound(config->block_size));
        if (cmpBytes <= 0)
            return LZ41_COMPRESSION_ERROR;

        fwrite(cmpBuf, (size_t)cmpBytes, 1, file_out);
        written_bytes += (size_t)cmpBytes;

        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }
    free(inpBuf);
    free(cmpBuf);
    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd)
    {
        fwrite(ptr++, sizeof(int), 1, file_out);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    fwrite(&i, sizeof(int), 1, file_out);
    written_bytes += sizeof(int);

    /* Write Filesize */
    fwrite(&file_size, 4, 1, file_out);
    written_bytes += sizeof(int);

    /* Write Block Size */
    fwrite(&config->block_size, 4, 1, file_out);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_stream_decompress(FILE* file_in, FILE* file_out)
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
    int offset = 0;
    int length = 0;
    int written_bytes = 0;
    int block_size = 0;
    int numOffsets;

    /* Num Offsets */
    fseek(file_in, -12, SEEK_END);
    fread(&numOffsets, sizeof(int), 1, file_in);

    /* File Size */
    fread(&length, sizeof(int), 1, file_in);

    /* Block Size */
    fread(&block_size, sizeof(int), 1, file_in);

    fseek(file_in, 0, SEEK_SET);
    /*Read Header*/
    char header[sizeof(LZ41_FILE_HEADER)];
    fread(header, 1, sizeof(LZ41_FILE_HEADER), file_in);
    if (memcmp(LZ41_FILE_HEADER, header, sizeof(LZ41_FILE_HEADER)))
        return LZ41_HEADER_MISMATCH;

    char* decBuf = (char*)malloc(block_size);
    fseek(file_in, 0, SEEK_END);
    int* offsets = (int*)malloc(numOffsets*sizeof(int));
    fseek(file_in, 0, SEEK_SET);

    /* The blocks [currentBlock, endBlock) contain the data we want */
    int currentBlock = offset / block_size;
    int endBlock = ((offset + length - 1) / block_size) + 1;

    /* Special cases */
    if (length == 0)
        return 0;

    /* Read the offsets tail */
    int block;
    int* offsetsPtr = offsets;
    if (numOffsets <= endBlock)
        return LZ41_OFFSETS_MISMATCH;
    fseek(file_in, (-4 * (numOffsets + 1)) - (sizeof(int) * 2), SEEK_END);
    for (block = 0; block <= endBlock; ++block)
        fread(offsetsPtr++, sizeof(int), 1, file_in);

    /* Seek to the first block to read */
    fseek(file_in, offsets[currentBlock], SEEK_SET);
    offset = offset % block_size;

    /* Start decoding */
    char* cmpBuf = (char*)malloc(LZ4_compressBound(block_size));
    for (; currentBlock < endBlock; ++currentBlock)
    {
        /* The difference in offsets is the size of the block */
        int  cmpBytes = offsets[currentBlock + 1] - offsets[currentBlock];
        fread(cmpBuf, (size_t)cmpBytes, 1, file_in);

        const int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decBuf, cmpBytes, block_size);
        if (decBytes <= 0)
            return LZ41_DECOMPRESSION_ERROR;

        /* Write out the part of the data we care about */
        int blockLength = ((length) < ((decBytes - offset)) ? (length) : ((decBytes - offset)));
        fwrite(decBuf + offset, (size_t)blockLength, 1, file_out);
        written_bytes += blockLength;
        offset = 0;
        length -= blockLength;
    }
    free(cmpBuf);
    free(decBuf);
    free(offsets);
    return written_bytes;
}

void compress_single_file(char* fn_in, char* fn_out, const LZ41CONFIG *config, THREAD_INFO* ti)
{
    int size, final_size;
    FILE* file_in = fopen(fn_in, "rb");
    FILE* file_out = fopen(fn_out, "wb");
    strcpy(ti->vp_file, "\0\n");
    if (file_in != NULL)
    {
        ti->max_files = 1;
        if (config->verbose)
            printf("\n[THREAD #%d] COMPRESSING FILE: %s", ti->thread_num, fn_in);
        if (config->log != nullptr)
            fprintf(config->log, "\n[THREAD #%d] COMPRESSING FILE: %s", ti->thread_num, fn_in);

        fseek(file_in, 0, SEEK_END);
        size = ftell(file_in);
        fseek(file_in, 0, SEEK_SET);

        char* file_extension = strrchr(fn_in, '.');
        if (file_extension != nullptr)
        {
            std::string str = file_extension;
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            strcpy(file_extension, str.c_str());
        }
        if (file_extension != nullptr && !strstr(config->ignore_list, file_extension) && ( size >= config->minimum_size || (strstr(file_extension, ".pof") && config->fix_pof) ) )
        {
            char header[sizeof(LZ41_FILE_HEADER)];
            fread(header, 1, sizeof(LZ41_FILE_HEADER), file_in);
            if (memcmp(LZ41_FILE_HEADER, header, sizeof(LZ41_FILE_HEADER)) != 0)
            {
                int version;
                fseek(file_in, 4, SEEK_SET);
                fread(&version, sizeof(int), 1, file_in);

                if (strstr(file_extension, ".pof") && config->fix_pof && version == 2117)
                {
                    fseek(file_in, 0, SEEK_SET);

                    char* bytes_in = (char*)malloc(size);
                    char* fixed_pof = (char*)malloc(size * 2);
                    memset(fixed_pof, '\0', size * 2);
                    fread(bytes_in, 1, size, file_in);

                    if (config->log != nullptr)
                        fprintf(config->log, "\n[THREAD #%d] ==> CONVERTING POF %s TO VERSION 2200...\n", ti->thread_num, fn_in);
                    if (config->verbose)
                        printf("\n[THREAD #%d] ==> CONVERTING POF %s TO VERSION 2200...\n", ti->thread_num, fn_in);

                    size = align_pof(bytes_in, size, fixed_pof, false);
                    free(bytes_in);
                    char* bytes_out = (char*)malloc(LZ4_COMPRESSBOUND(size));

                    if (config->compression_level <= 4)
                        final_size = lz41_compress_memory(fixed_pof,bytes_out,size,config);
                    else
                        final_size = lz41_compress_memory_HC(fixed_pof, bytes_out, size, config);

                    fwrite(bytes_out, final_size, 1, file_out);
                    free(bytes_out);
                    free(fixed_pof);
                }
                else
                {
                    fseek(file_in, 0, SEEK_SET);
                    unsigned int max_memory = LZ4_COMPRESSBOUND(size);

                    if (config->compression_level <= 4)
                        final_size = lz41_stream_compress(file_in, file_out, config);
                    else
                        final_size = lz41_stream_compress_HC(file_in, file_out, config);
                }

                fclose(file_out);
                fclose(file_in);

                if (final_size >= size || final_size <= 0)
                {
                    fs::copy_file(fn_in, fn_out, fs::copy_options::overwrite_existing);

                    if (final_size <= 0)
                    {
                        if (config->log != nullptr)
                            fprintf(config->log, "\n[THREAD #%d] Skipping %s compression error. Result: %d\n", ti->thread_num, fn_in, final_size);
                        if (config->verbose)
                            printf("\n[THREAD #%d] Skipping %s compression error. Result: %d\n", ti->thread_num, fn_in, final_size);
                    }
                    else
                    {
                        if (config->log != nullptr)
                            fprintf(config->log, "\n[THREAD #%d] Skipping %s compressed is larger than the original C: %d | O: %d\n", ti->thread_num, fn_in, final_size, size);
                        if (config->verbose)
                            printf("\n[THREAD #%d] Skipping %s compressed is larger than the original C: %d | O: %d\n", ti->thread_num, fn_in, final_size, size);
                    }
                }

                if (config->verbose)
                    printf("\n[THREAD #%d] COMPRESSING FILE: %s DONE! OS: %d | CS: %d", ti->thread_num, fn_in, size, final_size);
                if (config->log != nullptr)
                    fprintf(config->log, "\n[THREAD #%d] COMPRESSING FILE: %s DONE! OS: %d | CS: %d", ti->thread_num, fn_in, size, final_size);
            }
            else
            {
                fclose(file_out);
                fclose(file_in);
                fs::copy_file(fn_in, fn_out, fs::copy_options::overwrite_existing);
                if (config->log != nullptr)
                    fprintf(config->log, "\n[THREAD #%d] Skipping %s file is already compressed.\n", ti->thread_num, fn_in);
                if (config->verbose)
                    printf("\n[THREAD #%d] Skipping %s file is already compressed.\n", ti->thread_num, fn_in);
            }
        }
        else
        {
            fclose(file_out);
            fclose(file_in);
            fs::copy_file(fn_in, fn_out, fs::copy_options::overwrite_existing);
            if (config->log != nullptr)
                fprintf(config->log, "\n[THREAD #%d] Skipping %s extension %s is set to ignore or file is below minimal size. Filesize: %d | Minimum: %d\n", ti->thread_num, fn_in, file_extension, size, config->minimum_size);
            if (config->verbose)
                printf("\n[THREAD #%d] Skipping %s extension %s is set to ignore or file is below minimal size. Filesize: %d | Minimum: %d\n", ti->thread_num, fn_in, file_extension, size, config->minimum_size);
        }
    }
    else
    {
        if(config->verbose)
            printf("\n[THREAD #%d] Unable to open file : %s", ti->thread_num, fn_in);
    }
    ti->finished_files = 1;
    free(fn_in);
    free(fn_out);
}

void decompress_single_file(const char* fn_in, const char* fn_out, const LZ41CONFIG *config)
{
    FILE* file_in = fopen(fn_in, "rb");
    FILE* file_out = fopen(fn_out, "wb");
    if (file_in != NULL)
    {
        if(config->verbose)
            std::cout << "\nDECOMPRESSING FILE:" << fn_in << "\n";
        if(config->log!=nullptr)
            fprintf(config->log, "\nDOMPRESSING FILE: %s", fn_in);

        int final_size=lz41_stream_decompress(file_in, file_out);

        if (final_size == LZ41_HEADER_MISMATCH)
        {
            if (config->verbose)
                std::cout << "\n" << "File : " << fn_in << "\n" << " was not compressed, copying..." << "\n";
            if (config->log != nullptr)
                fprintf(config->log, "\nFile %s was not compressed, copying...", fn_in);
            fs::copy_file(fn_in, fn_out, fs::copy_options::overwrite_existing);

        }
        else if (final_size <= 0) 
        {
            if (config->verbose)
                std::cout << "\n" << "Unable to open file : " << fn_in << "\n" << "or file header does not match" << "\n";
            if (config->log != nullptr)
                fprintf(config->log, "\nUnable to open file %s or file header does not match", fn_in);
        }
        fclose(file_in);
        fclose(file_out);
        if(config->verbose)
            std::cout << "\nDECOMPRESSING FILE DONE!";
        if (config->log != nullptr)
            fprintf(config->log, "\nDECOMPRESSING FILE %s DONE!", fn_in);
    }
}

void compress_vp(char *fn_in, char *fn_out, const LZ41CONFIG *config, THREAD_INFO* ti)
{
    char header[5];
    bool is_compressed = false;
    unsigned int version, index_offset, numfiles;
    FILE* vp_in = fopen(fn_in, "rb");
    FILE* vp_out = fopen(fn_out, "wb");

    if (config->verbose)
        printf("\n[THREAD #%d] COMPRESSING VP FILE: %s", ti->thread_num,fn_in);
    if(config->log!=nullptr)
        fprintf(config->log,"\n[THREAD #%d] COMPRESSING VP FILE: %s", ti->thread_num, fn_in);
    if (!read_vp_header(vp_in, header, &version, &index_offset, &numfiles) == 1)
    {
        fclose(vp_in);
        fclose(vp_out);
        free(fn_in);
        free(fn_out);
        if (config->log != nullptr)
            fprintf(config->log, "\n[THREAD #%d] Not a VP file: %s", ti->thread_num,fn_in);
        if(config->verbose)
            printf("\n[THREAD #%d] Not a VP file: %s", ti->thread_num, fn_in);
        ti->finished_files = 0;
        ti->max_files = 0;
        return;
    }
    ti->max_files = numfiles;
    vp_index_entry* index;
    index = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);
    load_vp_index(vp_in, index, index_offset, numfiles);

    unsigned int wvp_num_files = 0, wvp_index_offset = 16;
    vp_index_entry* index_out;
    index_out = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);

    for (unsigned int x = 0; x < numfiles; x++)
    {
        if (index[x].offset != 0 && index[x].filesize != 0 && index[x].timestamp != 0)
        {
            char* file_extension = strrchr(index[x].name, '.');
            char* file = load_vp_file(vp_in, &index[x]);
            strcpy(ti->vp_file, index[x].name);

            if (file_extension != nullptr)
            {
                std::string str = file_extension;
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                strcpy(file_extension, str.c_str());
            }

            /*Check if file is already compressed*/
            if (memcmp(LZ41_FILE_HEADER, file, sizeof(LZ41_FILE_HEADER)) == 0 )
            {
                if (config->log != nullptr)
                    fprintf(config->log, "\n[THREAD #%d] Skipping %s file is already compressed.\n", ti->thread_num, index[x].name);
                if (config->verbose)
                    printf("\n[THREAD #%d] Skipping %s file is already compressed.\n", ti->thread_num, index[x].name);
                write_vp_file(vp_out, file, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
                free(file);
            }
            else
            {
                /* Do pof conversion */
                if (file_extension != nullptr && strstr(file_extension, ".pof") && config->fix_pof)
                {
                    int version;
                    memcpy(&version, file + 4, 4);
                    if (version == 2117)
                    {
                        if (config->log != nullptr)
                            fprintf(config->log, "\n[THREAD #%d] ==> CONVERTING POF %s TO VERSION 2200...\n", ti->thread_num, index[x].name);
                        if (config->verbose)
                            printf("\n[THREAD #%d] ==> CONVERTING POF %s TO VERSION 2200...\n", ti->thread_num, index[x].name);
                        char* fixed_pof = (char*)malloc(index[x].filesize * 2);
                        memset(fixed_pof, '\0', index[x].filesize * 2);
                        index[x].filesize = align_pof(file, index[x].filesize, fixed_pof, false);
                        index[x].timestamp = getUnixTime();
                        free(file);
                        file = fixed_pof;
                    }
                }

                /*Check extension and minimal size*/
                if (file_extension != nullptr && !strstr(config->ignore_list, file_extension) && (int)index[x].filesize >= config->minimum_size)
                {
                    char* compressed_bytes;
                    compressed_bytes = (char*)malloc(LZ4_compressBound(index[x].filesize) * 1.5);

                    int newsize;
                    if (config->compression_level <= 4)
                        newsize = lz41_compress_memory(file, compressed_bytes, index[x].filesize, config);
                    else
                        newsize = lz41_compress_memory_HC(file, compressed_bytes, index[x].filesize, config);

                    if (newsize <= 0)
                    {
                        if (config->log != nullptr)
                            fprintf(config->log, "\n[THREAD #%d] COMPRESSION ERROR: %d File: %s\n", ti->thread_num, newsize, index[x].name);
                        if (config->verbose)
                            printf("\n[THREAD #%d] COMPRESSION ERROR: %d File: %s\n", ti->thread_num, newsize, index[x].name);
                    }

                    /*Do not write compressed files that are larger than the original, also skip errors*/
                    if (newsize >= (int)index[x].filesize || newsize <= 0)
                    {
                        if (newsize <= 0)
                        {
                            if (config->log != nullptr)
                                fprintf(config->log, "\n[THREAD #%d] Skipping %s compression error. Result: %d\n", ti->thread_num, index[x].name, newsize);
                            if(config->verbose)
                                printf("\n[THREAD #%d] Skipping %s compression error. Result: %d\n", ti->thread_num, index[x].name, newsize);
                        }
                        else
                        {
                            if (config->log != nullptr)
                                fprintf(config->log, "\n[THREAD #%d] Skipping %s compressed is larger than the original C: %d | O: %d\n", ti->thread_num, index[x].name, newsize, index[x].filesize);
                            if (config->verbose)
                                printf("\n[THREAD #%d] Skipping %s compressed is larger than the original C: %d | O: %d\n", ti->thread_num, index[x].name, newsize, index[x].filesize);
                        }
                        write_vp_file(vp_out, file, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);

                    }
                    else
                    {
                        is_compressed = true;
                        write_vp_file(vp_out, compressed_bytes, index[x].name, newsize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
                    }
                    free(compressed_bytes);
                    free(file);
                }
                else
                {
                    if (config->log != nullptr)
                        fprintf(config->log, "\n[THREAD #%d] Skipping %s extension %s is set to ignore or file is below minimal size. Filesize: %d | Minimum: %d\n", ti->thread_num, index[x].name, file_extension, index[x].filesize, config->minimum_size);
                    if (config->verbose)
                        printf("\n[THREAD #%d] Skipping %s extension %s is set to ignore or file is below minimal size. Filesize: %d | Minimum: %d\n", ti->thread_num, index[x].name, file_extension, index[x].filesize, config->minimum_size);
                    write_vp_file(vp_out, file, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
                    free(file);
                }
            }
        }
        else
        {
            write_vp_file(vp_out, NULL, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
        }
        ti->finished_files = x;
    }
    fclose(vp_in);
    fclose(vp_out);
    ti->finished_files = ti->max_files;

    /* Tag Compressed VPs */
    try {
        if (fs::exists(fn_out) && is_compressed && config->tag_c_vps)
        {
            std::string path = fs::path(fn_out).string();
            std::string pathout = path.substr(0, path.length() - 3).append("_vpc.vp");
            fs::rename(path, pathout);
        }
    }catch (const fs::filesystem_error& e)
    {

    }

    strcpy(ti->vp_file, "FINISHED!");
    if (config->log != nullptr)
        fprintf(config->log, "\n[THREAD #%d] COMPRESSING VP %s DONE!", ti->thread_num, fn_in);
    if(config->verbose)
        printf("\n[THREAD #%d] COMPRESSING VP %s DONE!", ti->thread_num,fn_in);
    free(fn_in);
    free(fn_out);
}

void decompress_vp(const char* fn_in, const char* fn_out, const LZ41CONFIG *config)
{
    char header[5];
    unsigned int version, index_offset, numfiles, count = 1, wvp_num_files = 0, wvp_index_offset = 16;
    FILE* vp_in = fopen(fn_in, "rb");

    std::string temp_out=fn_out;
    size_t pos = temp_out.find("_vpc.vp");
    if(pos != std::string::npos)
        temp_out.replace(pos, 7, ".vp");

    FILE* vp_out = fopen(temp_out.c_str(), "wb");
    if(config->verbose)
        std::cout << "\nDECOMPRESSING FILE : " << fn_in;
    if (config->log != nullptr)
        fprintf(config->log, "\nDOMPRESSING FILE: %s", fn_in);

    if (!read_vp_header(vp_in, header, &version, &index_offset, &numfiles) == 1)
    {
        fclose(vp_in);
        fclose(vp_out);
        if (config->log != nullptr)
            fprintf(config->log, "\nNo a VP file %s.\n", fn_in);
        if (config->verbose)
            printf("\nNo a VP file %s.\n", fn_in);
        return;
    }

    vp_index_entry* index = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);
    load_vp_index(vp_in, index, index_offset, numfiles);

    vp_index_entry* index_out = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);

    for (unsigned int x = 0; x < numfiles; x++)
    {
        if (index[x].offset != 0 && index[x].filesize != 0 && index[x].timestamp != 0)
        {
            char* file;
            file = load_vp_file(vp_in, &index[x]);
            int original_size;
            char* uncompressed_bytes;
            char file_header[5];
            file_header[4] = '\0';

            memcpy(file_header, file, 4);
            memcpy(&original_size, file+(index[x].filesize-8), sizeof(int));
            
            if (strcmp(file_header, LZ41_FILE_HEADER) == 0)
            {
                uncompressed_bytes = (char*)malloc(original_size);

                int result=lz41_decompress_memory(file, uncompressed_bytes, index[x].filesize);
                if (result <= 0)
                {
                    if(config->verbose)
                        std::cout << "DECOMPRESSION ERROR: " << result << " File:" << index[x].name;
                    if (config->log != nullptr)
                        fprintf(config->log, "\nDECOMPRESSION ERROR: File: %s Code: %d .\n", index[x].name,result);
                }
                write_vp_file(vp_out, uncompressed_bytes, index[x].name, original_size, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);

                free(file);
                free(uncompressed_bytes);
            }
            else
            {
                write_vp_file(vp_out, file, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
                free(file);
            }
            count++;
        }
        else
        {
            write_vp_file(vp_out, NULL, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
            count++;
        }
    }
    fclose(vp_in);
    fclose(vp_out);
    if (config->log != nullptr)
        fprintf(config->log, "\nDOMPRESSING FILE: %s DONE!", fn_in);
    if(config->verbose)
        std::cout << "\nDECOMPRESSING FILE DONE!";
}

