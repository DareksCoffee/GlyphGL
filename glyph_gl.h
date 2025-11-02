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

#ifndef __GLYPH_GL_H
#define __GLYPH_GL_H

#ifndef GLYPH_NO_GL_LOADER
    #if defined(_WIN32) || defined(_WIN64)
        #include <windows.h>
        #include <GL/gl.h>
    #elif defined(__APPLE__)
        #include <TargetConditionals.h>
        #if TARGET_OS_MAC
            #include <OpenGL/gl.h>
        #endif
    #elif defined(__linux__) || defined(__unix__)
        #include <GL/gl.h>
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "glyph_util.h"

#ifndef GLYPH_NO_GL_LOADER

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006
#endif

typedef void (*PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (*PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum type);
typedef void (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint (*PFNGLGETATTRIBLOCATIONPROC)(GLuint program, const GLchar *name);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (*PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (*PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (*PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (*PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (*PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);

static PFNGLGENBUFFERSPROC glyph__glGenBuffers;
static PFNGLDELETEBUFFERSPROC glyph__glDeleteBuffers;
static PFNGLBINDBUFFERPROC glyph__glBindBuffer;
static PFNGLBUFFERDATAPROC glyph__glBufferData;
static PFNGLBUFFERSUBDATAPROC glyph__glBufferSubData;
static PFNGLCREATESHADERPROC glyph__glCreateShader;
static PFNGLDELETESHADERPROC glyph__glDeleteShader;
static PFNGLSHADERSOURCEPROC glyph__glShaderSource;
static PFNGLCOMPILESHADERPROC glyph__glCompileShader;
static PFNGLGETSHADERIVPROC glyph__glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glyph__glGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC glyph__glCreateProgram;
static PFNGLDELETEPROGRAMPROC glyph__glDeleteProgram;
static PFNGLATTACHSHADERPROC glyph__glAttachShader;
static PFNGLLINKPROGRAMPROC glyph__glLinkProgram;
static PFNGLGETPROGRAMIVPROC glyph__glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glyph__glGetProgramInfoLog;
static PFNGLUSEPROGRAMPROC glyph__glUseProgram;
static PFNGLGETATTRIBLOCATIONPROC glyph__glGetAttribLocation;
static PFNGLGETUNIFORMLOCATIONPROC glyph__glGetUniformLocation;
static PFNGLVERTEXATTRIBPOINTERPROC glyph__glVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glyph__glEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glyph__glDisableVertexAttribArray;
static PFNGLUNIFORM1IPROC glyph__glUniform1i;
static PFNGLUNIFORM1FPROC glyph__glUniform1f;
static PFNGLUNIFORM2FPROC glyph__glUniform2f;
static PFNGLUNIFORM3FPROC glyph__glUniform3f;
static PFNGLUNIFORM4FPROC glyph__glUniform4f;
static PFNGLUNIFORMMATRIX4FVPROC glyph__glUniformMatrix4fv;
static PFNGLACTIVETEXTUREPROC glyph__glActiveTexture;
static PFNGLGENVERTEXARRAYSPROC glyph__glGenVertexArrays;
static PFNGLDELETEVERTEXARRAYSPROC glyph__glDeleteVertexArrays;
static PFNGLBINDVERTEXARRAYPROC glyph__glBindVertexArray;

#if defined(_WIN32) || defined(_WIN64)
    #define GLYPH_GL_LOAD_PROC(type, name) \
        glyph__##name = (type)wglGetProcAddress(#name); \
        if (!glyph__##name) { \
            GLYPH_LOG("Failed to load OpenGL function: %s\n", #name); \
            return 0; \
        }
#elif defined(__APPLE__)
    #include <dlfcn.h>
    #define GLYPH_GL_LOAD_PROC(type, name) \
        glyph__##name = (type)dlsym(RTLD_DEFAULT, #name); \
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

static int glyph_gl_load_functions(void) {
    GLYPH_GL_LOAD_PROC(PFNGLGENBUFFERSPROC, glGenBuffers);
    GLYPH_GL_LOAD_PROC(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    GLYPH_GL_LOAD_PROC(PFNGLBINDBUFFERPROC, glBindBuffer);
    GLYPH_GL_LOAD_PROC(PFNGLBUFFERDATAPROC, glBufferData);
    GLYPH_GL_LOAD_PROC(PFNGLBUFFERSUBDATAPROC, glBufferSubData);
    GLYPH_GL_LOAD_PROC(PFNGLCREATESHADERPROC, glCreateShader);
    GLYPH_GL_LOAD_PROC(PFNGLDELETESHADERPROC, glDeleteShader);
    GLYPH_GL_LOAD_PROC(PFNGLSHADERSOURCEPROC, glShaderSource);
    GLYPH_GL_LOAD_PROC(PFNGLCOMPILESHADERPROC, glCompileShader);
    GLYPH_GL_LOAD_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv);
    GLYPH_GL_LOAD_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    GLYPH_GL_LOAD_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    GLYPH_GL_LOAD_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    GLYPH_GL_LOAD_PROC(PFNGLATTACHSHADERPROC, glAttachShader);
    GLYPH_GL_LOAD_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram);
    GLYPH_GL_LOAD_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    GLYPH_GL_LOAD_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    GLYPH_GL_LOAD_PROC(PFNGLUSEPROGRAMPROC, glUseProgram);
    GLYPH_GL_LOAD_PROC(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
    GLYPH_GL_LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    GLYPH_GL_LOAD_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    GLYPH_GL_LOAD_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    GLYPH_GL_LOAD_PROC(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM1IPROC, glUniform1i);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM1FPROC, glUniform1f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM2FPROC, glUniform2f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM3FPROC, glUniform3f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORM4FPROC, glUniform4f);
    GLYPH_GL_LOAD_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    GLYPH_GL_LOAD_PROC(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    GLYPH_GL_LOAD_PROC(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    GLYPH_GL_LOAD_PROC(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
    GLYPH_GL_LOAD_PROC(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    
    return 1;
}

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
static inline char glyph_glsl_version_str[32] = "#version 330 core\n";
static inline void glyph_gl_set_opengl_version(int major, int minor) {
    sprintf(glyph_glsl_version_str, "#version %d%d0 core\n", major, minor);
}
static const char* glyph__vertex_shader_body = 
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"}\n";
static const char* glyph__fragment_shader_body = 
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D textTexture;\n"
"uniform vec3 textColor;\n"
"#ifndef GLYPHGL_MINIMAL\n"
"uniform int effects;\n"
"#endif\n"
"void main() {\n"
"    float sample;\n"
"#ifndef GLYPHGL_MINIMAL\n"
"    if (TexCoord.x == -1.0 && TexCoord.y == -1.0 && (effects & 4) != 0) {\n"
"        sample = 1.0;\n"
"    } else {\n"
"        sample = texture(textTexture, TexCoord).r;\n"
"    }\n"
"    float alpha;\n"
"    if ((effects & 8) != 0) {\n"
"        float dist = sample * 2.0 - 1.0;\n"
"        alpha = dist < 0.0 ? 1.0 : 0.0;\n"
"    } else {\n"
"        alpha = sample;\n"
"    }\n"
"#else\n"
"    sample = texture(textTexture, TexCoord).r;\n"
"    float dist = sample * 2.0 - 1.0;\n"
"    float alpha = dist < 0.0 ? 1.0 : 0.0;\n"
"#endif\n"
"    FragColor = vec4(textColor, alpha);\n"
"}\n";
static char glyph__vertex_shader_buffer[2048];
static char glyph__fragment_shader_buffer[2048];
static const char* glyph__get_vertex_shader_source() {
    sprintf(glyph__vertex_shader_buffer, "%s%s", glyph_glsl_version_str, glyph__vertex_shader_body);
    return glyph__vertex_shader_buffer;
}
static const char* glyph__get_fragment_shader_source() {
    sprintf(glyph__fragment_shader_buffer, "%s%s", glyph_glsl_version_str, glyph__fragment_shader_body);
    return glyph__fragment_shader_buffer;
}

static const char* glyph__vertex_shader_source = glyph__get_vertex_shader_source();

static const char* glyph__fragment_shader_source = glyph__get_fragment_shader_source();

static GLuint glyph__compile_shader(GLenum type, const char* source) {
    GLuint shader = glyph__glCreateShader(type);
    glyph__glShaderSource(shader, 1, &source, NULL);
    glyph__glCompileShader(shader);
    
    GLint success;
    glyph__glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glyph__glGetShaderInfoLog(shader, 512, NULL, info_log);
        GLYPH_LOG("Shader compilation failed: %s\n", info_log);
        glyph__glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

static GLuint glyph__create_program(const char* vertex_source, const char* fragment_source) {
    GLuint vertex_shader = glyph__compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (!vertex_shader) return 0;
    
    GLuint fragment_shader = glyph__compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fragment_shader) {
        glyph__glDeleteShader(vertex_shader);
        return 0;
    }
    
    GLuint program = glyph__glCreateProgram();
    glyph__glAttachShader(program, vertex_shader);
    glyph__glAttachShader(program, fragment_shader);
    glyph__glLinkProgram(program);
    
    GLint success;
    glyph__glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glyph__glGetProgramInfoLog(program, 512, NULL, info_log);
        GLYPH_LOG("Program linking failed: %s\n", info_log);
        glyph__glDeleteProgram(program);
        glyph__glDeleteShader(vertex_shader);
        glyph__glDeleteShader(fragment_shader);
        return 0;
    }
    
    glyph__glDeleteShader(vertex_shader);
    glyph__glDeleteShader(fragment_shader);
    
    return program;
}

#endif