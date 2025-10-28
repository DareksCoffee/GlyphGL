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

#ifndef __GLYPH_ATLAS_H
#define __GLYPH_ATLAS_H

#include "glyph_image.h"

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
    int codepoint;
    int x, y;
    int width, height;
    int xoff, yoff;
    int advance;
} glyph_atlas_char_t;

typedef struct {
    glyph_image_t image;
    glyph_atlas_char_t* chars;
    int num_chars;
    float pixel_height;
} glyph_atlas_t;

static int glyph_atlas__next_pow2(int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

glyph_atlas_t glyph_atlas_create(const char* font_path, float pixel_height, const char* charset) {
    glyph_atlas_t atlas = {0};
    glyph_font_t font;
    
    if (!glyph_ttf_load_font_from_file(&font, font_path)) {
        printf("Failed to load font: %s\n", font_path);
        return atlas;
    }
    
    float scale = glyph_ttf_scale_for_pixel_height(&font, pixel_height);
    atlas.pixel_height = pixel_height;
    
    if (!charset) {
        charset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    }
    
    int charset_len = strlen(charset);
    atlas.num_chars = charset_len;
    atlas.chars = (glyph_atlas_char_t*)malloc(charset_len * sizeof(glyph_atlas_char_t));
    if (!atlas.chars) {
        glyph_ttf_free_font(&font);
        return atlas;
    }
    
    typedef struct {
        unsigned char* bitmap;
        int width, height;
        int xoff, yoff;
        int advance;
    } temp_glyph_t;
    
    temp_glyph_t* temp_glyphs = (temp_glyph_t*)malloc(charset_len * sizeof(temp_glyph_t));
    if (!temp_glyphs) {
        free(atlas.chars);
        atlas.chars = NULL;
        glyph_ttf_free_font(&font);
        return atlas;
    }
    
    int total_width = 0;
    int max_height = 0;
    
    for (int i = 0; i < charset_len; i++) {
        int codepoint = (unsigned char)charset[i];
        int glyph_idx = glyph_ttf_find_glyph_index(&font, codepoint);
        
        if (glyph_idx == 0 && codepoint != ' ') {
            temp_glyphs[i].bitmap = NULL;
            temp_glyphs[i].width = 0;
            temp_glyphs[i].height = 0;
            temp_glyphs[i].xoff = 0;
            temp_glyphs[i].yoff = 0;
            temp_glyphs[i].advance = (int)(pixel_height * 0.5f);
            atlas.chars[i].codepoint = codepoint;
            continue;
        }
        
        int width, height, xoff, yoff;
        unsigned char* bitmap = glyph_ttf_get_glyph_bitmap(&font, glyph_idx, scale, scale,
                                                            &width, &height, &xoff, &yoff);
        
        temp_glyphs[i].bitmap = bitmap;
        temp_glyphs[i].width = width;
        temp_glyphs[i].height = height;
        temp_glyphs[i].xoff = xoff;
        temp_glyphs[i].yoff = yoff;
        temp_glyphs[i].advance = (int)(glyph_ttf_get_glyph_advance(&font, glyph_idx) * scale);
        
        atlas.chars[i].codepoint = codepoint;
        atlas.chars[i].advance = temp_glyphs[i].advance;
        
        total_width += width + 4;
        if (height > max_height) {
            max_height = height;
        }
    }
    
    int padding = 4;
    int atlas_width = glyph_atlas__next_pow2((int)sqrtf(total_width * max_height) + 256);
    int atlas_height = atlas_width;
    
    atlas.image = glyph_image_create(atlas_width, atlas_height);
    memset(atlas.image.data, 0, atlas_width * atlas_height * 3);
    
    int pen_x = padding;
    int pen_y = padding;
    int row_height = 0;
    
    typedef struct {
        int index;
        int yoff;
    } row_glyph_t;
    
    row_glyph_t* current_row = (row_glyph_t*)malloc(charset_len * sizeof(row_glyph_t));
    int row_count = 0;
    
    for (int i = 0; i < charset_len; i++) {
        if (!temp_glyphs[i].bitmap || temp_glyphs[i].width == 0) {
            atlas.chars[i].x = 0;
            atlas.chars[i].y = 0;
            atlas.chars[i].width = 0;
            atlas.chars[i].height = 0;
            atlas.chars[i].xoff = 0;
            atlas.chars[i].yoff = 0;
            continue;
        }
        
        if (pen_x + temp_glyphs[i].width + padding > atlas_width) {
            int max_yoff = 0;
            for (int r = 0; r < row_count; r++) {
                if (temp_glyphs[current_row[r].index].yoff > max_yoff) {
                    max_yoff = temp_glyphs[current_row[r].index].yoff;
                }
            }
            
            for (int r = 0; r < row_count; r++) {
                int idx = current_row[r].index;
                int glyph_top = max_yoff - temp_glyphs[idx].yoff;
                
                for (int y = 0; y < temp_glyphs[idx].height; y++) {
                    for (int x = 0; x < temp_glyphs[idx].width; x++) {
                        int atlas_x = atlas.chars[idx].x + x;
                        int atlas_y = atlas.chars[idx].y + glyph_top + y;
                        unsigned char alpha = temp_glyphs[idx].bitmap[y * temp_glyphs[idx].width + x];

                        int pixel_idx = (atlas_y * atlas_width + atlas_x) * 3;
                        atlas.image.data[pixel_idx + 0] = alpha;
                        atlas.image.data[pixel_idx + 1] = alpha;
                        atlas.image.data[pixel_idx + 2] = alpha;
                    }
                }
                
                atlas.chars[idx].y = atlas.chars[idx].y + glyph_top;
            }
            
            pen_x = padding;
            pen_y += row_height + padding;
            row_height = 0;
            row_count = 0;
        }
        
        if (pen_y + temp_glyphs[i].height + padding > atlas_height) {
            printf("Warning: Atlas too small, some glyphs may be cut off\n");
            break;
        }
        
        current_row[row_count].index = i;
        current_row[row_count].yoff = temp_glyphs[i].yoff;
        row_count++;
        
        atlas.chars[i].x = pen_x;
        atlas.chars[i].y = pen_y;
        atlas.chars[i].width = temp_glyphs[i].width;
        atlas.chars[i].height = temp_glyphs[i].height;
        atlas.chars[i].xoff = temp_glyphs[i].xoff;
        atlas.chars[i].yoff = temp_glyphs[i].yoff;
        
        int bottom = temp_glyphs[i].height;
        row_height = (bottom > row_height) ? bottom : row_height;
        
        pen_x += temp_glyphs[i].width + padding * 2;
    }
    
    if (row_count > 0) {
        int max_yoff = 0;
        for (int r = 0; r < row_count; r++) {
            if (temp_glyphs[current_row[r].index].yoff > max_yoff) {
                max_yoff = temp_glyphs[current_row[r].index].yoff;
            }
        }
        
        for (int r = 0; r < row_count; r++) {
            int idx = current_row[r].index;
            int glyph_top = max_yoff - temp_glyphs[idx].yoff;
            
            for (int y = 0; y < temp_glyphs[idx].height; y++) {
                for (int x = 0; x < temp_glyphs[idx].width; x++) {
                    int atlas_x = atlas.chars[idx].x + x;
                    int atlas_y = atlas.chars[idx].y + glyph_top + y;
                    unsigned char alpha = temp_glyphs[idx].bitmap[y * temp_glyphs[idx].width + x];

                    int pixel_idx = (atlas_y * atlas_width + atlas_x) * 3;
                    atlas.image.data[pixel_idx + 0] = alpha;
                    atlas.image.data[pixel_idx + 1] = alpha;
                    atlas.image.data[pixel_idx + 2] = alpha;
                }
            }
            
            atlas.chars[idx].y = atlas.chars[idx].y + glyph_top;
        }
    }
    
    free(current_row);
    
    for (int i = 0; i < charset_len; i++) {
        if (temp_glyphs[i].bitmap) {
            glyph_ttf_free_bitmap(temp_glyphs[i].bitmap);
        }
    }
    free(temp_glyphs);
    glyph_ttf_free_font(&font);
    
    return atlas;
}

void glyph_atlas_free(glyph_atlas_t* atlas) {
    if (!atlas) return;
    if (atlas->chars) {
        free(atlas->chars);
        atlas->chars = NULL;
    }
    glyph_image_free(&atlas->image);
    atlas->num_chars = 0;
}

int glyph_atlas_save_png(glyph_atlas_t* atlas, const char* output_path) {
    if (!atlas || !atlas->image.data) return -1;
    return glyph_write_png(output_path, &atlas->image);
}

int glyph_atlas_save_bmp(glyph_atlas_t* atlas, const char* output_path) {
    if (!atlas || !atlas->image.data) return -1;
    return glyph_write_bmp(output_path, &atlas->image);
}

int glyph_atlas_save_metadata(glyph_atlas_t* atlas, const char* output_path) {
    if (!atlas || !atlas->chars) return -1;
    
    FILE* f = fopen(output_path, "w");
    if (!f) return -1;
    
    fprintf(f, "# Font Atlas Metadata\n");
    fprintf(f, "pixel_height: %.2f\n", atlas->pixel_height);
    fprintf(f, "atlas_width: %u\n", atlas->image.width);
    fprintf(f, "atlas_height: %u\n", atlas->image.height);
    fprintf(f, "num_chars: %d\n\n", atlas->num_chars);
    fprintf(f, "# codepoint x y width height xoff yoff advance\n");
    
    for (int i = 0; i < atlas->num_chars; i++) {
        glyph_atlas_char_t* c = &atlas->chars[i];
        fprintf(f, "%d %d %d %d %d %d %d %d\n", 
                c->codepoint, c->x, c->y, c->width, c->height, 
                c->xoff, c->yoff, c->advance);
    }
    
    fclose(f);
    return 0;
}

glyph_atlas_char_t* glyph_atlas_find_char(glyph_atlas_t* atlas, int codepoint) {
    if (!atlas || !atlas->chars) return NULL;
    
    for (int i = 0; i < atlas->num_chars; i++) {
        if (atlas->chars[i].codepoint == codepoint) {
            return &atlas->chars[i];
        }
    }
    return NULL;
}

void glyph_atlas_print_info(glyph_atlas_t* atlas) {
    if (!atlas) return;
    
    printf("Font Atlas Info:\n");
    printf("  Atlas Size: %ux%u\n", atlas->image.width, atlas->image.height);
    printf("  Pixel Height: %.2f\n", atlas->pixel_height);
    printf("  Characters: %d\n", atlas->num_chars);
    printf("\nCharacter Details:\n");
    
    for (int i = 0; i < atlas->num_chars; i++) {
        glyph_atlas_char_t* c = &atlas->chars[i];
        char ch = (c->codepoint >= 32 && c->codepoint < 127) ? c->codepoint : '?';
        printf("  '%c' (U+%04X): pos=(%d,%d) size=(%dx%d) offset=(%d,%d) advance=%d\n",
               ch, c->codepoint, c->x, c->y, c->width, c->height, 
               c->xoff, c->yoff, c->advance);
    }
}

#endif