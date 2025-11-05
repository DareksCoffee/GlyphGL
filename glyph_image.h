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

/*
 * Image Processing Module for GlyphGL
 *
 * This module provides basic image handling functionality for the glyph atlas system,
 * including image creation, memory management, and export capabilities to common
 * image formats (PNG, BMP). It also includes checksum algorithms for data integrity
 * verification used in PNG compression.
 *
 * Key features:
 * - Simple RGB image structure and memory management
 * - PNG export with DEFLATE compression
 * - BMP export for uncompressed bitmaps
 * - CRC32 and Adler32 checksum calculations
 * - Cross-platform compatibility
 */

#ifndef __GLYPH_IMAGE_h
#define __GLYPH_IMAGE_h

#include "glyph_util.h"

/* Cross-platform standard library includes */
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

/*
 * Basic RGB image structure
 *
 * Represents an image in memory with RGB color data stored as 3 bytes per pixel
 * in row-major order (top to bottom). Used for glyph atlas textures and export.
 */
typedef struct {
    unsigned int width;      /* Image width in pixels */
    unsigned int height;     /* Image height in pixels */
    unsigned char* data;     /* RGB pixel data: width * height * 3 bytes */
} glyph_image_t;

/*
 * Creates a new RGB image with the specified dimensions
 *
 * Allocates memory for an image structure containing RGB pixel data.
 * Each pixel uses 3 bytes (red, green, blue) stored in row-major order
 * from top to bottom. The caller is responsible for freeing the image
 * with glyph_image_free().
 *
 * Parameters:
 *   width: Image width in pixels
 *   height: Image height in pixels
 *
 * Returns: glyph_image_t structure with allocated pixel buffer
 */
static glyph_image_t glyph_image_create(unsigned int width, unsigned int height) {
    glyph_image_t img;
    img.width = width;
    img.height = height;
    /* Allocate RGB pixel buffer: 3 bytes per pixel */
    img.data = (unsigned char*)GLYPH_MALLOC((size_t)width * height * 3);
    return img;
}

/*
 * Frees memory associated with a glyph image
 *
 * Deallocates the pixel data buffer and clears the data pointer.
 * Safe to call on NULL or already-freed images.
 *
 * Parameters:
 *   img: Pointer to glyph_image_t to free
 */
static void glyph_image_free(glyph_image_t* img) {
    if (!img) return;
    GLYPH_FREE(img->data);
    img->data = NULL;
}

/* CRC32 lookup table and initialization flag for PNG checksums */
static uint32_t crc32_table[256];
static int crc32_table_inited = 0;

/*
 * Initializes the CRC32 lookup table for efficient checksum calculation
 *
 * Creates a precomputed table using the IEEE 802.3 polynomial (0xEDB88320).
 * This table-based approach is much faster than computing CRC bit-by-bit.
 * The table is computed only once and reused for all CRC calculations.
 */
static void crc32_init_table(void) {
    if (crc32_table_inited) return; /* Already initialized */
    uint32_t poly = 0xEDB88320u;    /* IEEE 802.3 CRC32 polynomial */
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        /* Process each bit of the byte */
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) crc = (crc >> 1) ^ poly;
            else crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_inited = 1;
}

/*
 * Computes CRC32 checksum of data using the precomputed lookup table
 *
 * CRC32 provides a digital fingerprint of data that changes dramatically
 * with even single-bit modifications. Used in PNG format for data integrity
 * verification and in networking protocols for error detection.
 *
 * Parameters:
 *   data: Input data buffer
 *   len: Length of data in bytes
 *
 * Returns: 32-bit CRC32 checksum
 */
static uint32_t crc32(const unsigned char* data, size_t len) {
    crc32_init_table();           /* Ensure table is ready */
    uint32_t crc = 0xFFFFFFFFu;   /* Standard initial value */
    for (size_t i = 0; i < len; ++i) {
        uint8_t idx = (uint8_t)(crc ^ data[i]);
        crc = (crc >> 8) ^ crc32_table[idx];
    }
    return crc ^ 0xFFFFFFFFu;     /* Final XOR with all 1s */
}

/*
 * Computes Adler32 checksum - a fast alternative to CRC32
 *
 * Adler32 is a simple, fast checksum algorithm that maintains two 16-bit
 * sums. It's not as robust as CRC32 for error detection but is faster
 * and used in zlib compression for the DEFLATE compressed data checksum.
 *
 * Parameters:
 *   data: Input data buffer
 *   len: Length of data in bytes
 *
 * Returns: 32-bit Adler32 checksum (b << 16 | a)
 */
static uint32_t adler32(const unsigned char* data, size_t len) {
    const uint32_t MOD_ADLER = 65521u; /* Adler32 modulus */
    uint32_t a = 1;  /* Primary sum */
    uint32_t b = 0;  /* Secondary sum */
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a; /* Combine sums into 32-bit value */
}

/*
 * Writes a 32-bit unsigned integer to file in big-endian byte order
 *
 * Big-endian (network byte order) stores the most significant byte first.
 * This is required for PNG format compatibility and ensures cross-platform
 * data consistency regardless of the host system's endianness.
 *
 * Parameters:
 *   f: Open file handle to write to
 *   v: 32-bit value to write
 */
static void write_u32_be(FILE* f, uint32_t v) {
    unsigned char b[4];
    b[0] = (v >> 24) & 0xFF; /* Most significant byte */
    b[1] = (v >> 16) & 0xFF;
    b[2] = (v >> 8) & 0xFF;
    b[3] = v & 0xFF;         /* Least significant byte */
    fwrite(b, 1, 4, f);
}

/*
 * Writes a 32-bit unsigned integer to buffer in little-endian byte order
 *
 * Little-endian stores the least significant byte first, which matches
 * Intel x86/x64 processor architecture. Used for BMP file format headers.
 *
 * Parameters:
 *   buf: Buffer to write to (must have at least 4 bytes available)
 *   v: 32-bit value to write
 */
static void write_u32_le(unsigned char* buf, uint32_t v) {
    buf[0] = v & 0xFF;         /* Least significant byte */
    buf[1] = (v >> 8) & 0xFF;
    buf[2] = (v >> 16) & 0xFF;
    buf[3] = (v >> 24) & 0xFF; /* Most significant byte */
}
/*
 * Exports a glyph image to BMP (Bitmap) file format
 *
 * BMP is a simple, uncompressed image format supported by most image viewers.
 * This implementation creates a 24-bit RGB BMP with proper headers and row padding.
 * Pixels are written bottom-to-top (BMP convention) with BGR color order.
 *
 * Parameters:
 *   filename: Output BMP file path
 *   img: Pointer to glyph_image_t to export
 *
 * Returns: 0 on success, -1 on failure
 */
static int glyph_write_bmp(const char* filename, glyph_image_t* img) {
    /* Validate input parameters */
    if (!img || !img->data) return -1;

    /* Open file for binary writing */
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    /* Calculate BMP parameters */
    int width = (int)img->width;
    int height = (int)img->height;
    int row_size = ((width * 3 + 3) / 4) * 4; /* Row size must be multiple of 4 bytes */
    int data_size = row_size * height;
    int file_size = 54 + data_size; /* 54 bytes for headers */

    /* Write BMP file header (14 bytes) */
    unsigned char fileheader[14] = {
        'B','M',  /* BMP signature */
        (unsigned char)(file_size & 0xFF),
        (unsigned char)((file_size >> 8) & 0xFF),
        (unsigned char)((file_size >> 16) & 0xFF),
        (unsigned char)((file_size >> 24) & 0xFF),  /* File size */
        0,0,0,0,  /* Reserved */
        54,0,0,0 /* Data offset */
    };
    fwrite(fileheader, 1, 14, f);

    /* Write BMP info header (40 bytes) */
    unsigned char infoheader[40] = {0};
    infoheader[0] = 40;  /* Header size */
    /* Image dimensions */
    infoheader[4] = (unsigned char)(width & 0xFF);
    infoheader[5] = (unsigned char)((width >> 8) & 0xFF);
    infoheader[6] = (unsigned char)((width >> 16) & 0xFF);
    infoheader[7] = (unsigned char)((width >> 24) & 0xFF);
    infoheader[8] = (unsigned char)(height & 0xFF);
    infoheader[9] = (unsigned char)((height >> 8) & 0xFF);
    infoheader[10] = (unsigned char)((height >> 16) & 0xFF);
    infoheader[11] = (unsigned char)((height >> 24) & 0xFF);
    infoheader[12] = 1;    /* Planes */
    infoheader[14] = 24;   /* Bits per pixel */
    /* Image size */
    infoheader[20] = (unsigned char)(data_size & 0xFF);
    infoheader[21] = (unsigned char)((data_size >> 8) & 0xFF);
    infoheader[22] = (unsigned char)((data_size >> 16) & 0xFF);
    infoheader[23] = (unsigned char)((data_size >> 24) & 0xFF);
    fwrite(infoheader, 1, 40, f);

    /* Write pixel data bottom-to-top (BMP convention) with BGR color order */
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            /* Extract RGB components from image data */
            unsigned char r = img->data[(y * width + x) * 3 + 0];
            unsigned char g = img->data[(y * width + x) * 3 + 1];
            unsigned char b = img->data[(y * width + x) * 3 + 2];
            /* Write in BGR order (BMP format) */
            fputc(b, f);
            fputc(g, f);
            fputc(r, f);
        }
        /* Add row padding to align to 4-byte boundaries */
        int padding = (4 - (width * 3 % 4)) % 4;
        for (int p = 0; p < padding; p++) fputc(0, f);
    }
    fclose(f);
    return 0;
}
/*
 * Exports a glyph image to PNG (Portable Network Graphics) file format
 *
 * PNG is a compressed, lossless image format widely supported on the web and
 * in applications. This implementation creates a valid PNG with IHDR, IDAT,
 * and IEND chunks, using DEFLATE compression for the image data.
 *
 * Parameters:
 *   filename: Output PNG file path
 *   img: Pointer to glyph_image_t to export
 *
 * Returns: 0 on success, -1 on failure
 */
static int glyph_write_png(const char* filename, glyph_image_t* img) {
    /* Validate input parameters */
    if (!img || !img->data) return -1;

    /* Open file for binary writing */
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    /* Write PNG signature (required first 8 bytes of all PNG files) */
    const unsigned char png_sig[8] = {137,80,78,71,13,10,26,10};
    fwrite(png_sig, 1, 8, f);

    /* Create IHDR chunk data (Image Header) */
    unsigned char ihdr_data[13];
    /* Image width in big-endian */
    ihdr_data[0] = (img->width >> 24) & 0xFF;
    ihdr_data[1] = (img->width >> 16) & 0xFF;
    ihdr_data[2] = (img->width >> 8) & 0xFF;
    ihdr_data[3] = img->width & 0xFF;
    /* Image height in big-endian */
    ihdr_data[4] = (img->height >> 24) & 0xFF;
    ihdr_data[5] = (img->height >> 16) & 0xFF;
    ihdr_data[6] = (img->height >> 8) & 0xFF;
    ihdr_data[7] = img->height & 0xFF;
    ihdr_data[8] = 8;  /* Bit depth: 8 bits per channel */
    ihdr_data[9] = 2;  /* Color type: RGB (2) */
    ihdr_data[10] = 0; /* Compression method: DEFLATE (0) */
    ihdr_data[11] = 0; /* Filter method: Adaptive (0) */
    ihdr_data[12] = 0; /* Interlace method: None (0) */

    /* Write IHDR chunk */
    write_u32_be(f, 13);              /* Chunk length */
    fwrite("IHDR", 1, 4, f);         /* Chunk type */
    fwrite(ihdr_data, 1, 13, f);     /* Chunk data */
    /* Calculate and write CRC32 checksum */
    {
        unsigned char* tmp = (unsigned char*)GLYPH_MALLOC(4 + 13);
        memcpy(tmp, "IHDR", 4);
        memcpy(tmp + 4, ihdr_data, 13);
        uint32_t crc = crc32(tmp, 4 + 13);
        write_u32_be(f, crc);
        GLYPH_FREE(tmp);
    }

    /* Prepare image data for DEFLATE compression */
    /* PNG uses "filter bytes" and requires big-endian storage */
    size_t raw_row_bytes = (size_t)img->width * 3 + 1; /* +1 for filter byte */
    size_t raw_size = raw_row_bytes * img->height;
    unsigned char* raw = (unsigned char*)GLYPH_MALLOC(raw_size);
    if (!raw) { fclose(f); return -1; }

    int bpp = 3; /* Bytes per pixel (RGB) */
    /* Apply PNG filtering (Sub filter for simplicity) */
    unsigned char* row_data = (unsigned char*)GLYPH_MALLOC((size_t)img->width * bpp);
    for (unsigned int y = 0; y < img->height; ++y) {
        unsigned char* row_ptr = raw + y * raw_row_bytes;
        /* Copy current row */
        memcpy(row_data, &img->data[(y * img->width) * bpp], (size_t)img->width * bpp);
        row_ptr[0] = 1; /* Filter type: Sub (1) */
        /* Apply Sub filter: each pixel = current - left neighbor */
        for (size_t i = 0; i < (size_t)img->width * bpp; ++i) {
            if (i < bpp) {
                row_ptr[i + 1] = row_data[i]; /* First pixel unchanged */
            } else {
                row_ptr[i + 1] = row_data[i] - row_data[i - bpp]; /* Subtract left pixel */
            }
        }
    }
    GLYPH_FREE(row_data);

    /* Compress using DEFLATE algorithm (stored/uncompressed for simplicity) */
    /* Zlib header for DEFLATE compression */
    unsigned char zlib_header[2] = {0x78, 0x01}; /* Fastest compression, stored blocks */

    /* Calculate maximum compressed size (worst case) */
    size_t max_blocks = (raw_size + 65534) / 65535; /* Number of 64KB blocks needed */
    size_t comp_cap = 2 + raw_size + max_blocks * 5 + 4; /* Header + data + block overhead + checksum */
    unsigned char* comp = (unsigned char*)GLYPH_MALLOC(comp_cap);
    if (!comp) { GLYPH_FREE(raw); fclose(f); return -1; }

    /* Build compressed data stream */
    size_t comp_pos = 0;
    comp[comp_pos++] = zlib_header[0]; /* Zlib compression method/flags */
    comp[comp_pos++] = zlib_header[1];

    /* Process data in 64KB blocks (DEFLATE stored blocks) */
    size_t remaining = raw_size;
    size_t raw_pos = 0;
    while (remaining > 0) {
        uint16_t block_len = (uint16_t)((remaining > 65535) ? 65535 : remaining);
        uint8_t bfinal = (remaining <= 65535) ? 1 : 0; /* Final block flag */
        uint8_t block_hdr = (uint8_t)((bfinal & 1) | (0 << 1)); /* Stored block type */
        comp[comp_pos++] = block_hdr;
        /* Block length and one's complement */
        comp[comp_pos++] = (uint8_t)(block_len & 0xFF);
        comp[comp_pos++] = (uint8_t)((block_len >> 8) & 0xFF);
        uint16_t nlen = (uint16_t)(~block_len);
        comp[comp_pos++] = (uint8_t)(nlen & 0xFF);
        comp[comp_pos++] = (uint8_t)((nlen >> 8) & 0xFF);
        /* Copy uncompressed data */
        memcpy(comp + comp_pos, raw + raw_pos, block_len);
        comp_pos += block_len;
        raw_pos += block_len;
        remaining -= block_len;
    }

    /* Append Adler32 checksum (required by zlib) */
    uint32_t a32 = adler32(raw, raw_size);
    comp[comp_pos++] = (unsigned char)((a32 >> 24) & 0xFF);
    comp[comp_pos++] = (unsigned char)((a32 >> 16) & 0xFF);
    comp[comp_pos++] = (unsigned char)((a32 >> 8) & 0xFF);
    comp[comp_pos++] = (unsigned char)(a32 & 0xFF);

    GLYPH_FREE(raw); /* Free uncompressed data */

    /* Write IDAT chunk (Image Data) */
    uint32_t comp_len = (uint32_t)comp_pos;
    write_u32_be(f, comp_len);        /* Chunk length */
    fwrite("IDAT", 1, 4, f);         /* Chunk type */
    fwrite(comp, 1, comp_len, f);    /* Compressed data */
    /* Calculate and write CRC32 */
    {
        unsigned char* tmp = (unsigned char*)GLYPH_MALLOC(4 + comp_len);
        memcpy(tmp, "IDAT", 4);
        memcpy(tmp + 4, comp, comp_len);
        uint32_t crc = crc32(tmp, 4 + comp_len);
        write_u32_be(f, crc);
        GLYPH_FREE(tmp);
    }
    GLYPH_FREE(comp); /* Free compressed data */

    /* Write IEND chunk (Image End) - marks end of PNG file */
    write_u32_be(f, 0);                          /* Empty chunk */
    fwrite("IEND", 1, 4, f);                     /* Chunk type */
    uint32_t iend_crc = crc32((const unsigned char*)"IEND", 4);
    write_u32_be(f, iend_crc);                   /* CRC32 checksum */

    fclose(f);
    return 0;
}

#endif