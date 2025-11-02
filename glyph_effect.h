#ifndef __GLYPH_EFFECT_H
#define __GLYPH_EFFECT_H

#include "glyph_gl.h"

typedef enum {
    GLYPH_NONE,
    GLYPH_GLOW,
    GLYPH_RAINBOW,
    GLYPH_OUTLINE,
    GLYPH_SHADOW,
    GLYPH_WAVE,
    GLYPH_GRADIENT,
    GLYPH_NEON
} glyph_effect_type_t;

typedef struct {
    glyph_effect_type_t type;
    const char* vertex_shader;
    const char* fragment_shader;
} glyph_effect_t;

/*
 * ================== INTEGRATED SHADERS ==================
 *
 * Glow Effect:
 * | Relatively simple and kind of messy implementation of a glowing text effect.
 * | Simulates bloom by sampling surrounding pixels and blending weighted intensity.
 * | - Uniforms: (float) glowIntensity — controls how bright the glow appears.
 *
 * Rainbow Effect:
 * | Animated color cycling effect based on screen coordinates and time.
 * | Useful for flashy UI elements or debug overlays.
 * | - Uniforms: (float) time — drives hue shifting animation.
 *
 * Outline Effect:
 * | Generates a basic black outline around glyphs using neighboring alpha samples.
 * | - Uniforms: (vec3) outlineColor — defines outline color.
 *
 * Shadow Effect:
 * | Renders a soft shadow by offsetting glyph sampling and blending underneath text.
 * | - Uniforms:
 * |   (vec2) shadowOffset — pixel offset of the shadow.
 * |   (vec3) shadowColor  — color of the shadow.
 *
 * Wave Effect:
 * | Wavy distortion effect using a sine function along the X-axis.
 * | Great for playful, animated text.
 * | - Uniforms:
 * |   (float) time          — drives wave motion.
 * |   (float) waveAmplitude — vertical distortion strength.
 *
 * Gradient Effect:
 * | Smooth color blend from top to bottom using two color uniforms.
 * | - Uniforms:
 * |   (vec3) gradientStart — starting color at the top.
 * |   (vec3) gradientEnd   — ending color at the bottom.
 *
 * Neon Effect:
 * | Pulsating glow animation simulating neon lighting.
 * | - Uniforms:
 * |   (float) time — controls the pulsing brightness over time.
 *
 * =========================================================
 */

static const char* glyph__glow_vertex_shader = glyph__get_vertex_shader_source();

static char glyph__glow_fragment_shader_buffer[2048];
static const char* glyph__glow_fragment_shader = NULL;
static const char* glyph__get_glow_fragment_shader() {
    if (!glyph__glow_fragment_shader) {
        sprintf(glyph__glow_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform float glowIntensity = 1.0;\n"
            "void main() {\n"
            "    float alpha = texture(textTexture, TexCoord).r;\n"
            "    float glow = 0.0;\n"
            "    const int radius = 4;\n"
            "    float totalWeight = 0.0;\n"
            "    for(int i = -radius; i <= radius; i++) {\n"
            "        for(int j = -radius; j <= radius; j++) {\n"
            "            vec2 offset = vec2(float(i), float(j)) * 0.001;\n"
            "            float dist = length(vec2(float(i), float(j))) / float(radius);\n"
            "            float weight = exp(-dist * dist * 4.0);\n"
            "            glow += texture(textTexture, TexCoord + offset).r * weight;\n"
            "            totalWeight += weight;\n"
            "        }\n"
            "    }\n"
            "    glow /= totalWeight;\n"
            "    float finalAlpha = alpha + glow * glowIntensity;\n"
            "    FragColor = vec4(textColor, min(finalAlpha, 1.0));\n"
            "}\n");
        glyph__glow_fragment_shader = glyph__glow_fragment_shader_buffer;
    }
    return glyph__glow_fragment_shader;
}

static inline glyph_effect_t glyph_effect_create_custom(const char* vertex_shader, const char* fragment_shader) {
    glyph_effect_t effect = {GLYPH_NONE, vertex_shader, fragment_shader};
    return effect;
}

static char glyph__rainbow_fragment_shader_buffer[2048];
static const char* glyph__rainbow_fragment_shader = NULL;
static const char* glyph__get_rainbow_fragment_shader() {
    if (!glyph__rainbow_fragment_shader) {
        sprintf(glyph__rainbow_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform float time;\n"
            "void main() {\n"
            "    float alpha = texture(textTexture, TexCoord).r;\n"
            "    if (alpha > 0.0) {\n"
            "        float hue = mod(gl_FragCoord.x * 0.01 + time * 2.0, 6.0);\n"
            "        vec3 rainbow;\n"
            "        if (hue < 1.0) rainbow = vec3(1.0, hue, 0.0);\n"
            "        else if (hue < 2.0) rainbow = vec3(2.0 - hue, 1.0, 0.0);\n"
            "        else if (hue < 3.0) rainbow = vec3(0.0, 1.0, hue - 2.0);\n"
            "        else if (hue < 4.0) rainbow = vec3(0.0, 4.0 - hue, 1.0);\n"
            "        else if (hue < 5.0) rainbow = vec3(hue - 4.0, 0.0, 1.0);\n"
            "        else rainbow = vec3(1.0, 0.0, 6.0 - hue);\n"
            "        FragColor = vec4(rainbow, alpha);\n"
            "    } else {\n"
            "        FragColor = vec4(0.0);\n"
            "    }\n"
            "}\n");
        glyph__rainbow_fragment_shader = glyph__rainbow_fragment_shader_buffer;
    }
    return glyph__rainbow_fragment_shader;
}

static inline glyph_effect_t glyph_effect_create_glow() {
    glyph_effect_t effect = {GLYPH_GLOW, glyph__glow_vertex_shader, glyph__get_glow_fragment_shader()};
    return effect;
}

static char glyph__outline_fragment_shader_buffer[2048];
static const char* glyph__outline_fragment_shader = NULL;
static const char* glyph__get_outline_fragment_shader() {
    if (!glyph__outline_fragment_shader) {
        sprintf(glyph__outline_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform vec3 outlineColor = vec3(0.0, 0.0, 0.0);\n"
            "void main() {\n"
            "    float alpha = texture(textTexture, TexCoord).r;\n"
            "    float outline = 0.0;\n"
            "    for(int i = -1; i <= 1; i++) {\n"
            "        for(int j = -1; j <= 1; j++) {\n"
            "            vec2 offset = vec2(float(i), float(j)) * 0.001;\n"
            "            outline += texture(textTexture, TexCoord + offset).r;\n"
            "        }\n"
            "    }\n"
            "    outline = min(outline, 1.0);\n"
            "    float finalAlpha = max(alpha, outline * 0.3);\n"
            "    vec3 finalColor = mix(outlineColor, textColor, alpha / max(finalAlpha, 0.001));\n"
            "    FragColor = vec4(finalColor, finalAlpha);\n"
            "}\n");
        glyph__outline_fragment_shader = glyph__outline_fragment_shader_buffer;
    }
    return glyph__outline_fragment_shader;
}

static char glyph__shadow_fragment_shader_buffer[2048];
static const char* glyph__shadow_fragment_shader = NULL;
static const char* glyph__get_shadow_fragment_shader() {
    if (!glyph__shadow_fragment_shader) {
        sprintf(glyph__shadow_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform vec2 shadowOffset = vec2(0.005, -0.005);\n"
            "uniform vec3 shadowColor = vec3(0.0, 0.0, 0.0);\n"
            "void main() {\n"
            "    float shadowAlpha = texture(textTexture, TexCoord + shadowOffset).r * 0.5;\n"
            "    float textAlpha = texture(textTexture, TexCoord).r;\n"
            "    vec3 finalColor = mix(shadowColor, textColor, textAlpha);\n"
            "    float finalAlpha = max(textAlpha, shadowAlpha);\n"
            "    FragColor = vec4(finalColor, finalAlpha);\n"
            "}\n");
        glyph__shadow_fragment_shader = glyph__shadow_fragment_shader_buffer;
    }
    return glyph__shadow_fragment_shader;
}

static char glyph__wave_fragment_shader_buffer[2048];
static const char* glyph__wave_fragment_shader = NULL;
static const char* glyph__get_wave_fragment_shader() {
    if (!glyph__wave_fragment_shader) {
        sprintf(glyph__wave_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform float time;\n"
            "uniform float waveAmplitude = 0.001;\n"
            "void main() {\n"
            "    vec2 waveCoord = TexCoord;\n"
            "    waveCoord.y += sin(TexCoord.x * 10.0 + time * 3.0) * waveAmplitude;\n"
            "    float alpha = texture(textTexture, waveCoord).r;\n"
            "    FragColor = vec4(textColor, alpha);\n"
            "}\n");
        glyph__wave_fragment_shader = glyph__wave_fragment_shader_buffer;
    }
    return glyph__wave_fragment_shader;
}

static char glyph__gradient_fragment_shader_buffer[2048];
static const char* glyph__gradient_fragment_shader = NULL;
static const char* glyph__get_gradient_fragment_shader() {
    if (!glyph__gradient_fragment_shader) {
        sprintf(glyph__gradient_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform vec3 gradientStart = vec3(1.0, 0.0, 0.0);\n"
            "uniform vec3 gradientEnd = vec3(0.0, 0.0, 1.0);\n"
            "void main() {\n"
            "    float alpha = texture(textTexture, TexCoord).r;\n"
            "    vec3 gradientColor = mix(gradientStart, gradientEnd, TexCoord.y);\n"
            "    FragColor = vec4(gradientColor, alpha);\n"
            "}\n");
        glyph__gradient_fragment_shader = glyph__gradient_fragment_shader_buffer;
    }
    return glyph__gradient_fragment_shader;
}

static char glyph__neon_fragment_shader_buffer[2048];
static const char* glyph__neon_fragment_shader = NULL;
static const char* glyph__get_neon_fragment_shader() {
    if (!glyph__neon_fragment_shader) {
        sprintf(glyph__neon_fragment_shader_buffer, "%s%s", glyph_glsl_version_str,
            "in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D textTexture;\n"
            "uniform vec3 textColor;\n"
            "uniform int effects;\n"
            "uniform float time;\n"
            "void main() {\n"
            "    float alpha = texture(textTexture, TexCoord).r;\n"
            "    float glow = sin(time * 5.0) * 0.5 + 0.5;\n"
            "    vec3 neonColor = textColor * (1.0 + glow * 0.5);\n"
            "    FragColor = vec4(neonColor, alpha);\n"
            "}\n");
        glyph__neon_fragment_shader = glyph__neon_fragment_shader_buffer;
    }
    return glyph__neon_fragment_shader;
}

static inline glyph_effect_t glyph_effect_create_rainbow() {
    glyph_effect_t effect = {GLYPH_RAINBOW, glyph__glow_vertex_shader, glyph__get_rainbow_fragment_shader()};
    return effect;
}

static inline glyph_effect_t glyph_effect_create_outline() {
    glyph_effect_t effect = {GLYPH_OUTLINE, glyph__glow_vertex_shader, glyph__get_outline_fragment_shader()};
    return effect;
}

static inline glyph_effect_t glyph_effect_create_shadow() {
    glyph_effect_t effect = {GLYPH_SHADOW, glyph__glow_vertex_shader, glyph__get_shadow_fragment_shader()};
    return effect;
}

static inline glyph_effect_t glyph_effect_create_wave() {
    glyph_effect_t effect = {GLYPH_WAVE, glyph__glow_vertex_shader, glyph__get_wave_fragment_shader()};
    return effect;
}

static inline glyph_effect_t glyph_effect_create_gradient() {
    glyph_effect_t effect = {GLYPH_GRADIENT, glyph__glow_vertex_shader, glyph__get_gradient_fragment_shader()};
    return effect;
}

static inline glyph_effect_t glyph_effect_create_neon() {
    glyph_effect_t effect = {GLYPH_NEON, glyph__glow_vertex_shader, glyph__get_neon_fragment_shader()};
    return effect;
}

#endif