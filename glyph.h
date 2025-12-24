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
 * v1.0.8 | 2025-11-02
 * | - Added 'glyph_gl_set_opengl_version(int major, int minor)' for custom opengl versions.
 * | - Inlined some functions to avoid C++ errors.
 * v1.0.9 | 2025-11-14
 * | - Provided detailed comments on every aspect of the GlyphGL.
 * | - Renamed variables for readability and consistancy.
 * ========================================================
 */

/*
 * GlyphGL - A lightweight, header-only OpenGL text rendering library
 *
 * This library provides efficient text rendering capabilities using TrueType fonts,
 * atlas-based rendering, and OpenGL. It supports various text effects like bold,
 * italic, underline, and SDF (Signed Distance Field) rendering for scalable text.
 *
 * Key features:
 * - Zero-dependency GL loader with cross-platform support
 * - Custom TrueType parser and winding-rule rasterizer
 * - Atlas-based glyph storage for optimal texture usage
 * - Vertex batching and persistent buffers for high-performance rendering
 * - Configurable memory allocation macros
 * - Minimal mode for reduced memory footprint and simpler builds
 * - UTF-8 and ASCII character set support
 * - Built-in shader effects and custom effect support
 *
 * Usage overview:
 * 1. Create a renderer with glyph_renderer_create()
 * 2. Set projection matrix with glyph_renderer_set_projection()
 * 3. Render text with glyph_renderer_draw_text()
 * 4. Clean up with glyph_renderer_free()
 */

#ifndef __GLYPH_H
#define __GLYPH_H

/* Configurable atlas dimensions - can be overridden at compile time for memory optimization */
#ifndef GLYPHGL_ATLAS_WIDTH
#define GLYPHGL_ATLAS_WIDTH 2048  /* Default atlas width in pixels */
#endif
#ifndef GLYPHGL_ATLAS_HEIGHT
#define GLYPHGL_ATLAS_HEIGHT 2048  /* Default atlas height in pixels */
#endif
#ifndef GLYPHGL_VERTEX_BUFFER_SIZE
#define GLYPHGL_VERTEX_BUFFER_SIZE 73728  /* Default vertex buffer size (vertices) */
#endif


#include <stdlib.h>
#include "glyph_truetype.h"
#include "glyph_image.h"
#include "glyph_gl.h"
#include "glyph_util.h"
#ifndef GLYPHGL_MINIMAL
#include "glyph_effect.h"
#endif

/* Text styling bitmask flags - can be combined with bitwise OR */
#define GLYPHGL_BOLD        (1 << 0)  /* Render text with bold effect (duplicate glyphs offset) */
#define GLYPHGL_ITALIC      (1 << 1)  /* Apply italic shear transformation to glyphs */
#define GLYPHGL_UNDERLINE   (1 << 2)  /* Draw underline beneath text */
#define GLYPHGL_SDF         (1 << 3)  /* Enable Signed Distance Field rendering for scalable text */

#include "glyph_atlas.h"


/*
 * Default character sets for atlas generation
 * GLYPHGL_CHARSET_BASIC: Essential ASCII characters for basic text rendering
 * GLYPHGL_CHARSET_DEFAULT: Extended set including mathematical symbols and currency
 */
#define GLYPHGL_CHARSET_BASIC "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%%^&*()_+-=,./?| \n\t"
#define GLYPHGL_CHARSET_DEFAULT GLYPHGL_CHARSET_BASIC "€£¥¢₹₽±×÷√∫πΩ°∞≠≈≤≥∑∏∂∇∀∃∈∉⊂⊃∩∪←↑→↓"


/*
 * Forward declaration for UTF-8 decoding function used internally
 */
static int glyph_utf8_decode(const char* str, size_t* index);

/*
 * Main renderer structure containing all OpenGL resources and state for text rendering
 *
 * This struct encapsulates the complete state needed for rendering text with GlyphGL,
 * including the glyph atlas, OpenGL objects (texture, shader, VAO/VBO), vertex buffers
 * for batching, and cached uniform values for performance optimization.
 */
typedef struct {
    glyph_atlas_t atlas;                /* Glyph atlas containing pre-rasterized character data */
    GLuint texture;                     /* OpenGL texture object for atlas storage */
    GLuint shader;                      /* Compiled shader program for text rendering */
    GLuint vao;                         /* Vertex Array Object for vertex attribute setup */
    GLuint vbo;                         /* Vertex Buffer Object for batched vertex data */
    float* vertex_buffer;               /* CPU-side vertex buffer for batching glyph quads */
    size_t vertex_buffer_size;          /* Current allocated size of vertex buffer (in floats) */
    int initialized;                    /* Flag indicating if renderer was successfully created */
    glyph_encoding_type_t char_type;    /* Character encoding type (ASCII or UTF-8) */
    float cached_text_color[3];         /* Cached RGB color values to avoid redundant uniform updates */
    int cached_effects;                 /* Cached effects bitmask to avoid redundant uniform updates */
#ifndef GLYPHGL_MINIMAL
    glyph_effect_t effect;              /* Custom shader effect configuration (disabled in minimal mode) */
#endif
} glyph_renderer_t;


/*
 * Creates and initializes a new glyph renderer with the specified font and configuration
 *
 * This function performs the complete setup of a text renderer, including:
 * - Loading and parsing the TrueType font file
 * - Generating a glyph atlas with the specified character set
 * - Creating OpenGL texture, shader, and buffer objects
 * - Setting up vertex attributes for batched rendering
 * - Initializing performance caches for uniform values
 *
 * Parameters:
 *   font_path: Path to the TrueType (.ttf) font file
 *   pixel_height: Desired font size in pixels (affects glyph quality and atlas size)
 *   charset: String containing all characters to include in the atlas
 *   char_type: Character encoding (GLYPH_ENCODING_UTF8 or GLYPH_ENCODING_ASCII)
 *   effect: Pointer to glyph_effect_t struct for custom shaders (NULL for default)
 *   use_sdf: Enable SDF rendering (GLYPHGL_SDF flag) for scalable text
 *
 * Returns: Initialized glyph_renderer_t struct, or zero-initialized struct on failure
 *          Check renderer.initialized field to verify success
 */
static inline glyph_renderer_t glyph_renderer_create(const char* font_path, float pixel_height, const char* charset, glyph_encoding_type_t char_type, void* effect, int use_sdf) {
    /* Set up default effect if none provided (only in full mode) */
#ifndef GLYPHGL_MINIMAL
    glyph_effect_t default_effect = {(glyph_effect_type_t)GLYPH_EFFECT_NONE, NULL, NULL};
    if (effect == NULL) {
        effect = &default_effect;
    }
#endif

    /* Initialize renderer struct to zero */
    glyph_renderer_t renderer = {0};

    /* Load OpenGL function pointers - required for cross-platform compatibility */
    if (!glyph_gl_load_functions()) {
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Failed to load OpenGL functions\n");
        #endif
        return renderer;
    }

    /* Store character encoding type for text processing */
    renderer.char_type = char_type;

    /* Copy effect configuration (full mode only) */
#ifndef GLYPHGL_MINIMAL
    renderer.effect = *(glyph_effect_t*)effect;
#endif

    /* Generate glyph atlas from font file - this is the core text processing step */
    renderer.atlas = glyph_atlas_create(font_path, pixel_height, charset, char_type, use_sdf);
    if (!renderer.atlas.chars || !renderer.atlas.image.data) {
        #ifdef GLYPHGL_DEBUG
        GLYPH_LOG("Failed to create font atlas\n");
        #endif
        return renderer;
    }

    /* Create OpenGL texture for glyph atlas - different paths for minimal vs full mode */
    // Defer atlas channel copy for minimal builds - upload directly from RGB data
#ifndef GLYPHGL_MINIMAL
    /* Full mode: Extract red channel from RGB atlas for GL_RED texture format */
    /* This reduces texture memory usage and is optimal for luminance-based alpha */
    unsigned char* red_channel = (unsigned char*)GLYPH_MALLOC(renderer.atlas.image.width * renderer.atlas.image.height);
    if (!red_channel) {
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }

    /* Copy red channel from RGB data (glyph rasterization produces grayscale) */
    for (unsigned int i = 0; i < renderer.atlas.image.width * renderer.atlas.image.height; i++) {
        red_channel[i] = renderer.atlas.image.data[i * 3];
    }

    /* Create and configure OpenGL texture with red channel data */
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
    /* Minimal mode: Upload RGB texture directly (no channel extraction) */
    /* Simpler but uses more memory - suitable for basic rendering without effects */
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

    /* Create shader program - use custom effect shaders or default based on configuration */
#ifndef GLYPHGL_MINIMAL
    if (renderer.effect.type == GLYPH_EFFECT_NONE) {
        /* Use default shaders for basic text rendering */
        renderer.shader = glyph__create_program(glyph__get_vertex_shader_source_cached(), glyph__get_fragment_shader_source_cached());
    } else {
        /* Use custom effect shaders for advanced rendering features */
        renderer.shader = glyph__create_program(renderer.effect.vertex_shader, renderer.effect.fragment_shader);
    }
#else
    /* Minimal mode always uses default shaders */
    renderer.shader = glyph__create_program(glyph__get_vertex_shader_source_cached(), glyph__get_fragment_shader_source_cached());
#endif
    if (!renderer.shader) {
        /* Cleanup on shader compilation failure */
        glDeleteTextures(1, &renderer.texture);
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }

    /* Set up OpenGL vertex array and buffer objects for batched rendering */
    glyph__glGenVertexArrays(1, &renderer.vao);
    glyph__glGenBuffers(1, &renderer.vbo);
    glyph__glBindVertexArray(renderer.vao);
    glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    /* Allocate GPU buffer for batched vertex data - will be updated each draw call */
    glyph__glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GLYPHGL_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
    /* Configure vertex attributes: position (vec2) and texture coords (vec2) */
    glyph__glEnableVertexAttribArray(0);
    glyph__glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glyph__glEnableVertexAttribArray(1);
    glyph__glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);
    glyph__glBindVertexArray(0);

    /* Allocate CPU-side vertex buffer for batching glyph quads before GPU upload */
    renderer.vertex_buffer_size = GLYPHGL_VERTEX_BUFFER_SIZE * 4; /* Initial size for vertices (float * 4 per vertex) */
    renderer.vertex_buffer = (float*)GLYPH_MALLOC(sizeof(float) * renderer.vertex_buffer_size);
    if (!renderer.vertex_buffer) {
        /* Cleanup on memory allocation failure */
        glyph__glDeleteVertexArrays(1, &renderer.vao);
        glyph__glDeleteBuffers(1, &renderer.vbo);
        glDeleteTextures(1, &renderer.texture);
        glyph__glDeleteProgram(renderer.shader);
        glyph_atlas_free(&renderer.atlas);
        return renderer;
    }

    /* Initialize uniform caches to invalid values to force first update */
    renderer.cached_text_color[0] = -1.0f;
    renderer.cached_text_color[1] = -1.0f;
    renderer.cached_text_color[2] = -1.0f;
    renderer.cached_effects = -1;

    /* Mark renderer as successfully initialized */
    renderer.initialized = 1;
    return renderer;
}

/*
 * Frees all resources associated with a glyph renderer
 *
 * This function performs complete cleanup of OpenGL objects, memory buffers,
 * and atlas data. It should be called when the renderer is no longer needed
 * to prevent memory leaks and resource exhaustion.
 *
 * Parameters:
 *   renderer: Pointer to the glyph_renderer_t to free
 *
 * Note: Safe to call on uninitialized or NULL renderers (no-op in these cases)
 */
static inline void glyph_renderer_free(glyph_renderer_t* renderer) {
    /* Early return for invalid or uninitialized renderers */
    if (!renderer || !renderer->initialized) return;

    /* Clean up OpenGL objects in reverse order of creation */
    glyph__glDeleteVertexArrays(1, &renderer->vao);
    glyph__glDeleteBuffers(1, &renderer->vbo);
    glDeleteTextures(1, &renderer->texture);
    glyph__glDeleteProgram(renderer->shader);

    /* Free glyph atlas and its associated memory */
    glyph_atlas_free(&renderer->atlas);

    /* Free CPU-side vertex buffer */
    GLYPH_FREE(renderer->vertex_buffer);

    /* Mark renderer as uninitialized to prevent double-free */
    renderer->initialized = 0;
}

/*
 * Sets the orthographic projection matrix for 2D text rendering
 *
 * Configures the coordinate system to match window dimensions, mapping
 * pixel coordinates directly to screen space. This is essential for
 * proper text positioning and scaling.
 *
 * Parameters:
 *   renderer: Pointer to initialized glyph renderer
 *   width: Window/screen width in pixels
 *   height: Window/screen height in pixels
 *
 * The projection matrix transforms coordinates from (0,0) at bottom-left
 * to (width,height) at top-right, which is standard for 2D GUI rendering.
 */
static inline void glyph_renderer_set_projection(glyph_renderer_t* renderer, int width, int height) {
    if (!renderer || !renderer->initialized) return;

    /* Create orthographic projection matrix for pixel-perfect 2D rendering */
    float projection[16] = {
        2.0f / width, 0.0f, 0.0f, 0.0f,      /* Scale X: map width to [-1,1] range */
        0.0f, -2.0f / height, 0.0f, 0.0f,     /* Scale Y: map height to [-1,1] range (flipped) */
        0.0f, 0.0f, -1.0f, 0.0f,             /* Z: standard orthographic */
        -1.0f, 1.0f, 0.0f, 1.0f              /* Translation: center at (0,0) */
    };

    /* Upload projection matrix to shader uniform */
    glyph__glUseProgram(renderer->shader);
    glyph__glUniformMatrix4fv(glyph__glGetUniformLocation(renderer->shader, "projection"), 1, GL_FALSE, projection);
    glyph__glUseProgram(0);
}

/*
 * Updates the projection matrix when window dimensions change
 *
 * This is a convenience function identical to glyph_renderer_set_projection,
 * provided for handling window resize events. Call this whenever the
 * rendering window is resized to maintain correct text positioning.
 *
 * Parameters:
 *   renderer: Pointer to initialized glyph renderer
 *   width: New window width in pixels
 *   height: New window height in pixels
 *
 * Note: This function is functionally identical to set_projection but
 *       named for semantic clarity during window resize handling.
 */
static inline void glyph_renderer_update_projection(glyph_renderer_t* renderer, int width, int height) {
    if (!renderer || !renderer->initialized) return;

    /* Recalculate orthographic projection for new dimensions */
    float projection[16] = {
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    /* Update shader uniform with new projection matrix */
    glyph__glUseProgram(renderer->shader);
    glyph__glUniformMatrix4fv(glyph__glGetUniformLocation(renderer->shader, "projection"), 1, GL_FALSE, projection);
    glyph__glUseProgram(0);
}

/*
 * Renders text to the screen with specified styling and effects
 *
 * This is the core rendering function that processes text strings, looks up
 * glyph data from the atlas, applies text effects, and batches everything
 * into a single OpenGL draw call for optimal performance.
 *
 * Parameters:
 *   renderer: Pointer to initialized glyph renderer
 *   text: UTF-8 or ASCII string to render
 *   x, y: Screen coordinates for text baseline start position
 *   scale: Text scaling factor (1.0 = normal size)
 *   r, g, b: Text color as RGB values (0.0-1.0 range)
 *   effects: Bitmask of text effects (GLYPHGL_BOLD, GLYPHGL_ITALIC, etc.)
 *
 * Performance features:
 * - Vertex batching: All glyphs rendered in single draw call
 * - Uniform caching: Only updates shader uniforms when values change
 * - Dynamic buffer growth: Expands vertex buffer as needed
 * - Effect stacking: Multiple effects can be applied simultaneously
 */
static inline void glyph_renderer_draw_text(glyph_renderer_t* renderer, const char* text, float x, float y, float scale,
                                  float r, float g, float b, int effects) {
    /* Validate renderer state */
    if (!renderer || !renderer->initialized) return;

    /* Bind shader program and OpenGL state for rendering */
    glyph__glUseProgram(renderer->shader);
    glyph__glBindVertexArray(renderer->vao);
    glyph__glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->texture);

    /* Performance optimization: Only update uniforms if values have changed */
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

    /* Calculate text length and estimate vertex buffer requirements */
    size_t text_len = strlen(text);
    /* Conservative estimate: 24 floats per glyph * 3 for max effects (normal + bold + underline) */
    size_t required_size = sizeof(float) * 24 * text_len * 3;
    if (required_size > renderer->vertex_buffer_size) {
        /* Grow vertex buffer dynamically to accommodate text */
        size_t new_size = required_size * 2; /* Double size to minimize future reallocations */
        float* new_buffer = (float*)GLYPH_REALLOC(renderer->vertex_buffer, new_size);
        if (!new_buffer) return; /* Memory allocation failure - skip rendering */
        renderer->vertex_buffer = new_buffer;
        renderer->vertex_buffer_size = new_size;
    }
    float* vertices = renderer->vertex_buffer;
    size_t vertex_count = 0;

    /* Process each character in the text string */
    float current_x = x; /* Track horizontal position for kerning */
    size_t i = 0;
    while (i < text_len) {
        /* Decode next character based on encoding type */
        int codepoint;
        if (renderer->char_type == GLYPH_ENCODING_UTF8) {
            codepoint = glyph_utf8_decode(text, &i); /* Handle multi-byte UTF-8 sequences */
        } else {
            codepoint = (unsigned char)text[i]; /* Simple ASCII byte */
            i++;
        }

        /* Look up glyph data in atlas */
        glyph_atlas_char_t* ch = glyph_atlas_find_char(&renderer->atlas, codepoint);
        if (!ch) {
            /* Fallback to question mark for missing characters */
            ch = glyph_atlas_find_char(&renderer->atlas, '?');
        }
        if (!ch || ch->width == 0) {
            /* Skip invalid/missing glyphs, advance cursor */
            current_x += ch ? ch->advance * scale : (renderer->atlas.pixel_height * 0.5f * scale);
            continue;
        }

        /* Calculate glyph quad position and size in screen space */
        float xpos = current_x + ch->xoff * scale; /* Apply left bearing offset */
        float ypos = y - ch->yoff * scale;         /* Apply baseline offset (inverted Y) */
        float w = ch->width * scale;               /* Scaled glyph width */
        float h = ch->height * scale;              /* Scaled glyph height */

        /* Calculate texture coordinates for glyph in atlas */
        float tex_x1 = (float)ch->x / renderer->atlas.image.width;
        float tex_y1 = (float)ch->y / renderer->atlas.image.height;
        float tex_x2 = (float)(ch->x + ch->width) / renderer->atlas.image.width;
        float tex_y2 = (float)(ch->y + ch->height) / renderer->atlas.image.height;

        /* Build vertex data for glyph quad (two triangles = 6 vertices) */
        /* Format: [pos_x, pos_y, tex_u, tex_v] per vertex */
        float glyph_vertices[24] = {
            /* Triangle 1 */
            xpos,     ypos + h,   tex_x1, tex_y2,  /* Top-left */
            xpos,     ypos,       tex_x1, tex_y1,  /* Bottom-left */
            xpos + w, ypos,       tex_x2, tex_y1,  /* Bottom-right */

            /* Triangle 2 */
            xpos,     ypos + h,   tex_x1, tex_y2,  /* Top-left */
            xpos + w, ypos,       tex_x2, tex_y1,  /* Bottom-right */
            xpos + w, ypos + h,   tex_x2, tex_y2   /* Top-right */
        };

        /* Apply italic effect by shearing glyph vertices */
#ifndef GLYPHGL_MINIMAL
        if (effects & GLYPHGL_ITALIC) {
            float shear = 0.2f; /* Shear factor for italic slant */
            /* Apply shear to top vertices of both triangles */
            glyph_vertices[0] -= shear * h;   /* Triangle 1 top-left X */
            glyph_vertices[12] -= shear * h;  /* Triangle 2 top-left X */
            glyph_vertices[20] -= shear * h;  /* Triangle 2 top-right X */
        }
#endif

        /* Copy base glyph vertices to batch buffer */
        memcpy(vertices + vertex_count * 4, glyph_vertices, sizeof(glyph_vertices));
        vertex_count += 6;

        /* Render additional geometry for text effects */
#ifndef GLYPHGL_MINIMAL
        if (effects & GLYPHGL_BOLD) {
            /* Create bold effect by rendering duplicate glyph with offset */
            float bold_offset = 1.0f * scale; /* Pixel offset for bold thickness */
            float bold_vertices[24] = {
                /* Offset duplicate of base glyph */
                xpos + bold_offset,     ypos + h,   tex_x1, tex_y2,
                xpos + bold_offset,     ypos,       tex_x1, tex_y1,
                xpos + w + bold_offset, ypos,       tex_x2, tex_y1,

                xpos + bold_offset,     ypos + h,   tex_x1, tex_y2,
                xpos + w + bold_offset, ypos,       tex_x2, tex_y1,
                xpos + w + bold_offset, ypos + h,   tex_x2, tex_y2
            };

            /* Apply italic shear to bold glyph if both effects active */
            if (effects & GLYPHGL_ITALIC) {
                float shear = 0.2f;
                bold_vertices[0] -= shear * h;
                bold_vertices[12] -= shear * h;
                bold_vertices[20] -= shear * h;
            }

            /* Add bold glyph vertices to batch */
            memcpy(vertices + vertex_count * 4, bold_vertices, sizeof(bold_vertices));
            vertex_count += 6;
        }

        if (effects & GLYPHGL_UNDERLINE) {
            /* Render underline as a thin quad beneath the text */
            float underline_y = y + h * 0.1f; /* Position slightly below baseline */
            float underline_vertices[24] = {
                /* Horizontal line quad spanning glyph advance width */
                current_x, underline_y + 2, 0.0f, 0.0f,     /* Top-left of line */
                current_x, underline_y,     0.0f, 0.0f,     /* Bottom-left of line */
                current_x + ch->advance * scale, underline_y,     0.0f, 0.0f, /* Bottom-right */

                current_x, underline_y + 2, 0.0f, 0.0f,     /* Top-left */
                current_x + ch->advance * scale, underline_y,     0.0f, 0.0f, /* Bottom-right */
                current_x + ch->advance * scale, underline_y + 2, 0.0f, 0.0f  /* Top-right */
            };
            /* Add underline vertices to batch */
            memcpy(vertices + vertex_count * 4, underline_vertices, sizeof(underline_vertices));
            vertex_count += 6;
        }
#endif

        /* Advance cursor to next character position */
        current_x += ch->advance * scale;
    }

    /* Upload batched vertex data to GPU and execute draw call */
    glyph__glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glyph__glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * 4 * sizeof(float), vertices);
    glyph__glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Render all batched glyphs in single draw call - highly efficient! */
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    /* Clean up OpenGL state */
    glyph__glBindVertexArray(0);
    glyph__glUseProgram(0);
}

/*
 * Returns the OpenGL Vertex Array Object handle for advanced rendering control
 *
 * This getter provides access to the internal VAO for users who need to
 * integrate GlyphGL rendering with their own OpenGL state management or
 * custom rendering pipelines.
 *
 * Parameters:
 *   renderer: Pointer to initialized glyph renderer
 *
 * Returns: OpenGL VAO handle (GLuint), or 0 if renderer invalid
 */
static inline GLuint glyph_renderer_get_vao(glyph_renderer_t* renderer) {
    return renderer->vao;
}

/*
 * Returns the OpenGL Vertex Buffer Object handle for advanced rendering control
 *
 * Provides access to the internal VBO containing batched vertex data.
 * Useful for debugging, custom batching, or integration with external
 * rendering systems.
 *
 * Parameters:
 *   renderer: Pointer to initialized glyph renderer
 *
 * Returns: OpenGL VBO handle (GLuint), or 0 if renderer invalid
 */
static inline GLuint glyph_renderer_get_vbo(glyph_renderer_t* renderer) {
    return renderer->vbo;
}

/*
 * Returns the OpenGL shader program handle for advanced shader management
 *
 * Allows access to the compiled shader program for custom uniform setting,
 * shader introspection, or integration with effect systems that need
 * to modify shader state directly.
 *
 * Parameters:
 *   renderer: Pointer to initialized glyph renderer
 *
 * Returns: OpenGL shader program handle (GLuint), or 0 if renderer invalid
 */
static inline GLuint glyph_renderer_get_shader(glyph_renderer_t* renderer) {
    return renderer->shader;
}

/*
 * Decodes a single UTF-8 codepoint from a string and advances the index
 *
 * This function implements UTF-8 decoding according to RFC 3629, handling
 * 1-4 byte sequences correctly. It advances the index past the decoded
 * character and returns the Unicode codepoint value.
 *
 * Parameters:
 *   str: UTF-8 encoded string to decode from
 *   index: Pointer to current position in string (updated on return)
 *
 * Returns:
 *   Unicode codepoint (int), or 0xFFFD (replacement character) on error
 *   Returns 0 if end of string reached
 *
 * UTF-8 encoding:
 * - 1 byte:  0xxxxxxx (ASCII, 0-127)
 * - 2 bytes: 110xxxxx 10xxxxxx (128-2047)
 * - 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx (2048-65535)
 * - 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (65536-1114111)
 */
static inline int glyph_utf8_decode(const char* str, size_t* index) {
    size_t i = *index;
    /* Check for end of string */
    if (i >= strlen(str)) {
        return 0;
    }

    unsigned char c = (unsigned char)str[i++];
    /* Handle 1-byte ASCII characters (most common case) */
    if (c < 0x80) {
        *index = i;
        return c;
    }
    /* Handle 2-byte sequences (Latin-1 supplement, etc.) */
    else if ((c & 0xE0) == 0xC0) {
        if (i >= strlen(str)) return 0xFFFD; /* Incomplete sequence */
        unsigned char c2 = (unsigned char)str[i++];
        if ((c2 & 0xC0) != 0x80) return 0xFFFD; /* Invalid continuation byte */
        *index = i;
        return ((c & 0x1F) << 6) | (c2 & 0x3F); /* Decode 11-bit codepoint */
    }
    /* Handle 3-byte sequences (Basic Multilingual Plane) */
    else if ((c & 0xF0) == 0xE0) {
        if (i + 1 >= strlen(str)) return 0xFFFD; /* Incomplete sequence */
        unsigned char c2 = (unsigned char)str[i++];
        unsigned char c3 = (unsigned char)str[i++];
        if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return 0xFFFD; /* Invalid continuation */
        *index = i;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F); /* Decode 16-bit codepoint */
    }
    /* Handle 4-byte sequences (astral planes, emojis, etc.) */
    /* Its still really buggy right now */
    else if ((c & 0xF8) == 0xF0) {
        if (i + 2 >= strlen(str)) return 0xFFFD; /* Incomplete sequence */
        unsigned char c2 = (unsigned char)str[i++];
        unsigned char c3 = (unsigned char)str[i++];
        unsigned char c4 = (unsigned char)str[i++];
        if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) return 0xFFFD; /* Invalid continuation */
        *index = i;
        return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F); /* Decode 21-bit codepoint */
    }
    /* Invalid UTF-8 lead byte */
    *index = i;
    return 0xFFFD; /* Unicode replacement character */
}

#endif