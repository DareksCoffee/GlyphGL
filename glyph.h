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
 * ================== GLYPHGL UPDATE LOG ==================
 *
 * v1.0.0 | [Initial Release]
 * | - Custom TrueType parser and winding-rule rasterizer.
 * | - Zero-dependency GL loader (cross-platform).
 * | - Atlas-based rendering (GL_RED).
 *
 * v1.0.2 | 2025-10-28
 * | - Added 'glyph_renderer_update_projection' for handling window resize events.
 * | - Implemented text styling via bitmask: GLYPHGL_BOLD, GLYPHGL_UNDERLINE, GLYPHGL_ITALIC.
 * | - Optimized endianness conversions
 * | - Optimized contour decoding in 'glyph_ttf_get_glyph_bitmap' 
 * | - Optimized offset lookups in 'glyph_ttf__get_glyph_offset'
 * v1.0.3 | 2025-10-29
 * | - Added vertex batching as per the request from u/MGJared
 * | - Implemented custom memory allocation macros (GLYPH_MALLOC, GLYPH_FREE, GLYPH_REALLOC), for now relatively basic
 * v1.0.4 | 2025-10-29
 * | - Fixed memory fragmentation issue in glyph_renderer_draw_text by implementing a persistent vertex buffer
 * | - Replaced per-draw dynamic allocation with pre-allocated buffer that grows as needed
 * | - Reduces allocations from O(text_length) to O(1) for better performance in high-frequency rendering
 * v1.0.5 | 2025-10-30
 * | - Added 'GLYPHGL_DEBUG' macro to debug the library
 * | - Added 'GLYPH_LOG''
 * | - Created 'demos' and 'examples' folders
 * | - Added 'glyph_effect.h' that allows custom shader creation and includes many built in shaders
 * v1.0.6 | 2025-10-30
 * | - Added 'GLYPHGL_MINIMAL' compile-time flag to disable heavy features like effects
 * | - Implemented minimal shader path (no effects, luminance-based alpha) behind compile-time flag
 * | - Deferred large allocations (atlas channel copy) in minimal mode for reduced memory footprint
 * | - Exposed atlas/vertex-buffer sizes as configurable parameters (GLYPHGL_ATLAS_WIDTH, GLYPHGL_ATLAS_HEIGHT, GLYPHGL_VERTEX_BUFFER_SIZE)
 * | - Apps can now pick smaller defaults for memory-constrained environments
 * v1.0.7 | 2025-10-31
 * | - Added SDF (Signed Distance Field) support for smoother text rendering at various scales
 * | - Implemented SDF rendering with configurable spread parameter
 * | - Added GLYPHGL_SDF flag for enabling SDF mode in glyph_renderer_create
 * | - Fixed C++ compatibility by changing <cstddef> to <stddef.h> in glyph_gl.h
 * | - Made library 'true' header-only by adding 'static inline' to all function definitions
 * | - Resolved multiple definition linker errors when including headers in multiple C/C++ files
 * ========================================================
 */

#ifndef __GLYPH_H
#define __GLYPH_H

#ifndef GLYPHGL_ATLAS_WIDTH
#define GLYPHGL_ATLAS_WIDTH 2048
#endif
#ifndef GLYPHGL_ATLAS_HEIGHT
#define GLYPHGL_ATLAS_HEIGHT 2048
#endif
#ifndef GLYPHGL_VERTEX_BUFFER_SIZE
#define GLYPHGL_VERTEX_BUFFER_SIZE 73728
#endif


#include <stdlib.h>
#include "glyph_truetype.h"
#include "glyph_image.h"
#include "glyph_gl.h"
#include "glyph_util.h"
#ifndef GLYPHGL_MINIMAL
#include "glyph_effect.h"
#endif

#define GLYPHGL_BOLD        (1 << 0)
#define GLYPHGL_ITALIC      (1 << 1)
#define GLYPHGL_UNDERLINE   (1 << 2)
#define GLYPHGL_SDF         (1 << 3)


#define GLYPH_NONE           0
#define GLYPH_UTF8           0x010
#define GLYPH_ASCII          0x020

#include "glyph_atlas.h"


/*
    Default charsets for the atlas
*/
#define GLYPHGL_CHARSET_BASIC "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%%^&*()_+-=,./?|\n"
#define GLYPHGL_CHARSET_DEFAULT GLYPHGL_CHARSET_BASIC "€£¥¢₹₽±×÷√∫πΩ°∞≠≈≤≥∑∏∂∇∀∃∈∉⊂⊃∩∪←↑→↓"


static int glyph_utf8_decode(const char* str, size_t* index);

typedef struct {
    glyph_atlas_t atlas;
    GLuint texture;
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    float* vertex_buffer;
    size_t vertex_buffer_size;
    int initialized;
    uint32_t char_type;
    float cached_text_color[3];
    int cached_effects;
#ifndef GLYPHGL_MINIMAL
    glyph_effect_t effect;
#endif
} glyph_renderer_t;


static inline glyph_renderer_t glyph_renderer_create(const char* font_path, float pixel_height, const char* charset, uint32_t char_type, void* effect, int use_sdf) {
#ifndef GLYPHGL_MINIMAL
    glyph_effect_t default_effect = {(glyph_effect_type_t)GLYPH_NONE, NULL, NULL};
    if (effect == NULL) {
        effect = &default_effect;
    }
#endif
    glyph_renderer_t renderer = {0};

    if (!glyph_gl_load_functions()) {
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Failed to load OpenGL functions\n");
        #endif
        return renderer;
    }

    renderer.char_type = char_type;
#ifndef GLYPHGL_MINIMAL
    renderer.effect = *(glyph_effect_t*)effect;
#endif
    renderer.atlas = glyph_atlas_create(font_path, pixel_height, charset, char_type, use_sdf);
    if (!renderer.atlas.chars || !renderer.atlas.image.data) {
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Failed to create font atlas\n");
        #endif
        return renderer;
    }

    // Defer atlas channel copy for minimal builds - upload directly from RGB data
#ifndef GLYPHGL_MINIMAL
    unsigned char* red_channel = (unsigned char*)GLYPH_MALLOC(renderer.atlas.image.width * renderer.atlas.image.height);
    if (!red_channel) {
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }

    for (unsigned int i = 0; i < renderer.atlas.image.width * renderer.atlas.image.height; i++) {
        red_channel[i] = renderer.atlas.image.data[i * 3];
    }

    glGenTextures(1, &renderer.texture);
    glBindTexture(GL_TEXTURE_2D, renderer.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, renderer.atlas.image.width, renderer.atlas.image.height,
                  0, GL_RED, GL_UNSIGNED_BYTE, red_channel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLYPH_FREE(red_channel);
#else
    // Minimal mode: upload RGB texture directly (no channel extraction)
    glGenTextures(1, &renderer.texture);
    glBindTexture(GL_TEXTURE_2D, renderer.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderer.atlas.image.width, renderer.atlas.image.height,
                  0, GL_RGB, GL_UNSIGNED_BYTE, renderer.atlas.image.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

#ifndef GLYPHGL_MINIMAL
    if (renderer.effect.type == GLYPH_NONE) {
        renderer.shader = glyph__create_program(glyph__vertex_shader_source, glyph__fragment_shader_source);
    } else {
        renderer.shader = glyph__create_program(renderer.effect.vertex_shader, renderer.effect.fragment_shader);
    }
#else
    renderer.shader = glyph__create_program(glyph__vertex_shader_source, glyph__fragment_shader_source);
#endif
    if (!renderer.shader) {
        glDeleteTextures(1, &renderer.texture);
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }

    glyph__glGenVertexArrays(1, &renderer.vao);
    glyph__glGenBuffers(1, &renderer.vbo);
    glyph__glBindVertexArray(renderer.vao);
    glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glyph__glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GLYPHGL_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
    glyph__glEnableVertexAttribArray(0);
    glyph__glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glyph__glEnableVertexAttribArray(1);
    glyph__glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);
    glyph__glBindVertexArray(0);

    renderer.vertex_buffer_size = GLYPHGL_VERTEX_BUFFER_SIZE * 4; // Initial size for vertices (float * 4 per vertex)
    renderer.vertex_buffer = (float*)GLYPH_MALLOC(sizeof(float) * renderer.vertex_buffer_size);
    if (!renderer.vertex_buffer) {
        glyph__glDeleteVertexArrays(1, &renderer.vao);
        glyph__glDeleteBuffers(1, &renderer.vbo);
        glDeleteTextures(1, &renderer.texture);
        glyph__glDeleteProgram(renderer.shader);
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }

    // Initialize uniform caches
    renderer.cached_text_color[0] = -1.0f;
    renderer.cached_text_color[1] = -1.0f;
    renderer.cached_text_color[2] = -1.0f;
    renderer.cached_effects = -1;

    renderer.initialized = 1;
    return renderer;
}

static inline void glyph_renderer_free(glyph_renderer_t* renderer) {
    if (!renderer || !renderer->initialized) return;

    glyph__glDeleteVertexArrays(1, &renderer->vao);
    glyph__glDeleteBuffers(1, &renderer->vbo);
    glDeleteTextures(1, &renderer->texture);
    glyph__glDeleteProgram(renderer->shader);
    glyph_atlas_free(&renderer->atlas);
    GLYPH_FREE(renderer->vertex_buffer);

    renderer->initialized = 0;
}

static inline void glyph_renderer_set_projection(glyph_renderer_t* renderer, int width, int height) {
    if (!renderer || !renderer->initialized) return;
    
    float projection[16] = {
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    glyph__glUseProgram(renderer->shader);
    glyph__glUniformMatrix4fv(glyph__glGetUniformLocation(renderer->shader, "projection"), 1, GL_FALSE, projection);
    glyph__glUseProgram(0);
}

static inline void glyph_renderer_update_projection(glyph_renderer_t* renderer, int width, int height) {
    if (!renderer || !renderer->initialized) return;

    float projection[16] = {
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    glyph__glUseProgram(renderer->shader);
    glyph__glUniformMatrix4fv(glyph__glGetUniformLocation(renderer->shader, "projection"), 1, GL_FALSE, projection);
    glyph__glUseProgram(0);
}

static inline void glyph_renderer_draw_text(glyph_renderer_t* renderer, const char* text, float x, float y, float scale,
                                 float r, float g, float b, int effects) {
    if (!renderer || !renderer->initialized) return;

    glyph__glUseProgram(renderer->shader);
    glyph__glBindVertexArray(renderer->vao);
    glyph__glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->texture);

    // Batch uniform updates - only update if changed
    if (renderer->cached_text_color[0] != r || renderer->cached_text_color[1] != g || renderer->cached_text_color[2] != b) {
        glyph__glUniform3f(glyph__glGetUniformLocation(renderer->shader, "textColor"), r, g, b);
        renderer->cached_text_color[0] = r;
        renderer->cached_text_color[1] = g;
        renderer->cached_text_color[2] = b;
    }
#ifndef GLYPHGL_MINIMAL
    if (renderer->cached_effects != effects) {
        glyph__glUniform1i(glyph__glGetUniformLocation(renderer->shader, "effects"), effects);
        renderer->cached_effects = effects;
    }
#endif

    size_t text_len = strlen(text);
    size_t required_size = sizeof(float) * 24 * text_len * 3; // Estimate based on max effects (normal + bold + underline)
    if (required_size > renderer->vertex_buffer_size) {
        size_t new_size = required_size * 2; // Double the size to minimize reallocations
        float* new_buffer = (float*)GLYPH_REALLOC(renderer->vertex_buffer, new_size);
        if (!new_buffer) return;
        renderer->vertex_buffer = new_buffer;
        renderer->vertex_buffer_size = new_size;
    }
    float* vertices = renderer->vertex_buffer;
    size_t vertex_count = 0;

    float current_x = x;
    size_t i = 0;
    while (i < text_len) {
        int codepoint;
        if (renderer->char_type == GLYPH_UTF8) {
            codepoint = glyph_utf8_decode(text, &i);
        } else {
            codepoint = (unsigned char)text[i];
            i++;
        }
        glyph_atlas_char_t* ch = glyph_atlas_find_char(&renderer->atlas, codepoint);
        if (!ch) {
            ch = glyph_atlas_find_char(&renderer->atlas, '?');
        }
        if (!ch || ch->width == 0) {
            current_x += ch ? ch->advance * scale : (renderer->atlas.pixel_height * 0.5f * scale);
            continue;
        }

        float xpos = current_x + ch->xoff * scale;
        float ypos = y - ch->yoff * scale;
        float w = ch->width * scale;
        float h = ch->height * scale;

        float tex_x1 = (float)ch->x / renderer->atlas.image.width;
        float tex_y1 = (float)ch->y / renderer->atlas.image.height;
        float tex_x2 = (float)(ch->x + ch->width) / renderer->atlas.image.width;
        float tex_y2 = (float)(ch->y + ch->height) / renderer->atlas.image.height;

        float glyph_vertices[24] = {
            xpos,     ypos + h,   tex_x1, tex_y2,
            xpos,     ypos,       tex_x1, tex_y1,
            xpos + w, ypos,       tex_x2, tex_y1,

            xpos,     ypos + h,   tex_x1, tex_y2,
            xpos + w, ypos,       tex_x2, tex_y1,
            xpos + w, ypos + h,   tex_x2, tex_y2
        };

#ifndef GLYPHGL_MINIMAL
        if (effects & GLYPHGL_ITALIC) {
            float shear = 0.2f;
            glyph_vertices[0] -= shear * h;
            glyph_vertices[12] -= shear * h;
            glyph_vertices[20] -= shear * h;
        }
#endif

        memcpy(vertices + vertex_count * 4, glyph_vertices, sizeof(glyph_vertices));
        vertex_count += 6;

#ifndef GLYPHGL_MINIMAL
        if (effects & GLYPHGL_BOLD) {
            float bold_offset = 1.0f * scale;
            float bold_vertices[24] = {
                xpos + bold_offset,     ypos + h,   tex_x1, tex_y2,
                xpos + bold_offset,     ypos,       tex_x1, tex_y1,
                xpos + w + bold_offset, ypos,       tex_x2, tex_y1,

                xpos + bold_offset,     ypos + h,   tex_x1, tex_y2,
                xpos + w + bold_offset, ypos,       tex_x2, tex_y1,
                xpos + w + bold_offset, ypos + h,   tex_x2, tex_y2
            };

            if (effects & GLYPHGL_ITALIC) {
                float shear = 0.2f;
                bold_vertices[0] -= shear * h;
                bold_vertices[12] -= shear * h;
                bold_vertices[20] -= shear * h;
            }

            memcpy(vertices + vertex_count * 4, bold_vertices, sizeof(bold_vertices));
            vertex_count += 6;
        }

        if (effects & GLYPHGL_UNDERLINE) {
            float underline_y = y + h * 0.1f;
            float underline_vertices[24] = {
                current_x, underline_y + 2, 0.0f, 0.0f,
                current_x, underline_y,     0.0f, 0.0f,
                current_x + ch->advance * scale, underline_y,     0.0f, 0.0f,

                current_x, underline_y + 2, 0.0f, 0.0f,
                current_x + ch->advance * scale, underline_y,     0.0f, 0.0f,
                current_x + ch->advance * scale, underline_y + 2, 0.0f, 0.0f
            };
            memcpy(vertices + vertex_count * 4, underline_vertices, sizeof(underline_vertices));
            vertex_count += 6;
        }
#endif

        current_x += ch->advance * scale;
    }

    glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glyph__glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * 4 * sizeof(float), vertices);
    glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, vertex_count);


    glyph__glBindVertexArray(0);
    glyph__glUseProgram(0);
}

static inline GLuint glyph_renderer_get_vao(glyph_renderer_t* renderer) {
    return renderer->vao;
}

static inline GLuint glyph_renderer_get_vbo(glyph_renderer_t* renderer) {
    return renderer->vbo;
}

static inline GLuint glyph_renderer_get_shader(glyph_renderer_t* renderer) {
    return renderer->shader;
}

static inline int glyph_utf8_decode(const char* str, size_t* index) {
    size_t i = *index;
    unsigned char c = (unsigned char)str[i++];
    if (c < 0x80) {
        *index = i;
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        unsigned char c2 = (unsigned char)str[i++];
        *index = i;
        return ((c & 0x1F) << 6) | (c2 & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        unsigned char c2 = (unsigned char)str[i++];
        unsigned char c3 = (unsigned char)str[i++];
        *index = i;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        unsigned char c2 = (unsigned char)str[i++];
        unsigned char c3 = (unsigned char)str[i++];
        unsigned char c4 = (unsigned char)str[i++];
        *index = i;
        return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
    }
    *index = i;
    return 0xFFFD;
}

#endif

