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
 * Glyph Atlas Module - Font Texture Generation and Management
 *
 * This module handles the core font processing pipeline for GlyphGL:
 * - Loads TrueType (.ttf) fonts
 * - Rasterizes individual glyphs into bitmaps
 * - Packs glyphs efficiently into a texture atlas
 * - Provides SDF (Signed Distance Field) rendering support
 * - Manages character set encoding (ASCII/UTF-8)
 *
 * The atlas system is crucial for performance, allowing multiple characters
 * to be rendered from a single texture with minimal texture state changes.
 */

#ifndef __GLYPH_ATLAS_H
#define __GLYPH_ATLAS_H

#include "glyph_image.h"
#include "glyph_util.h"
#include "glyph_truetype.h"

/* Character encoding type flags */
typedef enum {
    GLYPH_ENCODING_NONE  = 0,        /* No special encoding */
    GLYPH_ENCODING_UTF8  = 0x010,    /* UTF-8 multi-byte character support */
    GLYPH_ENCODING_ASCII = 0x020,    /* Simple ASCII single-byte encoding */
} glyph_encoding_type_t;

/* Default atlas texture dimensions - can be overridden at compile time */
#define GLYPHGL_ATLAS_WIDTH 2048   /* Default atlas width in pixels */
#define GLYPHGL_ATLAS_HEIGHT 2048  /* Default atlas height in pixels */

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
 * Internal UTF-8 decoder for character set processing
 *
 * Decodes UTF-8 sequences during atlas creation to handle multi-byte characters.
 * This is a simplified version focused on charset parsing rather than full validation.
 *
 * Parameters:
 *   str: UTF-8 encoded string
 *   index: Current position in string (updated to next character)
 *
 * Returns: Unicode codepoint or 0xFFFD (replacement character) on error
 */
static int glyph_atlas_utf8_decode(const char* str, size_t* index) {
    size_t i = *index;
    unsigned char c = (unsigned char)str[i++];
    /* 1-byte ASCII character (most common case) */
    if (c < 0x80) {
        *index = i;
        return c;
    }
    /* 2-byte sequence (Latin-1 supplement, etc.) */
    else if ((c & 0xE0) == 0xC0) {
        unsigned char c2 = (unsigned char)str[i++];
        *index = i;
        return ((c & 0x1F) << 6) | (c2 & 0x3F); /* Decode 11-bit codepoint */
    }
    /* 3-byte sequence (Basic Multilingual Plane) */
    else if ((c & 0xF0) == 0xE0) {
        unsigned char c2 = (unsigned char)str[i++];
        unsigned char c3 = (unsigned char)str[i++];
        *index = i;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F); /* Decode 16-bit codepoint */
    }
    /* 4-byte sequence (astral planes, emojis) */
    else if ((c & 0xF8) == 0xF0) {
        unsigned char c2 = (unsigned char)str[i++];
        unsigned char c3 = (unsigned char)str[i++];
        unsigned char c4 = (unsigned char)str[i++];
        *index = i;
        return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F); /* Decode 21-bit codepoint */
    }
    /* Invalid UTF-8 lead byte */
    *index = i;
    return 0xFFFD; /* Unicode replacement character */
}

/*
 * Individual character data stored in the atlas
 *
 * Contains all the geometric and positioning information needed to render
 * a single glyph, including its location in the atlas texture and metrics.
 */
typedef struct {
    int codepoint;     /* Unicode codepoint this glyph represents */
    int x, y;          /* Position in atlas texture (top-left corner) */
    int width, height; /* Glyph bitmap dimensions in pixels */
    int xoff, yoff;    /* Offset from baseline to glyph origin (left-bearing, descent) */
    int advance;       /* Horizontal advance width for cursor positioning */
} glyph_atlas_char_t;

/*
 * Font atlas containing pre-rasterized glyphs packed into a texture
 *
 * This structure represents a complete font atlas with all character data
 * and the packed texture image. The atlas is the core data structure that
 * enables efficient text rendering by storing all glyphs in a single texture.
 */
typedef struct {
    glyph_image_t image;        /* RGB texture image containing packed glyphs */
    glyph_atlas_char_t* chars;  /* Array of character data (one per glyph) */
    int num_chars;              /* Number of characters in the atlas */
    float pixel_height;         /* Font size used for rasterization */
} glyph_atlas_t;

/*
 * Calculates the next power-of-2 value greater than or equal to input
 *
 * Used for determining optimal atlas texture dimensions, as OpenGL
 * historically preferred power-of-2 textures for better performance.
 * Uses bit manipulation for efficient calculation.
 *
 * Parameters:
 *   v: Input value
 *
 * Returns: Smallest power-of-2 >= v
 */
static int glyph_atlas__next_pow2(int v) {
    v--;                    /* Decrement to handle exact powers of 2 */
    v |= v >> 1;           /* Fill in bits to the right */
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;                   /* Increment to get next power of 2 */
    return v;
}


/*
 * Creates a font atlas by rasterizing and packing glyphs into a texture
 *
 * This is the core atlas generation function that:
 * 1. Loads the font file (TTF)
 * 2. Rasterizes each character in the charset
 * 3. Optionally converts to SDF for scalable rendering
 * 4. Packs glyphs efficiently into a 2D texture atlas
 * 5. Returns complete atlas with positioning data
 *
 * The packing algorithm sorts glyphs by height and uses a row-based approach
 * to minimize wasted texture space while maintaining efficient access patterns.
 *
 * Parameters:
 *   font_path: Path to .ttf font file
 *   pixel_height: Font size for rasterization (affects quality/detail)
 *   charset: String containing all characters to include
 *   char_type: GLYPH_ENCODING_UTF8 or GLYPH_ENCODING_ASCII encoding type
 *   use_sdf: Enable Signed Distance Field rendering (smoother scaling)
 *
 * Returns: Complete glyph_atlas_t or zero-initialized struct on failure
 */
static inline glyph_atlas_t glyph_atlas_create(const char* font_path, float pixel_height, const char* charset, glyph_encoding_type_t char_type, int use_sdf) {
    /* Initialize atlas structure */
    glyph_atlas_t atlas = {0};

    /* Font structure */
    glyph_font_t ttf_font;
    float scale; /* Font units to pixel conversion factor */

    /* Load TrueType font */
    if (!glyph_ttf_load_font_from_file(&ttf_font, font_path)) {
        GLYPH_LOG("Failed to load TTF font: %s\n", font_path);
        return atlas;
    }
    scale = glyph_ttf_scale_for_pixel_height(&ttf_font, pixel_height);
    
    /* Store the pixel height for reference */
    atlas.pixel_height = pixel_height;

    /* Use default ASCII charset if none provided */
    if (!charset) {
        if (char_type == GLYPH_ENCODING_UTF8) {
            charset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        } else {
            charset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        }
    }

    /* Calculate number of characters in charset (handles UTF-8 multi-byte) */
    int charset_len;
    if (char_type == GLYPH_ENCODING_UTF8) {
        charset_len = 0;
        size_t idx = 0;
        /* Count UTF-8 codepoints by decoding each sequence */
        while (idx < strlen(charset)) {
            glyph_atlas_utf8_decode(charset, &idx);
            charset_len++;
        }
    } else {
        /* Simple byte count for ASCII */
        charset_len = strlen(charset);
    }

    /* Allocate character data array */
    atlas.num_chars = charset_len;
    atlas.chars = (glyph_atlas_char_t*)GLYPH_MALLOC(charset_len * sizeof(glyph_atlas_char_t));
    if (!atlas.chars) {
        /* Cleanup on allocation failure */
        glyph_ttf_free_font(&ttf_font);
        return atlas;
    }

    /* Temporary structure to hold glyph bitmaps during processing */
    typedef struct {
        unsigned char* bitmap;  /* Rasterized glyph bitmap data */
        int width, height;      /* Bitmap dimensions */
        int xoff, yoff;         /* Baseline offsets */
        int advance;            /* Cursor advance width */
        int is_default;         /* Flag for SDF-generated bitmaps */
    } temp_glyph_t;

    /* Allocate temporary glyph storage */
    temp_glyph_t* temp_glyphs = (temp_glyph_t*)GLYPH_MALLOC(charset_len * sizeof(temp_glyph_t));
    if (!temp_glyphs) {
        /* Cleanup on allocation failure */
        GLYPH_FREE(atlas.chars);
        atlas.chars = NULL;
        glyph_ttf_free_font(&ttf_font);
        return atlas;
    }

    /* Phase 1: Rasterize all glyphs and calculate atlas requirements */
    int total_width = 0;  /* Estimate total width needed for all glyphs */
    int max_height = 0;   /* Track maximum glyph height */

    size_t charset_idx = 0; /* Index for UTF-8 charset traversal */
    for (int i = 0; i < charset_len; i++) {
        /* Decode next character from charset */
        int codepoint;
        if (char_type == GLYPH_ENCODING_UTF8) {
            codepoint = glyph_atlas_utf8_decode(charset, &charset_idx);
        } else {
            codepoint = (unsigned char)charset[i];
        }

        /* Find glyph index in font (maps codepoint to glyph) */
        int glyph_idx = glyph_ttf_find_glyph_index(&ttf_font, codepoint);

        /* Handle missing glyphs (glyph_idx == 0 means .notdef glyph) */
        if (glyph_idx == 0 && codepoint != ' ') {
            /* Create fallback data for missing characters */
            temp_glyphs[i].bitmap = NULL;
            temp_glyphs[i].width = 0;
            temp_glyphs[i].height = 0;
            temp_glyphs[i].xoff = 0;
            temp_glyphs[i].yoff = 0;
            temp_glyphs[i].advance = (int)(pixel_height * 0.5f); /* Half-width fallback */
            temp_glyphs[i].is_default = 0;
            atlas.chars[i].codepoint = codepoint;
            continue;
        }

        /* Rasterize the glyph into a bitmap */
        int width, height, xoff, yoff;
        unsigned char* bitmap;

        /* Get glyph bitmap from TrueType font */
        bitmap = glyph_ttf_get_glyph_bitmap(&ttf_font, glyph_idx, scale, scale,
                                             &width, &height, &xoff, &yoff);

        /* Convert to Signed Distance Field if requested */
        if (use_sdf && bitmap) {
            /* Generate SDF bitmap for smooth scaling */
            unsigned char* sdf = glyph_ttf_get_glyph_sdf_bitmap(bitmap, width, height, 4);
            /* Free original bitmap */
            glyph_ttf_free_bitmap(bitmap);
            bitmap = sdf; /* Use SDF bitmap instead */
            temp_glyphs[i].is_default = 1; /* Mark as SDF-generated */
        }

        /* Store glyph data in temporary structure */
        temp_glyphs[i].bitmap = bitmap;
        temp_glyphs[i].width = width;
        temp_glyphs[i].height = height;
        temp_glyphs[i].xoff = xoff;
        temp_glyphs[i].yoff = yoff;

        /* Get horizontal advance width */
        temp_glyphs[i].advance = (int)(glyph_ttf_get_glyph_advance(&ttf_font, glyph_idx) * scale);
        temp_glyphs[i].is_default = use_sdf ? 1 : 0;

        /* Store basic character info */
        atlas.chars[i].codepoint = codepoint;
        atlas.chars[i].advance = temp_glyphs[i].advance;

        /* Accumulate atlas size requirements */
        total_width += width + 4; /* Add padding between glyphs */
        if (height > max_height) {
            max_height = height;
        }
    }

    /* Phase 2: Sort glyphs by height for optimal packing */
    /* Sort glyphs tallest-first to minimize wasted vertical space */
    int* glyph_order = (int*)GLYPH_MALLOC(charset_len * sizeof(int));
    if (!glyph_order) {
        /* Cleanup on allocation failure */
        GLYPH_FREE(atlas.chars);
        atlas.chars = NULL;
        GLYPH_FREE(temp_glyphs);
        glyph_ttf_free_font(&ttf_font);
        return atlas;
    }

    /* Initialize sort indices */
    for (int i = 0; i < charset_len; i++) {
        glyph_order[i] = i;
    }

    /* Bubble sort by height (descending) - simple but effective for glyph counts */
    for (int i = 0; i < charset_len - 1; i++) {
        for (int j = 0; j < charset_len - i - 1; j++) {
            if (temp_glyphs[glyph_order[j]].height < temp_glyphs[glyph_order[j + 1]].height) {
                /* Swap indices to sort by decreasing height */
                int temp = glyph_order[j];
                glyph_order[j] = glyph_order[j + 1];
                glyph_order[j + 1] = temp;
            }
        }
    }
    
    /* Phase 3: Create atlas texture and pack glyphs */
    int padding = 4; /* Pixels between glyphs to prevent bleeding */

    /* Estimate atlas size using square root of total area, round up to power-of-2 */
    int atlas_width = glyph_atlas__next_pow2((int)sqrtf(total_width * max_height) + 256);
    int atlas_height = atlas_width;

    /* Ensure minimum atlas size */
    if (atlas_width < GLYPHGL_ATLAS_WIDTH) atlas_width = GLYPHGL_ATLAS_WIDTH;
    if (atlas_height < GLYPHGL_ATLAS_HEIGHT) atlas_height = GLYPHGL_ATLAS_HEIGHT;

    /* Create RGB image for atlas texture */
    atlas.image = glyph_image_create(atlas_width, atlas_height);
    memset(atlas.image.data, 0, atlas_width * atlas_height * 3); /* Clear to black */
    
    /* Initialize packing cursor and row tracking */
    int pen_x = padding;     /* Current X position in atlas */
    int pen_y = padding;     /* Current Y position in atlas */
    int row_height = 0;      /* Height of current row */

    /* Structure to track glyphs in current packing row */
    typedef struct {
        int index;  /* Index into temp_glyphs array */
        int yoff;   /* Y offset of this glyph */
    } row_glyph_t;

    /* Allocate space for tracking current row */
    row_glyph_t* current_row = (row_glyph_t*)GLYPH_MALLOC(charset_len * sizeof(row_glyph_t));
    int row_count = 0; /* Number of glyphs in current row */

    /* Pack glyphs into atlas using sorted order */
    for (int order_idx = 0; order_idx < charset_len; order_idx++) {
        int i = glyph_order[order_idx];

        /* Skip glyphs with no bitmap data */
        if (!temp_glyphs[i].bitmap || temp_glyphs[i].width == 0) {
            /* Set zero data for empty glyphs */
            atlas.chars[i].x = 0;
            atlas.chars[i].y = 0;
            atlas.chars[i].width = 0;
            atlas.chars[i].height = 0;
            atlas.chars[i].xoff = 0;
            atlas.chars[i].yoff = 0;
            continue;
        }

        /* Check if glyph fits in current row, if not, finalize current row */
        if (pen_x + temp_glyphs[i].width + padding > atlas_width) {
            /* Calculate baseline alignment for current row */
            int max_yoff = 0;
            for (int r = 0; r < row_count; r++) {
                if (temp_glyphs[current_row[r].index].yoff > max_yoff) {
                    max_yoff = temp_glyphs[current_row[r].index].yoff;
                }
            }

            /* Blit all glyphs in current row to atlas */
            for (int r = 0; r < row_count; r++) {
                int idx = current_row[r].index;
                /* Align glyph to baseline */
                int glyph_top = max_yoff - temp_glyphs[idx].yoff;

                /* Copy glyph bitmap to atlas texture */
                for (int y = 0; y < temp_glyphs[idx].height; y++) {
                    for (int x = 0; x < temp_glyphs[idx].width; x++) {
                        int atlas_x = atlas.chars[idx].x + x;
                        int atlas_y = atlas.chars[idx].y + glyph_top + y;
                        unsigned char alpha = temp_glyphs[idx].bitmap[y * temp_glyphs[idx].width + x];

                        /* Write grayscale alpha to RGB channels */
                        int pixel_idx = (atlas_y * atlas_width + atlas_x) * 3;
                        atlas.image.data[pixel_idx + 0] = alpha;
                        atlas.image.data[pixel_idx + 1] = alpha;
                        atlas.image.data[pixel_idx + 2] = alpha;
                    }
                }

                /* Update final Y position after baseline alignment */
                atlas.chars[idx].y = atlas.chars[idx].y + glyph_top;
            }

            /* Start new row */
            pen_x = padding;
            pen_y += row_height + padding;
            row_height = 0;
            row_count = 0;
        }
        
        /* Check if glyph fits vertically in atlas, resize if needed */
        if (pen_y + temp_glyphs[i].height + padding > atlas_height) {
            GLYPH_LOG("Warning: Atlas too small, increasing size for glyph %d\n", i);
            /* Double atlas size and restart packing */
            atlas_width *= 2;
            atlas_height *= 2;
            glyph_image_free(&atlas.image);
            atlas.image = glyph_image_create(atlas_width, atlas_height);
            memset(atlas.image.data, 0, atlas_width * atlas_height * 3);
            /* Reset packing state */
            pen_x = padding;
            pen_y = padding;
            row_height = 0;
            row_count = 0;
            order_idx = -1; /* Restart the packing loop */
            continue;
        }

        /* Add glyph to current row */
        current_row[row_count].index = i;
        current_row[row_count].yoff = temp_glyphs[i].yoff;
        row_count++;

        /* Record glyph position in atlas */
        atlas.chars[i].x = pen_x;
        atlas.chars[i].y = pen_y;
        atlas.chars[i].width = temp_glyphs[i].width;
        atlas.chars[i].height = temp_glyphs[i].height;
        atlas.chars[i].xoff = temp_glyphs[i].xoff;
        atlas.chars[i].yoff = temp_glyphs[i].yoff;

        /* Update row height to accommodate tallest glyph */
        int bottom = temp_glyphs[i].height;
        row_height = (bottom > row_height) ? bottom : row_height;

        /* Advance cursor for next glyph */
        pen_x += temp_glyphs[i].width + padding * 2;
    }
    
    /* Finalize last row if it contains glyphs */
    if (row_count > 0) {
        /* Calculate baseline alignment for final row */
        int max_yoff = 0;
        for (int r = 0; r < row_count; r++) {
            if (temp_glyphs[current_row[r].index].yoff > max_yoff) {
                max_yoff = temp_glyphs[current_row[r].index].yoff;
            }
        }

        /* Blit final row glyphs to atlas */
        for (int r = 0; r < row_count; r++) {
            int idx = current_row[r].index;
            int glyph_top = max_yoff - temp_glyphs[idx].yoff;

            /* Copy glyph bitmap to final position */
            for (int y = 0; y < temp_glyphs[idx].height; y++) {
                for (int x = 0; x < temp_glyphs[idx].width; x++) {
                    int atlas_x = atlas.chars[idx].x + x;
                    int atlas_y = atlas.chars[idx].y + glyph_top + y;
                    unsigned char alpha = temp_glyphs[idx].bitmap[y * temp_glyphs[idx].width + x];

                    /* Write to RGB atlas */
                    int pixel_idx = (atlas_y * atlas_width + atlas_x) * 3;
                    atlas.image.data[pixel_idx + 0] = alpha;
                    atlas.image.data[pixel_idx + 1] = alpha;
                    atlas.image.data[pixel_idx + 2] = alpha;
                }
            }

            /* Update final Y position */
            atlas.chars[idx].y = atlas.chars[idx].y + glyph_top;
        }
    }
    
    /* Cleanup temporary resources */
    GLYPH_FREE(current_row);
    GLYPH_FREE(glyph_order);

    /* Free all glyph bitmaps */
    for (int i = 0; i < charset_len; i++) {
        if (temp_glyphs[i].bitmap) {
            if (temp_glyphs[i].is_default) {
                /* SDF bitmaps allocated with GLYPH_MALLOC */
                GLYPH_FREE(temp_glyphs[i].bitmap);
            } else {
                /* Regular bitmaps from font parsers */
                glyph_ttf_free_bitmap(temp_glyphs[i].bitmap);
            }
        }
    }
    GLYPH_FREE(temp_glyphs);

    /* Free font resources */
    glyph_ttf_free_font(&ttf_font);

    /* Return completed atlas */
    return atlas;
}

/*
 * Frees all resources associated with a glyph atlas
 *
 * Deallocates the character array and atlas image. Safe to call
 * on partially initialized or empty atlases.
 *
 * Parameters:
 *   atlas: Pointer to glyph_atlas_t to free
 */
static inline void glyph_atlas_free(glyph_atlas_t* atlas) {
    if (!atlas) return;
    /* Free character data array */
    if (atlas->chars) {
        GLYPH_FREE(atlas->chars);
        atlas->chars = NULL;
    }
    /* Free atlas texture image */
    glyph_image_free(&atlas->image);
    atlas->num_chars = 0;
}

/*
 * Saves the atlas texture as a PNG image file
 *
 * Useful for debugging atlas packing or using the atlas
 * in external tools/renderers.
 *
 * Parameters:
 *   atlas: Pointer to glyph atlas
 *   output_path: Path where PNG file will be written
 *
 * Returns: 0 on success, -1 on failure
 */
static inline int glyph_atlas_save_png(glyph_atlas_t* atlas, const char* output_path) {
    if (!atlas || !atlas->image.data) return -1;
    return glyph_write_png(output_path, &atlas->image);
}

/*
 * Saves the atlas texture as a BMP image file
 *
 * Alternative format for atlas export, may be useful
 * for certain toolchains or debugging.
 *
 * Parameters:
 *   atlas: Pointer to glyph atlas
 *   output_path: Path where BMP file will be written
 *
 * Returns: 0 on success, -1 on failure
 */
static inline int glyph_atlas_save_bmp(glyph_atlas_t* atlas, const char* output_path) {
    if (!atlas || !atlas->image.data) return -1;
    return glyph_write_bmp(output_path, &atlas->image);
}

/*
 * Saves glyph positioning metadata to a text file
 *
 * Exports all character metrics and atlas positions in a
 * human-readable format. Useful for debugging or external
 * font processing tools.
 *
 * Parameters:
 *   atlas: Pointer to glyph atlas
 *   output_path: Path where metadata file will be written
 *
 * Returns: 0 on success, -1 on failure
 */
static inline int glyph_atlas_save_metadata(glyph_atlas_t* atlas, const char* output_path) {
    if (!atlas || !atlas->chars) return -1;

    FILE* f = fopen(output_path, "w");
    if (!f) return -1;

    /* Write header information */
    fprintf(f, "# Font Atlas Metadata\n");
    fprintf(f, "pixel_height: %.2f\n", atlas->pixel_height);
    fprintf(f, "atlas_width: %u\n", atlas->image.width);
    fprintf(f, "atlas_height: %u\n", atlas->image.height);
    fprintf(f, "num_chars: %d\n\n", atlas->num_chars);
    fprintf(f, "# codepoint x y width height xoff yoff advance\n");

    /* Write per-character data */
    for (int i = 0; i < atlas->num_chars; i++) {
        glyph_atlas_char_t* c = &atlas->chars[i];
        fprintf(f, "%d %d %d %d %d %d %d %d\n",
                c->codepoint, c->x, c->y, c->width, c->height,
                c->xoff, c->yoff, c->advance);
    }

    fclose(f);
    return 0;
}

/*
 * Looks up character data by Unicode codepoint
 *
 * Searches the atlas for glyph information corresponding to a
 * specific Unicode character. Returns NULL if character not found.
 *
 * Parameters:
 *   atlas: Pointer to glyph atlas
 *   codepoint: Unicode codepoint to search for
 *
 * Returns: Pointer to glyph_atlas_char_t or NULL if not found
 */
static inline glyph_atlas_char_t* glyph_atlas_find_char(glyph_atlas_t* atlas, int codepoint) {
    if (!atlas || !atlas->chars) return NULL;

    /* Linear search through character array */
    for (int i = 0; i < atlas->num_chars; i++) {
        if (atlas->chars[i].codepoint == codepoint) {
            return &atlas->chars[i];
        }
    }
    return NULL;
}

/*
 * Prints detailed information about the atlas to the log
 *
 * Useful for debugging atlas generation and inspecting
 * character metrics and positioning.
 *
 * Parameters:
 *   atlas: Pointer to glyph atlas to inspect
 */
static inline void glyph_atlas_print_info(glyph_atlas_t* atlas) {
    if (!atlas) return;

    /* Print atlas overview */
    GLYPH_LOG("Font Atlas Info:\n");
    GLYPH_LOG("  Atlas Size: %ux%u\n", atlas->image.width, atlas->image.height);
    GLYPH_LOG("  Pixel Height: %.2f\n", atlas->pixel_height);
    GLYPH_LOG("  Characters: %d\n", atlas->num_chars);
    GLYPH_LOG("\nCharacter Details:\n");

    /* Print per-character details */
    for (int i = 0; i < atlas->num_chars; i++) {
        glyph_atlas_char_t* c = &atlas->chars[i];
        /* Display printable ASCII chars, '?' for others */
        char ch = (c->codepoint >= 32 && c->codepoint < 127) ? c->codepoint : '?';
        GLYPH_LOG("  '%c' (U+%04X): pos=(%d,%d) size=(%dx%d) offset=(%d,%d) advance=%d\n",
                ch, c->codepoint, c->x, c->y, c->width, c->height,
                c->xoff, c->yoff, c->advance);
    }
}

#endif