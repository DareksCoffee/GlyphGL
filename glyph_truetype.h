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


#ifndef __GLYPH_TTF_H
#define __GLYPH_TTF_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "glyph_util.h"

typedef struct {
    unsigned char* data;
    int fontstart;
    int numGlyphs;
    int loca, head, glyf, hhea, hmtx, kern, gpos, cmap;
    int index_map;
    int indexToLocFormat;
    float scale;
} glyph_font_t;

typedef struct {
    int x0, y0, x1, y1;
    int advance, left_side_bearing;
} glyph_bbox_t;

typedef struct {
    float x, y;
    int on_curve;
} glyph_point_t;

static inline int glyph_ttf_init(glyph_font_t* font, const unsigned char* data, int offset);
static inline int glyph_ttf_find_glyph_index(const glyph_font_t* font, int codepoint);
static inline void glyph_ttf_get_glyph_bbox(const glyph_font_t* font, int glyph_index, glyph_bbox_t* bbox);
static inline unsigned char* glyph_ttf_get_glyph_bitmap(const glyph_font_t* font, int glyph_index, float scale_x, float scale_y, int* width, int* height, int* xoff, int* yoff);
static inline void glyph_ttf_free_bitmap(unsigned char* bitmap);
static inline unsigned char* glyph_ttf_get_glyph_sdf_bitmap(unsigned char* bitmap, int w, int h, int spread);
static inline float glyph_ttf_scale_for_pixel_height(const glyph_font_t* font, float pixels);
static inline int glyph_ttf_get_glyph_advance(const glyph_font_t* font, int glyph_index);

static int glyph_ttf__isfont(const unsigned char* font);
static int glyph_ttf__find_table(const unsigned char* data, int fontstart, const char* tag);
static int glyph_ttf__get16(const unsigned char* data, int offset);
static unsigned int glyph_ttf__get16u(const unsigned char* data, int offset);
static int glyph_ttf__get32(const unsigned char* data, int offset);
static int glyph_ttf__get_glyph_offset(const glyph_font_t* font, int glyph_index);
static void glyph_ttf__rasterize_contour(unsigned char* bitmap, int w, int h, glyph_point_t* points, int n_points, float offset_x, float offset_y);
static void glyph_ttf__draw_line_aa(float* accum, int w, int h, float x0, float y0, float x1, float y1);
static void glyph_ttf__add_edge(float* accum, int w, int h, float x0, float y0, float x1, float y1);

static int glyph_ttf__isfont(const unsigned char* font) {
    return font[0] == 't' && font[1] == 't' && font[2] == 'c' && font[3] == 'f' ||
           font[0] == 0x00 && font[1] == 0x01 && font[2] == 0x00 && font[3] == 0x00 ||
           font[0] == 'O' && font[1] == 'T' && font[2] == 'T' && font[3] == 'O' ||
           font[0] == 't' && font[1] == 'r' && font[2] == 'u' && font[3] == 'e';
}

static int glyph_ttf__find_table(const unsigned char* data, int fontstart, const char* tag) {
    int num_tables = glyph_ttf__get16u(data, fontstart + 4);
    int tabledir = fontstart + 12;
    for (int i = 0; i < num_tables; ++i) {
        int loc = tabledir + 16 * i;
        if (data[loc] == tag[0] && data[loc+1] == tag[1] && data[loc+2] == tag[2] && data[loc+3] == tag[3])
            return glyph_ttf__get32(data, loc + 8);
    }
    return 0;
}

static int glyph_ttf__get16(const unsigned char* data, int offset) {
    uint16_t val;
    memcpy(&val, data + offset, 2);
    val = __builtin_bswap16(val);
    return (int16_t)val;
}

static unsigned int glyph_ttf__get16u(const unsigned char* data, int offset) {
    uint16_t val;
    memcpy(&val, data + offset, 2);
    return __builtin_bswap16(val);
}

static int glyph_ttf__get32(const unsigned char* data, int offset) {
    uint32_t val;
    memcpy(&val, data + offset, 4);
    return __builtin_bswap32(val);
}

static inline int glyph_ttf_init(glyph_font_t* font, const unsigned char* data, int offset) {
    font->data = (unsigned char*)data;
    font->fontstart = offset;
    if (!glyph_ttf__isfont(data + offset)) return 0;

    font->cmap = glyph_ttf__find_table(data, offset, "cmap");
    font->loca = glyph_ttf__find_table(data, offset, "loca");
    font->head = glyph_ttf__find_table(data, offset, "head");
    font->glyf = glyph_ttf__find_table(data, offset, "glyf");
    font->hhea = glyph_ttf__find_table(data, offset, "hhea");
    font->hmtx = glyph_ttf__find_table(data, offset, "hmtx");
    font->kern = glyph_ttf__find_table(data, offset, "kern");
    font->gpos = glyph_ttf__find_table(data, offset, "GPOS");

    if (!font->cmap || !font->head || !font->hhea || !font->hmtx) return 0;
    if (font->glyf) {
        if (!font->loca) return 0;
    }

    int index_map = 0;
    int indexToLocFormat = glyph_ttf__get16(data, font->head + 50);
    font->indexToLocFormat = indexToLocFormat;

    int cmap_offset = font->cmap;
    int numTables = glyph_ttf__get16u(data, cmap_offset + 2);
    for (int i = 0; i < numTables; ++i) {
        int platformID = glyph_ttf__get16u(data, cmap_offset + 4 + 8 * i);
        int platformSpecificID = glyph_ttf__get16u(data, cmap_offset + 4 + 8 * i + 2);
        int offset_sub = glyph_ttf__get32(data, cmap_offset + 4 + 8 * i + 4);
        if (platformID == 0 || (platformID == 3 && (platformSpecificID == 1 || platformSpecificID == 10))) {
            index_map = cmap_offset + offset_sub;
            break;
        }
    }
    if (!index_map) return 0;
    font->index_map = index_map;

    font->numGlyphs = glyph_ttf__get16u(data, font->hhea + 34);
    return 1;
}

static inline int glyph_ttf_find_glyph_index(const glyph_font_t* font, int codepoint) {
    const unsigned char* data = font->data;
    int index_map = font->index_map;
    int format = glyph_ttf__get16u(data, index_map);
    if (format == 0) {
        int bytes = glyph_ttf__get16u(data, index_map + 2);
        if (codepoint < bytes - 6)
            return data[index_map + 6 + codepoint];
        return 0;
    } else if (format == 6) {
        int first = glyph_ttf__get16u(data, index_map + 6);
        int count = glyph_ttf__get16u(data, index_map + 8);
        if (codepoint >= first && codepoint < first + count)
            return glyph_ttf__get16u(data, index_map + 10 + (codepoint - first) * 2);
        return 0;
    } else if (format == 4) {
        int segcount = glyph_ttf__get16u(data, index_map + 6) >> 1;
        int endCount = index_map + 14;
        int startCount = endCount + segcount * 2 + 2;
        int idDelta = startCount + segcount * 2;
        int idRangeOffset = idDelta + segcount * 2;
        for (int i = 0; i < segcount; ++i) {
            int end = glyph_ttf__get16u(data, endCount + i * 2);
            if (codepoint <= end) {
                int start = glyph_ttf__get16u(data, startCount + i * 2);
                if (codepoint >= start) {
                    int delta = glyph_ttf__get16(data, idDelta + i * 2);
                    int rangeOffset = glyph_ttf__get16u(data, idRangeOffset + i * 2);
                    if (rangeOffset == 0) {
                        return (codepoint + delta) & 0xFFFF;
                    } else {
                        int glyphIndex = glyph_ttf__get16u(data, idRangeOffset + i * 2 + rangeOffset + (codepoint - start) * 2);
                        return glyphIndex ? (glyphIndex + delta) & 0xFFFF : 0;
                    }
                }
            }
        }
        return 0;
    } else if (format == 12 || format == 13) {
        int nGroups = glyph_ttf__get32(data, index_map + 12);
        for (int i = 0; i < nGroups; ++i) {
            int startCharCode = glyph_ttf__get32(data, index_map + 16 + i * 12);
            int endCharCode = glyph_ttf__get32(data, index_map + 16 + i * 12 + 4);
            if (codepoint >= startCharCode && codepoint <= endCharCode) {
                if (format == 12)
                    return glyph_ttf__get32(data, index_map + 16 + i * 12 + 8) + (codepoint - startCharCode);
                else
                    return glyph_ttf__get32(data, index_map + 16 + i * 12 + 8);
            }
        }
        return 0;
    }
    return 0;
}

static inline void glyph_ttf_get_glyph_bbox(const glyph_font_t* font, int glyph_index, glyph_bbox_t* bbox) {
    if (glyph_index >= font->numGlyphs) {
        bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0;
        return;
    }
    const unsigned char* data = font->data;
    int g = glyph_ttf__get_glyph_offset(font, glyph_index);
    if (g < 0) {
        bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0;
        return;
    }
    bbox->x0 = glyph_ttf__get16(data, g + 2);
    bbox->y0 = glyph_ttf__get16(data, g + 4);
    bbox->x1 = glyph_ttf__get16(data, g + 6);
    bbox->y1 = glyph_ttf__get16(data, g + 8);
}

static inline int glyph_ttf_get_glyph_advance(const glyph_font_t* font, int glyph_index) {
    const unsigned char* data = font->data;
    int numOfLongHorMetrics = glyph_ttf__get16u(data, font->hhea + 34);
    if (glyph_index < numOfLongHorMetrics)
        return glyph_ttf__get16u(data, font->hmtx + 4 * glyph_index);
    else
        return glyph_ttf__get16u(data, font->hmtx + 4 * (numOfLongHorMetrics - 1));
}

static inline float glyph_ttf_scale_for_pixel_height(const glyph_font_t* font, float pixels) {
    const unsigned char* data = font->data;
    int unitsPerEm = glyph_ttf__get16u(data, font->head + 18);
    return pixels / unitsPerEm;
}

static int glyph_ttf__get_glyph_offset(const glyph_font_t* font, int glyph_index) {
    const unsigned char* data = font->data;
    int offset = font->loca + glyph_index * (font->indexToLocFormat ? 4 : 2);
    int g1 = font->glyf + (font->indexToLocFormat ? glyph_ttf__get32(data, offset) : glyph_ttf__get16u(data, offset) * 2);
    int g2 = font->glyf + (font->indexToLocFormat ? glyph_ttf__get32(data, offset + 4) : glyph_ttf__get16u(data, offset + 2) * 2);
    return g1 == g2 ? -1 : g1;
}

static void glyph_ttf__add_edge(float* accum, int w, int h, float x0, float y0, float x1, float y1) {
    if (fabs(y1 - y0) < 0.001f) return;
    
    int dir = 1;
    if (y0 > y1) {
        float tmp;
        tmp = y0; y0 = y1; y1 = tmp;
        tmp = x0; x0 = x1; x1 = tmp;
        dir = -1;
    }
    
    int y_start = (int)floorf(y0);
    int y_end = (int)ceilf(y1);
    
    if (y_end <= 0 || y_start >= h) return;
    if (y_start < 0) y_start = 0;
    if (y_end > h) y_end = h;
    
    float dx = x1 - x0;
    float dy = y1 - y0;
    
    for (int y = y_start; y < y_end; ++y) {
        float sy0 = (float)y;
        float sy1 = (float)(y + 1);
        
        if (sy0 < y0) sy0 = y0;
        if (sy1 > y1) sy1 = y1;
        
        float coverage = sy1 - sy0;
        float y_mid = (sy0 + sy1) * 0.5f;
        float x_mid = x0 + dx * (y_mid - y0) / dy;
        
        int x_int = (int)floorf(x_mid);
        if (x_int >= 0 && x_int < w) {
            accum[y * w + x_int] += coverage * dir;
        }
    }
}

static void glyph_ttf__rasterize_shape(unsigned char* bitmap, int w, int h, glyph_point_t** contours, int* contour_sizes, int num_contours) {
    float* accum = (float*)calloc(w * h, sizeof(float));
    if (!accum) return;
    
    for (int c = 0; c < num_contours; ++c) {
        glyph_point_t* points = contours[c];
        int n_points = contour_sizes[c];
        
        if (n_points < 2) continue;
        
        int i = 0;
        while (i < n_points) {
            glyph_point_t p0 = points[i];
            int next = (i + 1) % n_points;
            glyph_point_t p1 = points[next];
            
            if (p0.on_curve && p1.on_curve) {
                glyph_ttf__add_edge(accum, w, h, p0.x, p0.y, p1.x, p1.y);
                i++;
            } else if (p0.on_curve && !p1.on_curve) {
                int next2 = (i + 2) % n_points;
                glyph_point_t p2 = points[next2];
                
                if (!p2.on_curve) {
                    p2.x = (p1.x + p2.x) * 0.5f;
                    p2.y = (p1.y + p2.y) * 0.5f;
                }
                
                float prev_x = p0.x;
                float prev_y = p0.y;
                
                int steps = 32;
                for (int t = 1; t <= steps; ++t) {
                    float u = (float)t / steps;
                    float b0 = (1 - u) * (1 - u);
                    float b1 = 2 * (1 - u) * u;
                    float b2 = u * u;
                    
                    float x = b0 * p0.x + b1 * p1.x + b2 * p2.x;
                    float y = b0 * p0.y + b1 * p1.y + b2 * p2.y;
                    
                    glyph_ttf__add_edge(accum, w, h, prev_x, prev_y, x, y);
                    
                    prev_x = x;
                    prev_y = y;
                }
                
                if (!points[next2].on_curve) {
                    i += 1;
                } else {
                    i += 2;
                }
            } else {
                i++;
            }
        }
    }
    
    for (int y = 0; y < h; ++y) {
        float winding = 0;
        for (int x = 0; x < w; ++x) {
            winding += accum[y * w + x];
            float alpha = fabsf(winding);
            if (alpha > 1.0f) alpha = 1.0f;
            bitmap[y * w + x] = (unsigned char)(alpha * 255.0f);
        }
    }
    
    GLYPH_FREE(accum);
}

static inline unsigned char* glyph_ttf_get_glyph_bitmap(const glyph_font_t* font, int glyph_index, float scale_x, float scale_y, int* width, int* height, int* xoff, int* yoff) {
    const unsigned char* data = font->data;
    int g = glyph_ttf__get_glyph_offset(font, glyph_index);
    if (g < 0) {
        *width = 0;
        *height = 0;
        *xoff = 0;
        *yoff = 0;
        return NULL;
    }

    int numberOfContours = glyph_ttf__get16(data, g);
    if (numberOfContours > 0) {
        int xMin = glyph_ttf__get16(data, g + 2);
        int yMin = glyph_ttf__get16(data, g + 4);
        int xMax = glyph_ttf__get16(data, g + 6);
        int yMax = glyph_ttf__get16(data, g + 8);

        int w = (int)ceilf((xMax - xMin) * scale_x) + 1;
        int h = (int)ceilf((yMax - yMin) * scale_y) + 1;
        if (w <= 0 || h <= 0) {
            *width = 0;
            *height = 0;
            *xoff = 0;
            *yoff = 0;
            return NULL;
        }

        unsigned char* bitmap = (unsigned char*)calloc(w * h, 1);
        if (!bitmap) return NULL;

        int endPtsOfContours = g + 10;
        int instructionLength = glyph_ttf__get16u(data, endPtsOfContours + numberOfContours * 2);
        int instructions = endPtsOfContours + numberOfContours * 2 + 2;
        int flags_start = instructions + instructionLength;

        int lastEndPt = glyph_ttf__get16u(data, endPtsOfContours + (numberOfContours - 1) * 2);
        int n_points = lastEndPt + 1;

        unsigned char* point_flags = (unsigned char*)GLYPH_MALLOC(n_points);
        int* x_coords = (int*)GLYPH_MALLOC(n_points * sizeof(int));
        int* y_coords = (int*)GLYPH_MALLOC(n_points * sizeof(int));

        if (!x_coords || !y_coords || !point_flags) {
            GLYPH_FREE(x_coords);
            GLYPH_FREE(y_coords);
            GLYPH_FREE(point_flags);
            GLYPH_FREE(bitmap);
            return NULL;
        }

        int flag_index = 0;
        int data_index = flags_start;
        while (flag_index < n_points) {
            unsigned char flag = data[data_index++];
            point_flags[flag_index++] = flag;
            if (flag & 8) {
                int repeat_count = data[data_index++];
                for (int r = 0; r < repeat_count && flag_index < n_points; ++r) {
                    point_flags[flag_index++] = flag;
                }
            }
        }

        int x = 0;
        for (int i = 0; i < n_points; ++i) {
            unsigned char flag = point_flags[i];
            if (flag & 2) {
                int dx = data[data_index++];
                if (!(flag & 16)) dx = -dx;
                x += dx;
            } else if (!(flag & 16)) {
                x += glyph_ttf__get16(data, data_index);
                data_index += 2;
            }
            x_coords[i] = x;
        }

        int y = 0;
        for (int i = 0; i < n_points; ++i) {
            unsigned char flag = point_flags[i];
            if (flag & 4) {
                int dy = data[data_index++];
                if (!(flag & 32)) dy = -dy;
                y += dy;
            } else if (!(flag & 32)) {
                y += glyph_ttf__get16(data, data_index);
                data_index += 2;
            }
            y_coords[i] = y;
        }

        glyph_point_t** contours = (glyph_point_t**)GLYPH_MALLOC(numberOfContours * sizeof(glyph_point_t*));
        int* contour_sizes = (int*)GLYPH_MALLOC(numberOfContours * sizeof(int));
        
        if (!contours || !contour_sizes) {
            GLYPH_FREE(contours);
            GLYPH_FREE(contour_sizes);
            GLYPH_FREE(x_coords);
            GLYPH_FREE(y_coords);
            GLYPH_FREE(point_flags);
            GLYPH_FREE(bitmap);
            return NULL;
        }
        
        for (int c = 0; c < numberOfContours; ++c) {
            int start_pt = (c == 0) ? 0 : glyph_ttf__get16u(data, endPtsOfContours + (c - 1) * 2) + 1;
            int end_pt = glyph_ttf__get16u(data, endPtsOfContours + c * 2);
            int contour_len = end_pt - start_pt + 1;
            
            glyph_point_t* contour = (glyph_point_t*)GLYPH_MALLOC(contour_len * 3 * sizeof(glyph_point_t));
            if (!contour) {
                contours[c] = NULL;
                contour_sizes[c] = 0;
                continue;
            }
            
            int out_idx = 0;
            for (int p = start_pt; p <= end_pt; ++p) {
                int next_p = (p == end_pt) ? start_pt : p + 1;
                
                contour[out_idx].x = (x_coords[p] - xMin) * scale_x;
                contour[out_idx].y = (yMax - y_coords[p]) * scale_y;
                contour[out_idx].on_curve = point_flags[p] & 1;
                out_idx++;
                
                if (!(point_flags[p] & 1) && !(point_flags[next_p] & 1)) {
                    contour[out_idx].x = ((x_coords[p] + x_coords[next_p]) * 0.5f - xMin) * scale_x;
                    contour[out_idx].y = (yMax - (y_coords[p] + y_coords[next_p]) * 0.5f) * scale_y;
                    contour[out_idx].on_curve = 1;
                    out_idx++;
                }
            }
            
            contours[c] = contour;
            contour_sizes[c] = out_idx;
        }
        
        glyph_ttf__rasterize_shape(bitmap, w, h, contours, contour_sizes, numberOfContours);
        
        for (int c = 0; c < numberOfContours; ++c) {
            GLYPH_FREE(contours[c]);
        }
        GLYPH_FREE(contours);
        GLYPH_FREE(contour_sizes);

        GLYPH_FREE(x_coords);
        GLYPH_FREE(y_coords);
        GLYPH_FREE(point_flags);

        *width = w;
        *height = h;
        *xoff = (int)(xMin * scale_x);
        *yoff = (int)(yMax * scale_y);

        return bitmap;
    } else {
        *width = 0;
        *height = 0;
        *xoff = 0;
        *yoff = 0;
        return NULL;
    }
}

static inline void glyph_ttf_free_bitmap(unsigned char* bitmap) {
    GLYPH_FREE(bitmap);
}

#endif

#include "glyph_image.h"

static void glyph_ttf_debug_glyph(const glyph_font_t* font, int glyph_index) {
    const unsigned char* data = font->data;
    int g = glyph_ttf__get_glyph_offset(font, glyph_index);
    if (g < 0) {
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Empty glyph\n");
        #endif
        return;
    }

    int numberOfContours = glyph_ttf__get16(data, g);
    #ifdef GLYPHGL_DEBUG
    GLYPH_LOG("Number of contours: %d\n", numberOfContours);
    #endif

    if (numberOfContours > 0) {
        int xMin = glyph_ttf__get16(data, g + 2);
        int yMin = glyph_ttf__get16(data, g + 4);
        int xMax = glyph_ttf__get16(data, g + 6);
        int yMax = glyph_ttf__get16(data, g + 8);
        GLYPH_LOG("Bounding box: (%d,%d) to (%d,%d)\n", xMin, yMin, xMax, yMax);

        int endPtsOfContours = g + 10;
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("End points of contours:\n");
        #endif
        for (int i = 0; i < numberOfContours; ++i) {
            int endPt = glyph_ttf__get16u(data, endPtsOfContours + i * 2);
            #ifdef GLYPHGL_DEBUG
            GLYPH_LOG("  Contour %d: ends at point %d\n", i, endPt);
            #endif
        }

        int instructionLength = glyph_ttf__get16u(data, endPtsOfContours + numberOfContours * 2);
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Instruction length: %d\n", instructionLength);
        #endif
        int lastEndPt = glyph_ttf__get16u(data, endPtsOfContours + (numberOfContours - 1) * 2);
        int totalPoints = lastEndPt + 1;
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Total points: %d\n", totalPoints);
        #endif
    }
}

static glyph_image_t glyph_ttf_render_glyph_to_image(const glyph_font_t* font, int glyph_index, float scale_x, float scale_y, unsigned char r, unsigned char g, unsigned char b) {
    int width, height, xoff, yoff;
    unsigned char* bitmap = glyph_ttf_get_glyph_bitmap(font, glyph_index, scale_x, scale_y, &width, &height, &xoff, &yoff);
    if (!bitmap) {
        glyph_image_t img = {0};
        return img;
    }

    glyph_image_t img = glyph_image_create(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char alpha = bitmap[y * width + x];
            int index = (y * width + x) * 3;
            img.data[index + 0] = (unsigned char)((r * alpha) / 255);
            img.data[index + 1] = (unsigned char)((g * alpha) / 255);
            img.data[index + 2] = (unsigned char)((b * alpha) / 255);
        }
    }
    glyph_ttf_free_bitmap(bitmap);
    return img;
}

static int glyph_ttf_load_font_from_file(glyph_font_t* font, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* data = (unsigned char*)GLYPH_MALLOC(size);
    if (!data) {
        fclose(f);
        return 0;
    }
    fread(data, 1, size, f);
    fclose(f);
    int result = glyph_ttf_init(font, data, 0);
    if (!result) GLYPH_FREE(data);
    return result;
}

static void glyph_ttf_free_font(glyph_font_t* font) {
    if (font->data) GLYPH_FREE(font->data);
    font->data = NULL;
}

static inline unsigned char* glyph_ttf_get_glyph_sdf_bitmap(unsigned char* bitmap, int w, int h, int spread) {
    unsigned char* mask = (unsigned char*)GLYPH_MALLOC(w * h);
    for(int i = 0; i < w * h; i++) {
        mask[i] = bitmap[i] > 127 ? 1 : 0;
    }
    float* dt1 = (float*)GLYPH_MALLOC(w * h * sizeof(float));
    for(int i = 0; i < w * h; i++) dt1[i] = mask[i] ? 0.0f : 1e9f;
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (x > 0) dt1[idx] = fminf(dt1[idx], dt1[idx-1] + 1.0f);
        }
        for(int x = w-1; x >= 0; x--) {
            int idx = y * w + x;
            if (x < w-1) dt1[idx] = fminf(dt1[idx], dt1[idx+1] + 1.0f);
        }
    }
    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            int idx = y * w + x;
            if (y > 0) dt1[idx] = fminf(dt1[idx], dt1[(y-1)*w + x] + 1.0f);
        }
        for(int y = h-1; y >= 0; y--) {
            int idx = y * w + x;
            if (y < h-1) dt1[idx] = fminf(dt1[idx], dt1[(y+1)*w + x] + 1.0f);
        }
    }
    float* dt0 = (float*)GLYPH_MALLOC(w * h * sizeof(float));
    for(int i = 0; i < w * h; i++) dt0[i] = mask[i] ? 1e9f : 0.0f;
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (x > 0) dt0[idx] = fminf(dt0[idx], dt0[idx-1] + 1.0f);
        }
        for(int x = w-1; x >= 0; x--) {
            int idx = y * w + x;
            if (x < w-1) dt0[idx] = fminf(dt0[idx], dt0[idx+1] + 1.0f);
        }
    }
    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            int idx = y * w + x;
            if (y > 0) dt0[idx] = fminf(dt0[idx], dt0[(y-1)*w + x] + 1.0f);
        }
        for(int y = h-1; y >= 0; y--) {
            int idx = y * w + x;
            if (y < h-1) dt0[idx] = fminf(dt0[idx], dt0[(y+1)*w + x] + 1.0f);
        }
    }
    unsigned char* sdf = (unsigned char*)GLYPH_MALLOC(w * h);
    for(int i = 0; i < w * h; i++) {
        float dist = mask[i] ? -dt0[i] : dt1[i];
        if (dist < -spread) dist = -spread;
        if (dist > spread) dist = spread;
        sdf[i] = (unsigned char)((dist / spread + 1.0f) * 0.5f * 255.0f);
    }
    GLYPH_FREE(mask);
    GLYPH_FREE(dt1);
    GLYPH_FREE(dt0);
    return sdf;
}