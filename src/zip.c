#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define INPUT_FILE_MAX_SIZE 1024 * 1024 // 1MiB

void     get_current_datetime(uint16_t* dos_time, uint16_t* dos_date);
uint32_t crc32(uint8_t* data, size_t length);

int main(int argc, char* argv[])
{
    const uint8_t MAGIC_FILE_HEADER[4]                     = {0x50, 0x4B, 0x03, 0x04};
    const uint8_t MAGIC_CENTRAL_DIRECTORY_FILE_HEADER[4]   = {0x50, 0x4B, 0x01, 0x02};
    const uint8_t MAGIC_END_OF_CENTRAL_DIRECTORY_RECORD[4] = {0x50, 0x4B, 0x05, 0x06};
    const uint8_t VERSION[2]                               = {0x0A, 0x00}; // v1.0;
    const uint8_t FLAGS[2]                                 = {0x00, 0x00}; // None
    const uint8_t COMPRESSION[2]                           = {0x00, 0x00}; // Store (no compression)
    const uint8_t ZERO_16BIT[2]                            = {0x00, 0x00};
    const uint8_t ZERO_32BIT[4]                            = {0x00, 0x00, 0x00, 0x00};

    uint16_t dos_time;
    uint16_t dos_date;

    get_current_datetime(&dos_time, &dos_date);

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <output.zip> <input1> [<input2> ... <inputN>]\n", argv[0]);
        return 1;
    }

    FILE* zip_file = fopen(argv[1], "wb");
    if (!zip_file) {
        fprintf(stderr, "Error: Unable to create zip file %s\n", argv[1]);
        return 1;
    }

    // File Header
    for (int i = 2; i < argc; i++) {
        FILE* input_file = fopen(argv[i], "rb");

        if (!input_file) {
            fprintf(stderr, "Error: Unable to open input file %s\n", argv[i]);
            fclose(zip_file);
            return 1;
        }

        struct stat st;

        if (stat(argv[i], &st) != 0) {
            perror("stat");
            return 1;
        }

        if ((size_t)st.st_size > (size_t)UINT32_MAX) {
            fprintf(stderr, "Error: Input file %s is too large (size: %lld bytes)\n", argv[i], (long long)st.st_size);
            fclose(input_file);
            fclose(zip_file);
            return 1;
        }

        uint32_t input_file_size = (uint32_t)st.st_size;

        if (input_file_size > INPUT_FILE_MAX_SIZE) {
            fprintf(stderr, "Error: Input file %s exceeds the maximum allowed size of %d bytes\n", argv[i], INPUT_FILE_MAX_SIZE);
            fclose(input_file);
            fclose(zip_file);
            return 1;
        }

        uint8_t* input_file_data = (uint8_t*)malloc(input_file_size);
        if (!input_file_data) {
            fprintf(stderr, "Error: Memory allocation failed for input file %s\n", argv[i]);
            fclose(input_file);
            fclose(zip_file);
            return 1;
        }

        fread(input_file_data, input_file_size, 1, input_file);

        uint32_t crc32_value      = crc32(input_file_data, input_file_size);
        uint16_t file_name_length = (uint16_t)strlen(argv[i]);

        fwrite(MAGIC_FILE_HEADER, sizeof(MAGIC_FILE_HEADER), 1, zip_file);
        fwrite(VERSION, sizeof(VERSION), 1, zip_file);                    // Version needed to min.
        fwrite(FLAGS, sizeof(FLAGS), 1, zip_file);                        // Flags
        fwrite(COMPRESSION, sizeof(COMPRESSION), 1, zip_file);            // Compression method
        fwrite(&dos_time, sizeof(dos_time), 1, zip_file);                 // Last modification time
        fwrite(&dos_date, sizeof(dos_date), 1, zip_file);                 // Last modification date
        fwrite(&crc32_value, sizeof(crc32_value), 1, zip_file);           // CRC32 of uncompressed data
        fwrite(&input_file_size, sizeof(input_file_size), 1, zip_file);   // Compressed size
        fwrite(&input_file_size, sizeof(input_file_size), 1, zip_file);   // Uncompressed size
        fwrite(&file_name_length, sizeof(file_name_length), 1, zip_file); // File name length
        fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);              // Extra field length
        fwrite(argv[i], strlen(argv[i]), 1, zip_file);                    // File name
        fwrite(input_file_data, input_file_size, 1, zip_file);            // File data

        free(input_file_data);
        fclose(input_file);
    }

    // Central Directory File Header
    uint32_t central_directory_offset = (uint32_t)ftell(zip_file);
    uint32_t local_file_header_offset = 0;
    for (int i = 2; i < argc; i++) {
        FILE* input_file = fopen(argv[i], "rb");

        if (!input_file) {
            fprintf(stderr, "Error: Unable to open input file %s\n", argv[i]);
            fclose(zip_file);
            return 1;
        }

        struct stat st;

        if (stat(argv[i], &st) != 0) {
            perror("stat");
            return 1;
        }

        if ((size_t)st.st_size > (size_t)UINT32_MAX) {
            fprintf(stderr, "Error: Input file %s is too large (size: %lld bytes)\n", argv[i], (long long)st.st_size);
            fclose(input_file);
            fclose(zip_file);
            return 1;
        }

        uint32_t input_file_size = (uint32_t)st.st_size;

        uint8_t* input_file_data = (uint8_t*)malloc(input_file_size);
        if (!input_file_data) {
            fprintf(stderr, "Error: Memory allocation failed for input file %s\n", argv[i]);
            fclose(input_file);
            fclose(zip_file);
            return 1;
        }

        fread(input_file_data, input_file_size, 1, input_file);

        uint32_t crc32_value      = crc32(input_file_data, input_file_size);
        uint16_t file_name_length = (uint16_t)strlen(argv[i]);

        fwrite(MAGIC_CENTRAL_DIRECTORY_FILE_HEADER, sizeof(MAGIC_CENTRAL_DIRECTORY_FILE_HEADER), 1, zip_file);
        fwrite(VERSION, sizeof(VERSION), 1, zip_file);                                    // Version made by
        fwrite(VERSION, sizeof(VERSION), 1, zip_file);                                    // Version needed to min.
        fwrite(FLAGS, sizeof(FLAGS), 1, zip_file);                                        // Flags
        fwrite(COMPRESSION, sizeof(COMPRESSION), 1, zip_file);                            // Compression method
        fwrite(&dos_time, sizeof(dos_time), 1, zip_file);                                 // Last modification time
        fwrite(&dos_date, sizeof(dos_date), 1, zip_file);                                 // Last modification date
        fwrite(&crc32_value, sizeof(crc32_value), 1, zip_file);                           // CRC32 of uncompressed data
        fwrite(&input_file_size, sizeof(input_file_size), 1, zip_file);                   // Compressed size
        fwrite(&input_file_size, sizeof(input_file_size), 1, zip_file);                   // Uncompressed size
        fwrite(&file_name_length, sizeof(file_name_length), 1, zip_file);                 // File name length
        fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // Extra field length
        fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // File comment length
        fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // Disk number start
        fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // Internal file attributes
        fwrite(ZERO_32BIT, sizeof(ZERO_32BIT), 1, zip_file);                              // External file attributes
        fwrite(&local_file_header_offset, sizeof(local_file_header_offset), 1, zip_file); // Relative offset of local header
        fwrite(argv[i], strlen(argv[i]), 1, zip_file);                                    // File name

        local_file_header_offset += (uint32_t)(sizeof(MAGIC_FILE_HEADER) + sizeof(VERSION) + sizeof(FLAGS) + sizeof(COMPRESSION) + sizeof(dos_time) + sizeof(dos_date) + sizeof(uint32_t) * 3 + sizeof(file_name_length) + sizeof(ZERO_16BIT) + file_name_length + input_file_size);
    }

    uint16_t total_entries          = (uint16_t)(argc - 2);
    uint32_t central_directory_size = (uint32_t)ftell(zip_file) - central_directory_offset;

    // End of Central Directory Record
    fwrite(MAGIC_END_OF_CENTRAL_DIRECTORY_RECORD, sizeof(MAGIC_END_OF_CENTRAL_DIRECTORY_RECORD), 1, zip_file);
    fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // Number of this disk
    fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // Disk where central directory starts
    fwrite(&total_entries, sizeof(total_entries), 1, zip_file);                       // Number of central directory records on this disk
    fwrite(&total_entries, sizeof(total_entries), 1, zip_file);                       // Total number of central directory records
    fwrite(&central_directory_size, sizeof(central_directory_size), 1, zip_file);     // Size of central directory (bytes)
    fwrite(&central_directory_offset, sizeof(central_directory_offset), 1, zip_file); // Relative offset of central directory
    fwrite(ZERO_16BIT, sizeof(ZERO_16BIT), 1, zip_file);                              // Comment length

    fclose(zip_file);

    return 0;
}

void get_current_datetime(uint16_t* dos_time, uint16_t* dos_date)
{
    time_t     now   = time(NULL);
    struct tm* local = localtime(&now);

    if (dos_time == NULL || dos_date == NULL) {
        return;
    }

    if (local == NULL) {
        return;
    }

    *dos_time = (local->tm_hour << 11) | (local->tm_min << 5) | (local->tm_sec / 2);
    *dos_date = ((local->tm_year - 80) << 9) | ((local->tm_mon + 1) << 5) | local->tm_mday;
}

uint32_t crc32(uint8_t* data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= (uint8_t)data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}
