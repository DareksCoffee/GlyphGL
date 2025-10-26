/*
    MIT License

    Copyright (c) 2025 Darek

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef __GLYPH_IMAGE_h
#define __GLYPH_IMAGE_h

#ifdef __cplusplus
extern "C" {}
#endif

// Cross-platform includes
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdint.h>
    #include <math.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <stdint.h>
        #include <math.h>
    #endif
#elif defined(__linux__) || defined(__unix__)
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdint.h>
    #include <math.h>
#endif

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned char* data;
} glyph_image_t;

/*
    Creates a new glyph image structure setting its width and height and allocating memory for the image data
    which is stored as RGB values three bytes per pixel in row-major order from top to bottom
    quite a dumb fucking function to be honest
*/
static glyph_image_t glyph_image_create(unsigned int width, unsigned int height) {
    glyph_image_t img;
    img.width = width;
    img.height = height;
    img.data = (unsigned char*)malloc((size_t)width * height * 3);
    return img;
}

static void glyph_image_free(glyph_image_t* img) {
    if (!img) return;
    free(img->data);
    img->data = NULL;
}

static uint32_t crc32_table[256];
static int crc32_table_inited = 0;

/*  
    Initializes a lookup table for CRC32 calculations if it hasn't been done already 
    using the standard polynomial for IEEE 8023 
*/
static void crc32_init_table(void) {
    if (crc32_table_inited) return;
    uint32_t poly = 0xEDB88320u;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) crc = (crc >> 1) ^ poly;
            else crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_inited = 1;
}

/*
    Computes the CRC32 checksum of the given data using the precomputed table
    it's a way to verify data integrity like a digital fingerprint that changes if even one bit is wrong
    useful for detecting corruption in files
*/
static uint32_t crc32(const unsigned char* data, size_t len) {
    crc32_init_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        uint8_t idx = (uint8_t)(crc ^ data[i]);
        crc = (crc >> 8) ^ crc32_table[idx];
    }
    return crc ^ 0xFFFFFFFFu;
}

/*
    Computes the Adler32 checksum of the data which is a simpler and faster alternative to CRC32 for some uses
    it's like a quick hash that adds up the bytes in a clever way to catch errors
*/
static uint32_t adler32(const unsigned char* data, size_t len) {
    const uint32_t MOD_ADLER = 65521u;
    uint32_t a = 1;
    uint32_t b = 0;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
}

/*
Writes a 32-bit unsigned integer to a file in big-endian byte order meaning the most significant byte first
like how network protocols often send numbers to avoid endianness confusion
*/
static void write_u32_be(FILE* f, uint32_t v) {
    unsigned char b[4];
    b[0] = (v >> 24) & 0xFF;
    b[1] = (v >> 16) & 0xFF;
    b[2] = (v >> 8) & 0xFF;
    b[3] = v & 0xFF;
    fwrite(b, 1, 4, f);
}
/*
    Writes a 32-bit unsigned integer to a buffer in little-endian byte order least significant byte first
    like how Intel processors store numbers internally
*/
static void write_u32_le(unsigned char* buf, uint32_t v) {
    buf[0] = v & 0xFF;
    buf[1] = (v >> 8) & 0xFF;
    buf[2] = (v >> 16) & 0xFF;
    buf[3] = (v >> 24) & 0xFF;
}
/*
    Writes the glyph image to a BMP file format which is a simple uncompressed bitmap
    it creates the file header and info header then writes the pixel data in BGR order bottom to top with padding to align rows
    like saving a screenshot but programmatically and without the print screen key
*/
static int glyph_write_bmp(const char* filename, glyph_image_t* img) {
    if (!img || !img->data) return -1;
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    int width = (int)img->width;
    int height = (int)img->height;
    int row_size = ((width * 3 + 3) / 4) * 4;
    int data_size = row_size * height;
    int file_size = 54 + data_size;
    unsigned char fileheader[14] = {
        'B','M',
        (unsigned char)(file_size & 0xFF),
        (unsigned char)((file_size >> 8) & 0xFF),
        (unsigned char)((file_size >> 16) & 0xFF),
        (unsigned char)((file_size >> 24) & 0xFF),
        0,0,0,0,
        54,0,0,0
    };
    fwrite(fileheader, 1, 14, f);
    unsigned char infoheader[40] = {0};
    infoheader[0] = 40;
    infoheader[4] = (unsigned char)(width & 0xFF);
    infoheader[5] = (unsigned char)((width >> 8) & 0xFF);
    infoheader[6] = (unsigned char)((width >> 16) & 0xFF);
    infoheader[7] = (unsigned char)((width >> 24) & 0xFF);
    infoheader[8] = (unsigned char)(height & 0xFF);
    infoheader[9] = (unsigned char)((height >> 8) & 0xFF);
    infoheader[10] = (unsigned char)((height >> 16) & 0xFF);
    infoheader[11] = (unsigned char)((height >> 24) & 0xFF);
    infoheader[12] = 1;    // planes
    infoheader[14] = 24;   // bits per pixel
    infoheader[20] = (unsigned char)(data_size & 0xFF);
    infoheader[21] = (unsigned char)((data_size >> 8) & 0xFF);
    infoheader[22] = (unsigned char)((data_size >> 16) & 0xFF);
    infoheader[23] = (unsigned char)((data_size >> 24) & 0xFF);
    fwrite(infoheader, 1, 40, f);

    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            unsigned char r = img->data[(y * width + x) * 3 + 0];
            unsigned char g = img->data[(y * width + x) * 3 + 1];
            unsigned char b = img->data[(y * width + x) * 3 + 2];
            fputc(b, f);
            fputc(g, f);
            fputc(r, f);
        }
        int padding = (4 - (width * 3 % 4)) % 4;
        for (int p = 0; p < padding; p++) fputc(0, f);
    }
    fclose(f);
    return 0;
}
/*
    Writes the glyph image to a PNG file which is a compressed image format
    it builds the PNG structure with IHDR chunk for image info then compresses the raw pixel data using a simple stored deflate method
*/
static int glyph_write_png(const char* filename, glyph_image_t* img) {
    if (!img || !img->data) return -1;
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    const unsigned char png_sig[8] = {137,80,78,71,13,10,26,10};
    fwrite(png_sig, 1, 8, f);

    unsigned char ihdr_data[13];
    ihdr_data[0] = (img->width >> 24) & 0xFF;
    ihdr_data[1] = (img->width >> 16) & 0xFF;
    ihdr_data[2] = (img->width >> 8) & 0xFF;
    ihdr_data[3] = img->width & 0xFF;
    ihdr_data[4] = (img->height >> 24) & 0xFF;
    ihdr_data[5] = (img->height >> 16) & 0xFF;
    ihdr_data[6] = (img->height >> 8) & 0xFF;
    ihdr_data[7] = img->height & 0xFF;
    ihdr_data[8] = 8;
    ihdr_data[9] = 2;
    ihdr_data[10] = 0;
    ihdr_data[11] = 0;
    ihdr_data[12] = 0;

    write_u32_be(f, 13);
    fwrite("IHDR", 1, 4, f);
    fwrite(ihdr_data, 1, 13, f);
    {
        unsigned char* tmp = (unsigned char*)malloc(4 + 13);
        memcpy(tmp, "IHDR", 4);
        memcpy(tmp + 4, ihdr_data, 13);
        uint32_t crc = crc32(tmp, 4 + 13);
        write_u32_be(f, crc);
        free(tmp);
    }

    size_t raw_row_bytes = (size_t)img->width * 3 + 1;
    size_t raw_size = raw_row_bytes * img->height;
    unsigned char* raw = (unsigned char*)malloc(raw_size);
    if (!raw) { fclose(f); return -1; }
    int bpp = 3;
    unsigned char* row_data = (unsigned char*)malloc((size_t)img->width * bpp);
    for (unsigned int y = 0; y < img->height; ++y) {
        unsigned char* row_ptr = raw + y * raw_row_bytes;
        memcpy(row_data, &img->data[(y * img->width) * bpp], (size_t)img->width * bpp);
        row_ptr[0] = 1;
        for (size_t i = 0; i < (size_t)img->width * bpp; ++i) {
            if (i < bpp) {
                row_ptr[i + 1] = row_data[i];
            } else {
                row_ptr[i + 1] = row_data[i] - row_data[i - bpp];
            }
        }
    }
    free(row_data);

    unsigned char zlib_header[2] = {0x78, 0x01};

    size_t max_blocks = (raw_size + 65534) / 65535;
    size_t comp_cap = 2 + raw_size + max_blocks * 5 + 4;
    unsigned char* comp = (unsigned char*)malloc(comp_cap);
    if (!comp) { free(raw); fclose(f); return -1; }

    size_t comp_pos = 0;
    comp[comp_pos++] = zlib_header[0];
    comp[comp_pos++] = zlib_header[1];

    size_t remaining = raw_size;
    size_t raw_pos = 0;
    while (remaining > 0) {
        uint16_t block_len = (uint16_t)((remaining > 65535) ? 65535 : remaining);
        uint8_t bfinal = (remaining <= 65535) ? 1 : 0;
        uint8_t block_hdr = (uint8_t)((bfinal & 1) | (0 << 1));
        comp[comp_pos++] = block_hdr;
        comp[comp_pos++] = (uint8_t)(block_len & 0xFF);
        comp[comp_pos++] = (uint8_t)((block_len >> 8) & 0xFF);
        uint16_t nlen = (uint16_t)(~block_len);
        comp[comp_pos++] = (uint8_t)(nlen & 0xFF);
        comp[comp_pos++] = (uint8_t)((nlen >> 8) & 0xFF);
        memcpy(comp + comp_pos, raw + raw_pos, block_len);
        comp_pos += block_len;
        raw_pos += block_len;
        remaining -= block_len;
    }

    uint32_t a32 = adler32(raw, raw_size);
    comp[comp_pos++] = (unsigned char)((a32 >> 24) & 0xFF);
    comp[comp_pos++] = (unsigned char)((a32 >> 16) & 0xFF);
    comp[comp_pos++] = (unsigned char)((a32 >> 8) & 0xFF);
    comp[comp_pos++] = (unsigned char)(a32 & 0xFF);

    free(raw);

    uint32_t comp_len = (uint32_t)comp_pos;
    write_u32_be(f, comp_len);
    fwrite("IDAT", 1, 4, f);
    fwrite(comp, 1, comp_len, f);
    {
        unsigned char* tmp = (unsigned char*)malloc(4 + comp_len);
        memcpy(tmp, "IDAT", 4);
        memcpy(tmp + 4, comp, comp_len);
        uint32_t crc = crc32(tmp, 4 + comp_len);
        write_u32_be(f, crc);
        free(tmp);
    }
    free(comp);

    write_u32_be(f, 0);
    fwrite("IEND", 1, 4, f);
    uint32_t iend_crc = crc32((const unsigned char*)"IEND", 4);
    write_u32_be(f, iend_crc);

    fclose(f);
    return 0;
}



#endif