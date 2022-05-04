#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

#include "converter.c"

#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))

uint8_t scratch_buffer[8192];

uint32_t maav101_crc32s[] = {
    0x5018a455,
    0xa3606ef9
};

uint32_t reverse_u32(uint32_t x) {
    x = ((x & 0x55555555) <<  1) | ((x >>  1) & 0x55555555);
    x = ((x & 0x33333333) <<  2) | ((x >>  2) & 0x33333333);
    x = ((x & 0x0F0F0F0F) <<  4) | ((x >>  4) & 0x0F0F0F0F);
    x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
    return x;
}

unsigned int crc32(uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;

    for(uint32_t i = 0; i < len; i++)
    {
        uint32_t byte = reverse_u32(data[i]);
        for (int j = 0; j <= 7; j++)
        {
            if ((int)(crc ^ byte) < 0) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc = crc << 1;
            }

            byte = byte << 1;
        }
    }

    return reverse_u32(~crc);
}

bool inside_array(uint32_t value, uint32_t *array, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        if(array[i] == value) {
            return true;
        }
    }

    return false;
}

// All of decode/encode code is referenced from here
// https://github.com/pok3r-custom/pok3rtool
void decode_data(uint8_t *data, uint32_t len)
{
    for(uint32_t i = 4; i < len; i += 5)
    {
        uint8_t a = data[i-4];
        uint8_t b = data[i];
        data[i-4] = b;
        data[i] = a;
    }

    for(uint32_t i = 1; i < len; i += 2)
    {
        uint8_t d = data[i-1];
        uint8_t b = data[i];
        data[i-1] = b;
        data[i] = d;
    }

    for(uint32_t i = 0; i < len; i++)
    {
        data[i] = (((data[i] - 7) << 4) + (data[i] >> 4)) & 0xFF;
    }
}


void xor_decode_encode(uint8_t *data, uint32_t len){
    static const uint32_t xor_key[] = {
        0xe7c29474,
        0x79084b10,
        0x53d54b0d,
        0xfc1e8f32,
        0x48e81a9b,
        0x773c808e,
        0xb7483552,
        0xd9cb8c76,
        0x2a8c8bc6,
        0x0967ada8,
        0xd4520f5c,
        0xd0c3279d,
        0xeac091c5,
    };

    uint32_t *words = (uint32_t *)data;
    for(uint32_t i = 0; i < len / 4; i++){
        words[i] = words[i] ^ xor_key[i % 13];
    }
}

void decode_firmware_packet(uint8_t *data, uint32_t num){
    const uint8_t swap_key[] = {
        0,1,2,3,
        1,2,3,0,
        2,1,3,0,
        3,2,1,0,
        3,1,0,2,
        1,2,0,3,
        2,3,1,0,
        0,2,1,3,
    };

    static const uint32_t xor_key[] = {
        0x55aa55aa,
        0xaa55aa55,
        0x000000ff,
        0x0000ff00,
        0x00ff0000,
        0xff000000,
        0x00000000,
        0xffffffff,
        0x0f0f0f0f,
        0xf0f0f0f0,
        0xaaaaaaaa,
        0x55555555,
        0x00000000,
    };

    uint32_t *words = (uint32_t*)data;
    for(int i = 0; i < 13; ++i){
        words[i] = words[i] ^ xor_key[i];
    }

    uint8_t f = (num & 7) << 2;
    for(int i = 0; i < 52; i+=4){
        uint8_t a = data[i + swap_key[f + 0]];
        uint8_t b = data[i + swap_key[f + 1]];
        uint8_t c = data[i + swap_key[f + 2]];
        uint8_t d = data[i + swap_key[f + 3]];

        data[i + 0] = a;
        data[i + 1] = b;
        data[i + 2] = c;
        data[i + 3] = d;
    }
}

void decode_firmware(uint8_t *data, uint32_t len){
    uint32_t count = 0;
    for(uint32_t offset = 0; offset < len; offset += 52, count++)
    {
        if(count >= 10 && count <= 100) {
            decode_firmware_packet(data + offset, count);
        }
    }
}

const char *utf16_to_utf8_scrachted(char *str, int max_len)
{
    size_t output_len = sizeof(utf8_t) * utf16_to_utf8((const utf16_t *)str, max_len / sizeof(utf16_t), NULL, 0);
    utf16_to_utf8((const utf16_t *)str, max_len / sizeof(utf16_t), scratch_buffer, output_len / sizeof(utf8_t));
    return scratch_buffer;
}

void decode_maav101(FILE *exe, char *buff, const char *output_filename) {
    int32_t strings_len = 0x4bc;
    fseek(exe, -strings_len, SEEK_END);
    fread(buff, 1, strings_len, exe);

    decode_data(buff, strings_len);

    uint32_t vendor_offset = 0x10;
    uint32_t product_offset = 0x218;
    uint32_t version_offset = 0x461;
    uint32_t signature_offset = 0x4af;

    printf("Vendor:    %s\n", utf16_to_utf8_scrachted(buff + vendor_offset, strings_len - vendor_offset));
    printf("Product:   %s\n", utf16_to_utf8_scrachted(buff + product_offset, strings_len - product_offset));
    printf("Version:   %s\n", buff + version_offset);
    printf("Signature: %s\n", buff + signature_offset);

    uint32_t fw_len_offset = 0x420;
    uint32_t fw_layout_offset = 0x424;

    uint32_t fw_len = *(uint32_t *)(buff + fw_len_offset);
    printf("FW Length: %u\n", fw_len);
    printf("FW Layout: %s\n", utf16_to_utf8_scrachted(buff + fw_layout_offset, strings_len - fw_layout_offset));

    int32_t fw_start = strings_len + fw_len;
    printf("FW offset: 0x%x\n", fw_start);

    fseek(exe, -fw_start, SEEK_END);
    fread(buff, 1, fw_len, exe);
    fclose(exe);

    decode_data(buff, fw_len);
    decode_firmware(buff, fw_len);

    FILE *out = fopen(output_filename, "wb");
    assert(out);
    fwrite(buff, fw_len, 1, out);

    fclose(out);
}

int main(int argc, char **argv)
{
    if(argc != 3) {
        printf("Usage: %s <FWupdate.exe> <output.bin>\n", argv[0]);
        return 0;
    }

    const char *filename = argv[1];
    const char *output_filename = argv[2];
    FILE *exe = fopen(filename, "rb");
    assert(exe);
    fseek(exe, 0, SEEK_END);
    int file_len = ftell(exe);

    uint8_t *buff = malloc(file_len);
    memset(buff, 0, file_len);

    fseek(exe, 0, SEEK_SET);
    fread(buff, 1, file_len, exe);
    uint32_t crc = crc32(buff, file_len);
    printf("crc32: 0x%08x\n", crc);

    if(inside_array(crc, maav101_crc32s, ARRAY_LEN(maav101_crc32s))) {
        decode_maav101(exe, buff, output_filename);
    } else {
        printf("Didn't recognized that file\n");
    }

    return 0;
}
