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
 * OpenGL Abstraction Layer for GlyphGL
 *
 * This module provides cross-platform OpenGL function loading and shader
 * management. It handles the complexities of OpenGL extension loading across
 * different platforms (Windows, macOS, Linux) and provides a unified interface
 * for shader compilation and program creation.
 *
 * Key features:
 * - Cross-platform OpenGL function loading (WGL, GLX, Cocoa)
 * - GLSL shader compilation and program linking
 * - Built-in vertex and fragment shaders for text rendering
 * - Configurable GLSL version support
 * - Minimal mode support for reduced functionality
 */

#ifndef __GLYPH_GL_H
#define __GLYPH_GL_H

/* Platform-specific OpenGL headers */
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <GL/gl.h>  /* Windows OpenGL headers */
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #include <OpenGL/gl.h>  /* macOS OpenGL headers */
    #endif
#elif defined(__linux__) || defined(__unix__)
    #include <GL/gl.h>  /* Linux/Unix OpenGL headers */
#endif

/* Standard C libraries for memory and string operations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "glyph_util.h"  /* Utility functions and macros */

/* OpenGL function loading and type definitions (when loader is enabled) */
#ifndef GLYPH_NO_GL_LOADER

/* OpenGL type definitions for cross-platform compatibility */
typedef char GLchar;           /* Character type for shader source */
typedef ptrdiff_t GLsizeiptr;  /* Size type for buffer operations */
typedef ptrdiff_t GLintptr;    /* Pointer offset type */

/* OpenGL constants - defined here if not available in system headers */
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892  /* Vertex buffer target */
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893  /* Index buffer target */
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4  /* Buffer usage: rarely modified */
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8  /* Buffer usage: frequently modified */
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30  /* Fragment shader type */
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31  /* Vertex shader type */
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81  /* Shader compilation status */
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82  /* Program linking status */
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84  /* Info log length query */
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0  /* First texture unit */
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F  /* Texture wrap mode */
#endif
#ifndef GL_RED
#define GL_RED 0x1903  /* Red color channel */
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006  /* Blend equation: add */
#endif

/* Function pointer typedefs for OpenGL extension functions */
/* Buffer management functions */
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (*PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);

/* Shader management functions */
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum type);
typedef void (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

/* Program management functions */
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint program);

/* Attribute and uniform functions */
typedef GLint (*PFNGLGETATTRIBLOCATIONPROC)(GLuint program, const GLchar *name);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (*PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);

/* Uniform setting functions */
typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (*PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (*PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (*PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (*PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

/* Texture functions */
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (*PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
typedef void (*PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef void (*PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef void (*PFNGLPIXELSTOREIPROC)(GLenum pname, GLint param);
typedef void (*PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (*PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);

/* Drawing functions */
typedef void (*PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
typedef void (*PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*PFNGLENABLEPROC)(GLenum cap);
typedef void (*PFNGLBLENDFUNCPROC)(GLenum sfactor, GLenum dfactor);
typedef void (*PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*PFNGLCLEARPROC)(GLbitfield mask);

/* VAO functions */
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);

/* Static function pointers for loaded OpenGL functions */
/* Buffer management */
static PFNGLGENBUFFERSPROC glyph__glGenBuffers;
static PFNGLDELETEBUFFERSPROC glyph__glDeleteBuffers;
static PFNGLBINDBUFFERPROC glyph__glBindBuffer;
static PFNGLBUFFERDATAPROC glyph__glBufferData;
static PFNGLBUFFERSUBDATAPROC glyph__glBufferSubData;

/* Shader management */
static PFNGLCREATESHADERPROC glyph__glCreateShader;
static PFNGLDELETESHADERPROC glyph__glDeleteShader;
static PFNGLSHADERSOURCEPROC glyph__glShaderSource;
static PFNGLCOMPILESHADERPROC glyph__glCompileShader;
static PFNGLGETSHADERIVPROC glyph__glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glyph__glGetShaderInfoLog;

/* Program management */
static PFNGLCREATEPROGRAMPROC glyph__glCreateProgram;
static PFNGLDELETEPROGRAMPROC glyph__glDeleteProgram;
static PFNGLATTACHSHADERPROC glyph__glAttachShader;
static PFNGLLINKPROGRAMPROC glyph__glLinkProgram;
static PFNGLGETPROGRAMIVPROC glyph__glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glyph__glGetProgramInfoLog;
static PFNGLUSEPROGRAMPROC glyph__glUseProgram;

/* Attributes and uniforms */
static PFNGLGETATTRIBLOCATIONPROC glyph__glGetAttribLocation;
static PFNGLGETUNIFORMLOCATIONPROC glyph__glGetUniformLocation;
static PFNGLVERTEXATTRIBPOINTERPROC glyph__glVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glyph__glEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glyph__glDisableVertexAttribArray;

/* Uniform setting */
static PFNGLUNIFORM1IPROC glyph__glUniform1i;
static PFNGLUNIFORM1FPROC glyph__glUniform1f;
static PFNGLUNIFORM2FPROC glyph__glUniform2f;
static PFNGLUNIFORM3FPROC glyph__glUniform3f;
static PFNGLUNIFORM4FPROC glyph__glUniform4f;
static PFNGLUNIFORMMATRIX4FVPROC glyph__glUniformMatrix4fv;

/* Textures */
static PFNGLACTIVETEXTUREPROC glyph__glActiveTexture;
static PFNGLGENTEXTURESPROC glyph__glGenTextures;
static PFNGLDELETETEXTURESPROC glyph__glDeleteTextures;
static PFNGLBINDTEXTUREPROC glyph__glBindTexture;
static PFNGLPIXELSTOREIPROC glyph__glPixelStorei;
static PFNGLTEXIMAGE2DPROC glyph__glTexImage2D;
static PFNGLTEXPARAMETERIPROC glyph__glTexParameteri;

/* Drawing */
static PFNGLDRAWARRAYSPROC glyph__glDrawArrays;
static PFNGLVIEWPORTPROC glyph__glViewport;
static PFNGLENABLEPROC glyph__glEnable;
static PFNGLBLENDFUNCPROC glyph__glBlendFunc;
static PFNGLCLEARCOLORPROC glyph__glClearColor;
static PFNGLCLEARPROC glyph__glClear;

/* VAOs */
static PFNGLGENVERTEXARRAYSPROC glyph__glGenVertexArrays;
static PFNGLDELETEVERTEXARRAYSPROC glyph__glDeleteVertexArrays;
static PFNGLBINDVERTEXARRAYPROC glyph__glBindVertexArray;

#if defined(_WIN32) || defined(_WIN64)
    static HMODULE glyph__opengl_dll = NULL;
    #define GLYPH_GL_LOAD_PROC(type, name) \
        glyph__##name = (type)wglGetProcAddress(#name); \
        if (!glyph__##name) { \
            if (!glyph__opengl_dll) { \
                glyph__opengl_dll = LoadLibraryA("opengl32.dll"); \
            } \
            if (glyph__opengl_dll) { \
                glyph__##name = (type)GetProcAddress(glyph__opengl_dll, #name); \
            } \
        } \
        if (!glyph__##name) { \
            GLYPH_LOG("Failed to load OpenGL function: %s\n", #name); \
            return 0; \
        }
#elif defined(__APPLE__)
    #include <dlfcn.h>
    static void* glyph__opengl_handle = NULL;
    #define GLYPH_GL_LOAD_PROC(type, name) \
        if (!glyph__opengl_handle) { \
            glyph__opengl_handle = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY | RTLD_GLOBAL); \
        } \
        if (glyph__opengl_handle) { \
            glyph__##name = (type)dlsym(glyph__opengl_handle, #name); \
        } \
        if (!glyph__##name) { \
            GLYPH_LOG("Failed to load OpenGL function: %s\n", #name); \
            return 0; \
        }
#elif defined(__linux__) || defined(__unix__)
    #include <dlfcn.h>
    static void* glyph__libgl_handle = NULL;
    #define GLYPH_GL_LOAD_PROC(type, name) \
        if (!glyph__libgl_handle) { \
            glyph__libgl_handle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL); \
            if (!glyph__libgl_handle) glyph__libgl_handle = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL); \
        } \
        if (glyph__libgl_handle) { \
            typedef void* (*glXGetProcAddressARB_t)(const GLubyte*); \
            glXGetProcAddressARB_t glXGetProcAddressARB = (glXGetProcAddressARB_t)dlsym(glyph__libgl_handle, "glXGetProcAddressARB"); \
            if (glXGetProcAddressARB) { \
                glyph__##name = (type)glXGetProcAddressARB((const GLubyte*)#name); \
            } \
            if (!glyph__##name) glyph__##name = (type)dlsym(glyph__libgl_handle, #name); \
        } \
        if (!glyph__##name) { \
            GLYPH_LOG("Failed to load OpenGL function: %s\n", #name); \
            return 0; \
        }
#endif

/*
 * Loads all required OpenGL extension functions for the current platform
 *
 * This function dynamically loads OpenGL functions that may not be available
 * in the standard headers, using platform-specific extension loading mechanisms:
 * - Windows: wglGetProcAddress
 * - Linux/Unix: glXGetProcAddressARB or dlsym
 * - macOS: dlsym (OpenGL functions are available in system frameworks)
 *
 * Returns: 1 on success, 0 on failure (with error logging)
 */
static int glyph_gl_load_functions(void) {
    /* Load buffer management functions */
    GLYPH_GL_LOAD_PROC(PFNGLGENBUFFERSPROC, glGenBuffers);
    GLYPH_GL_LOAD_PROC(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    GLYPH_GL_LOAD_PROC(PFNGLBINDBUFFERPROC, glBindBuffer);
    GLYPH_GL_LOAD_PROC(PFNGLBUFFERDATAPROC, glBufferData);
    GLYPH_GL_LOAD_PROC(PFNGLBUFFERSUBDATAPROC, glBufferSubData);

    /* Load shader management functions */
    GLYPH_GL_LOAD_PROC(PFNGLCREATESHADERPROC, glCreateShader);
    GLYPH_GL_LOAD_PROC(PFNGLDELETESHADERPROC, glDeleteShader);
    GLYPH_GL_LOAD_PROC(PFNGLSHADERSOURCEPROC, glShaderSource);
    GLYPH_GL_LOAD_PROC(PFNGLCOMPILESHADERPROC, glCompileShader);
    GLYPH_GL_LOAD_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv);
    GLYPH_GL_LOAD_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);

    /* Load program management functions */
    GLYPH_GL_LOAD_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    GLYPH_GL_LOAD_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    GLYPH_GL_LOAD_PROC(PFNGLATTACHSHADERPROC, glAttachShader);
    GLYPH_GL_LOAD_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram);
    GLYPH_GL_LOAD_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    GLYPH_GL_LOAD_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    GLYPH_GL_LOAD_PROC(PFNGLUSEPROGRAMPROC, glUseProgram);

    /* Load attribute and uniform functions */
    GLYPH_GL_LOAD_PROC(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
    GLYPH_GL_LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    GLYPH_GL_LOAD_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    GLYPH_GL_LOAD_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    GLYPH_GL_LOAD_PROC(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);

    /* Load uniform setting functions */
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM1IPROC, glUniform1i);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM1FPROC, glUniform1f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM2FPROC, glUniform2f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM3FPROC, glUniform3f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM4FPROC, glUniform4f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);

    /* Load texture functions */
    GLYPH_GL_LOAD_PROC(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    GLYPH_GL_LOAD_PROC(PFNGLGENTEXTURESPROC, glGenTextures);
    GLYPH_GL_LOAD_PROC(PFNGLDELETETEXTURESPROC, glDeleteTextures);
    GLYPH_GL_LOAD_PROC(PFNGLBINDTEXTUREPROC, glBindTexture);
    GLYPH_GL_LOAD_PROC(PFNGLPIXELSTOREIPROC, glPixelStorei);
    GLYPH_GL_LOAD_PROC(PFNGLTEXIMAGE2DPROC, glTexImage2D);
    GLYPH_GL_LOAD_PROC(PFNGLTEXPARAMETERIPROC, glTexParameteri);

    /* Load drawing functions */
    GLYPH_GL_LOAD_PROC(PFNGLDRAWARRAYSPROC, glDrawArrays);
    GLYPH_GL_LOAD_PROC(PFNGLVIEWPORTPROC, glViewport);
    GLYPH_GL_LOAD_PROC(PFNGLENABLEPROC, glEnable);
    GLYPH_GL_LOAD_PROC(PFNGLBLENDFUNCPROC, glBlendFunc);
    GLYPH_GL_LOAD_PROC(PFNGLCLEARCOLORPROC, glClearColor);
    GLYPH_GL_LOAD_PROC(PFNGLCLEARPROC, glClear);

    /* Load VAO functions */
    GLYPH_GL_LOAD_PROC(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    GLYPH_GL_LOAD_PROC(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
    GLYPH_GL_LOAD_PROC(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);

    return 1; /* Success - all functions loaded */
}

/* Define macros to map standard OpenGL function names to loaded function pointers */
#define glGenBuffers glyph__glGenBuffers
#define glDeleteBuffers glyph__glDeleteBuffers
#define glBindBuffer glyph__glBindBuffer
#define glBufferData glyph__glBufferData
#define glBufferSubData glyph__glBufferSubData
#define glCreateShader glyph__glCreateShader
#define glDeleteShader glyph__glDeleteShader
#define glShaderSource glyph__glShaderSource
#define glCompileShader glyph__glCompileShader
#define glGetShaderiv glyph__glGetShaderiv
#define glGetShaderInfoLog glyph__glGetShaderInfoLog
#define glCreateProgram glyph__glCreateProgram
#define glDeleteProgram glyph__glDeleteProgram
#define glAttachShader glyph__glAttachShader
#define glLinkProgram glyph__glLinkProgram
#define glGetProgramiv glyph__glGetProgramiv
#define glGetProgramInfoLog glyph__glGetProgramInfoLog
#define glUseProgram glyph__glUseProgram
#define glGetAttribLocation glyph__glGetAttribLocation
#define glGetUniformLocation glyph__glGetUniformLocation
#define glVertexAttribPointer glyph__glVertexAttribPointer
#define glEnableVertexAttribArray glyph__glEnableVertexAttribArray
#define glDisableVertexAttribArray glyph__glDisableVertexAttribArray
#define glUniform1i glyph__glUniform1i
#define glUniform1f glyph__glUniform1f
#define glUniform2f glyph__glUniform2f
#define glUniform3f glyph__glUniform3f
#define glUniform4f glyph__glUniform4f
#define glUniformMatrix4fv glyph__glUniformMatrix4fv
#define glActiveTexture glyph__glActiveTexture
#define glGenTextures glyph__glGenTextures
#define glDeleteTextures glyph__glDeleteTextures
#define glBindTexture glyph__glBindTexture
#define glPixelStorei glyph__glPixelStorei
#define glTexImage2D glyph__glTexImage2D
#define glTexParameteri glyph__glTexParameteri
#define glGenVertexArrays glyph__glGenVertexArrays
#define glDeleteVertexArrays glyph__glDeleteVertexArrays
#define glBindVertexArray glyph__glBindVertexArray
#define glDrawArrays glyph__glDrawArrays
#define glViewport glyph__glViewport
#define glEnable glyph__glEnable
#define glBlendFunc glyph__glBlendFunc
#define glClearColor glyph__glClearColor
#define glClear glyph__glClear

#else

#define glyph__glGenBuffers glGenBuffers
#define glyph__glDeleteBuffers glDeleteBuffers
#define glyph__glBindBuffer glBindBuffer
#define glyph__glBufferData glBufferData
#define glyph__glBufferSubData glBufferSubData
#define glyph__glCreateShader glCreateShader
#define glyph__glDeleteShader glDeleteShader
#define glyph__glShaderSource glShaderSource
#define glyph__glCompileShader glCompileShader
#define glyph__glGetShaderiv glGetShaderiv
#define glyph__glGetShaderInfoLog glGetShaderInfoLog
#define glyph__glCreateProgram glCreateProgram
#define glyph__glDeleteProgram glDeleteProgram
#define glyph__glAttachShader glAttachShader
#define glyph__glLinkProgram glLinkProgram
#define glyph__glGetProgramiv glGetProgramiv
#define glyph__glGetProgramInfoLog glGetProgramInfoLog
#define glyph__glUseProgram glUseProgram
#define glyph__glGetAttribLocation glGetAttribLocation
#define glyph__glGetUniformLocation glGetUniformLocation
#define glyph__glVertexAttribPointer glVertexAttribPointer
#define glyph__glEnableVertexAttribArray glEnableVertexAttribArray
#define glyph__glDisableVertexAttribArray glDisableVertexAttribArray
#define glyph__glUniform1i glUniform1i
#define glyph__glUniform1f glUniform1f
#define glyph__glUniform2f glUniform2f
#define glyph__glUniform3f glUniform3f
#define glyph__glUniform4f glUniform4f
#define glyph__glUniformMatrix4fv glUniformMatrix4fv
#define glyph__glActiveTexture glActiveTexture
#define glyph__glGenVertexArrays glGenVertexArrays
#define glyph__glDeleteVertexArrays glDeleteVertexArrays
#define glyph__glBindVertexArray glBindVertexArray

static int glyph_gl_load_functions(void) {
    return 1;
}

#endif
/* GLSL version string for shader compilation - defaults to OpenGL 3.3 core */
static char glyph_glsl_version_str[32] = "#version 330 core\n";

/*
 * Sets the GLSL version string for shader compilation
 *
 * Allows customization of the OpenGL version used in shader #version directives.
 * This is useful for targeting different OpenGL versions or compatibility profiles.
 *
 * Parameters:
 *   major: Major OpenGL version (e.g., 3 for OpenGL 3.x)
 *   minor: Minor OpenGL version (e.g., 3 for OpenGL 3.3)
 *
 * Example: glyph_gl_set_opengl_version(4, 1) sets "#version 410 core\n"
 */
static inline void glyph_gl_set_opengl_version(int major, int minor) {
    sprintf(glyph_glsl_version_str, "#version %d%d0 core\n", major, minor);
}
/* Built-in vertex shader source for text rendering */
/* Transforms vertex positions and passes texture coordinates to fragment shader */
static const char* glyph__vertex_shader_body =
"layout (location = 0) in vec2 aPos;\n"           /* Vertex position input */
"layout (location = 1) in vec2 aTexCoord;\n"       /* Texture coordinate input */
"out vec2 TexCoord;\n"                             /* Output to fragment shader */
"uniform mat4 projection;\n"                       /* Projection matrix uniform */
"void main() {\n"
"    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"  /* Apply projection */
"    TexCoord = aTexCoord;\n"                     /* Pass texture coords */
"}\n";

/* Built-in fragment shader source for text rendering */
/* Samples texture and applies effects based on compile-time flags */
static const char* glyph__fragment_shader_body =
"in vec2 TexCoord;\n"                               /* Input from vertex shader */
"out vec4 FragColor;\n"                            /* Final fragment color output */
"uniform sampler2D textTexture;\n"                 /* Glyph atlas texture */
"uniform vec3 textColor;\n"                        /* Text color uniform */
"#ifndef GLYPHGL_MINIMAL\n"                        /* Conditional compilation for effects */
"uniform int effects;\n"                           /* Effects bitmask */
"#endif\n"
"void main() {\n"
"    float sample;\n"                              /* Texture sample value */
"#ifndef GLYPHGL_MINIMAL\n"                        /* Full mode with effects support */
"    if (TexCoord.x == -1.0 && TexCoord.y == -1.0 && (effects & 4) != 0) {\n"
"        sample = 1.0;\n"                          /* Special case for underline rendering */
"    } else {\n"
"        sample = texture(textTexture, TexCoord).r;\n"  /* Sample red channel */
"    }\n"
"    float alpha;\n"                               /* Final alpha value */
"    if ((effects & 8) != 0) {\n"                  /* SDF rendering mode */
"        float dist = sample * 2.0 - 1.0;\n"      /* Convert to signed distance */
"        alpha = dist < 0.0 ? 1.0 : 0.0;\n"       /* Threshold for glyph interior */
"    } else {\n"
"        alpha = sample;\n"                        /* Direct alpha from texture */
"    }\n"
"#else\n"                                          /* Minimal mode - SDF only */
"    sample = texture(textTexture, TexCoord).r;\n"
"    float dist = sample * 2.0 - 1.0;\n"          /* Always use SDF in minimal mode */
"    float alpha = dist < 0.0 ? 1.0 : 0.0;\n"
"#endif\n"
"    FragColor = vec4(textColor, alpha);\n"       /* Combine color and alpha */
"}\n";
/* Shader source buffers for dynamic GLSL version insertion */
static char glyph__vertex_shader_buffer[2048];
static char glyph__fragment_shader_buffer[2048];

/*
 * Generates complete vertex shader source with version directive
 *
 * Combines the GLSL version string with the vertex shader body
 * to create a complete, compilable shader source string.
 *
 * Returns: Pointer to buffer containing complete vertex shader source
 */
static const char* glyph__get_vertex_shader_source() {
    sprintf(glyph__vertex_shader_buffer, "%s%s", glyph_glsl_version_str, glyph__vertex_shader_body);
    return glyph__vertex_shader_buffer;
}

/*
 * Generates complete fragment shader source with version directive
 *
 * Combines the GLSL version string with the fragment shader body
 * to create a complete, compilable shader source string.
 *
 * Returns: Pointer to buffer containing complete fragment shader source
 */
static const char* glyph__get_fragment_shader_source() {
    sprintf(glyph__fragment_shader_buffer, "%s%s", glyph_glsl_version_str, glyph__fragment_shader_body);
    return glyph__fragment_shader_buffer;
}

/* Pre-generated shader sources for default effects - initialized on first use */
static char glyph__vertex_shader_source_buffer[2048];
static char glyph__fragment_shader_source_buffer[2048];
static const char* glyph__vertex_shader_source = NULL;
static const char* glyph__fragment_shader_source = NULL;

static const char* glyph__get_vertex_shader_source_cached() {
    if (!glyph__vertex_shader_source) {
        sprintf(glyph__vertex_shader_source_buffer, "%s%s", glyph_glsl_version_str, glyph__vertex_shader_body);
        glyph__vertex_shader_source = glyph__vertex_shader_source_buffer;
    }
    return glyph__vertex_shader_source;
}

static const char* glyph__get_fragment_shader_source_cached() {
    if (!glyph__fragment_shader_source) {
        sprintf(glyph__fragment_shader_source_buffer, "%s%s", glyph_glsl_version_str, glyph__fragment_shader_body);
        glyph__fragment_shader_source = glyph__fragment_shader_source_buffer;
    }
    return glyph__fragment_shader_source;
}

/*
 * Compiles a GLSL shader and returns the shader object
 *
 * Creates a shader object, sets its source code, compiles it, and
 * checks for compilation errors. On failure, logs the error and
 * cleans up the shader object.
 *
 * Parameters:
 *   type: Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 *   source: Complete GLSL shader source code string
 *
 * Returns: Compiled shader object handle, or 0 on failure
 */
static GLuint glyph__compile_shader(GLenum type, const char* source) {
    /* Create shader object */
    GLuint shader = glyph__glCreateShader(type);
    /* Set shader source */
    glyph__glShaderSource(shader, 1, &source, NULL);
    /* Compile shader */
    glyph__glCompileShader(shader);

    /* Check compilation status */
    GLint success;
    glyph__glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        /* Compilation failed - get error log */
        char info_log[512];
        glyph__glGetShaderInfoLog(shader, 512, NULL, info_log);
        GLYPH_LOG("Shader compilation failed: %s\n", info_log);
        /* Clean up failed shader */
        glyph__glDeleteShader(shader);
        return 0;
    }

    return shader; /* Success */
}

/*
 * Creates and links a complete GLSL program from vertex and fragment shaders
 *
 * Compiles both shaders, attaches them to a program, links the program,
 * and checks for linking errors. Cleans up intermediate objects on failure.
 *
 * Parameters:
 *   vertex_source: Complete vertex shader source code
 *   fragment_source: Complete fragment shader source code
 *
 * Returns: Linked program object handle, or 0 on failure
 */
static GLuint glyph__create_program(const char* vertex_source, const char* fragment_source) {
    /* Compile vertex shader */
    GLuint vertex_shader = glyph__compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (!vertex_shader) return 0;

    /* Compile fragment shader */
    GLuint fragment_shader = glyph__compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fragment_shader) {
        glyph__glDeleteShader(vertex_shader);
        return 0;
    }

    /* Create program and attach shaders */
    GLuint program = glyph__glCreateProgram();
    glyph__glAttachShader(program, vertex_shader);
    glyph__glAttachShader(program, fragment_shader);
    /* Link the program */
    glyph__glLinkProgram(program);

    /* Check linking status */
    GLint success;
    glyph__glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        /* Linking failed - get error log */
        char info_log[512];
        glyph__glGetProgramInfoLog(program, 512, NULL, info_log);
        GLYPH_LOG("Program linking failed: %s\n", info_log);
        /* Clean up failed program and shaders */
        glyph__glDeleteProgram(program);
        glyph__glDeleteShader(vertex_shader);
        glyph__glDeleteShader(fragment_shader);
        return 0;
    }

    /* Clean up intermediate shader objects (program retains compiled code) */
    glyph__glDeleteShader(vertex_shader);
    glyph__glDeleteShader(fragment_shader);

    return program; /* Success */
}

#endif