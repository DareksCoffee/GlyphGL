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
 * v1.0.1 | 2025-10-28
 * | - Added 'glyph_renderer_update_projection' for handling window resize events.
 * | - Implemented text styling via bitmask: GLYPHGL_BOLD, GLYPHGL_UNDERLINE, GLYPHGL_ITALIC.
 * | - Optimized endianness conversions
 * | - Optimized contour decoding in 'glyph_ttf_get_glyph_bitmap' 
 * | - Optimized offset lookups in 'glyph_ttf__get_glyph_offset'
 *
 * ========================================================
 */

#ifndef __GLYPH_H
#define __GLYPH_H

#include "glyph_truetype.h"
#include "glyph_image.h"
#include "glyph_atlas.h"
#include "glyph_gl.h"

#define GLYPHGL_BOLD        (1 << 0)
#define GLYPHGL_ITALIC      (1 << 1)
#define GLYPHGL_UNDERLINE   (1 << 2)

#define GLYPH_NONE           0


typedef struct {
    glyph_atlas_t atlas;
    GLuint texture;
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    int initialized;
} glyph_renderer_t;


glyph_renderer_t glyph_renderer_create(const char* font_path, float pixel_height, const char* charset) {
    glyph_renderer_t renderer = {0};
    
    if (!glyph_gl_load_functions()) {
        printf("Failed to load OpenGL functions\n");
        return renderer;
    }
    
    renderer.atlas = glyph_atlas_create(font_path, pixel_height, charset);
    if (!renderer.atlas.chars || !renderer.atlas.image.data) {
        printf("Failed to create font atlas\n");
        return renderer;
    }
    
    unsigned char* red_channel = (unsigned char*)malloc(renderer.atlas.image.width * renderer.atlas.image.height);
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
    
    free(red_channel);
    
    renderer.shader = glyph__create_program(glyph__vertex_shader_source, glyph__fragment_shader_source);
    if (!renderer.shader) {
        glDeleteTextures(1, &renderer.texture);
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }
    
    glyph__glGenVertexArrays(1, &renderer.vao);
    glyph__glGenBuffers(1, &renderer.vbo);
    glyph__glBindVertexArray(renderer.vao);
    glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glyph__glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, NULL, GL_DYNAMIC_DRAW);
    glyph__glEnableVertexAttribArray(0);
    glyph__glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glyph__glEnableVertexAttribArray(1);
    glyph__glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);
    glyph__glBindVertexArray(0);
    
    renderer.initialized = 1;
    return renderer;
}

void glyph_renderer_free(glyph_renderer_t* renderer) {
    if (!renderer || !renderer->initialized) return;
    
    glyph__glDeleteVertexArrays(1, &renderer->vao);
    glyph__glDeleteBuffers(1, &renderer->vbo);
    glDeleteTextures(1, &renderer->texture);
    glyph__glDeleteProgram(renderer->shader);
    glyph_atlas_free(&renderer->atlas);
    
    renderer->initialized = 0;
}

void glyph_renderer_set_projection(glyph_renderer_t* renderer, int width, int height) {
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

void glyph_renderer_update_projection(glyph_renderer_t* renderer, int width, int height) {
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

void glyph_renderer_draw_text(glyph_renderer_t* renderer, const char* text, float x, float y, float scale,
                              float r, float g, float b, int effects) {
    if (!renderer || !renderer->initialized) return;

    glyph__glUseProgram(renderer->shader);
    glyph__glBindVertexArray(renderer->vao);
    glyph__glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->texture);
    glyph__glUniform1i(glyph__glGetUniformLocation(renderer->shader, "textTexture"), 0);
    glyph__glUniform3f(glyph__glGetUniformLocation(renderer->shader, "textColor"), r, g, b);
    glyph__glUniform1i(glyph__glGetUniformLocation(renderer->shader, "effects"), effects);
    
    float current_x = x;
    for (size_t i = 0; i < strlen(text); ++i) {
        int codepoint = (unsigned char)text[i];
        glyph_atlas_char_t* ch = glyph_atlas_find_char(&renderer->atlas, codepoint);
        if (!ch || ch->width == 0) {
            current_x += ch ? ch->advance * scale : 0;
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

        float vertices[24] = {
            xpos,     ypos + h,   tex_x1, tex_y2,
            xpos,     ypos,       tex_x1, tex_y1,
            xpos + w, ypos,       tex_x2, tex_y1,

            xpos,     ypos + h,   tex_x1, tex_y2,
            xpos + w, ypos,       tex_x2, tex_y1,
            xpos + w, ypos + h,   tex_x2, tex_y2
        };

        if (effects & GLYPHGL_ITALIC) {
            float shear = 0.2f;
            vertices[0] -= shear * h;  // top left x
            vertices[12] -= shear * h; // top left x (second triangle)
            vertices[20] -= shear * h; // top right x
        }

        glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
        glyph__glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

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
                bold_vertices[0] -= shear * h;  // top left x
                bold_vertices[12] -= shear * h; // top left x (second triangle)
                bold_vertices[20] -= shear * h; // top right x
            }

            glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
            glyph__glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bold_vertices), bold_vertices);
            glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
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
            glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
            glyph__glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(underline_vertices), underline_vertices);
            glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        current_x += ch->advance * scale;
    }
    
    glyph__glBindVertexArray(0);
    glyph__glUseProgram(0);
}

#endif