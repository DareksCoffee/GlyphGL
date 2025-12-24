/*
 * Glyph Effects Module - Advanced Text Rendering Effects
 *
 * This module provides a collection of built-in shader effects for enhanced
 * text rendering. Effects are implemented as GLSL fragment shaders that
 * modify the appearance of rendered glyphs. The system supports both
 * pre-built effects and custom user-defined shaders.
 *
 * Key features:
 * - Built-in effects: glow, rainbow, outline, shadow, wave, gradient, neon
 * - Custom shader support for advanced effects
 * - GLSL-based implementation with automatic version handling
 * - Effect stacking through uniform parameters
 * - Performance-optimized shader generation and caching
 */

#ifndef __GLYPH_EFFECT_H
#define __GLYPH_EFFECT_H

#include "glyph_gl.h"

/* Enumeration of available built-in effects */
typedef enum {
    GLYPH_EFFECT_NONE,      /* No effect - basic text rendering */
    GLYPH_EFFECT_GLOW,      /* Soft glow/bloom effect around text */
    GLYPH_EFFECT_RAINBOW,   /* Animated color cycling effect */
    GLYPH_EFFECT_OUTLINE,   /* Black outline around glyphs */
    GLYPH_EFFECT_SHADOW,    /* Drop shadow effect */
    GLYPH_EFFECT_WAVE,      /* Wavy distortion animation */
    GLYPH_EFFECT_GRADIENT,  /* Vertical color gradient */
    GLYPH_EFFECT_NEON       /* Pulsating neon glow effect */
} glyph_effect_type_t;

/*
 * Effect configuration structure
 *
 * Contains the effect type and associated GLSL shader sources.
 * For built-in effects, shaders are automatically generated.
 * For custom effects, user provides their own shader strings.
 */
typedef struct {
    glyph_effect_type_t type;        /* Type of effect to apply */
    const char* vertex_shader;       /* GLSL vertex shader source */
    const char* fragment_shader;     /* GLSL fragment shader source */
} glyph_effect_t;

/*
 * ================== BUILT-IN SHADER EFFECTS ==================
 *
 * This section documents the available built-in effects, their visual behavior,
 * and the uniform parameters that control their appearance. Each effect is
 * implemented as a GLSL fragment shader with specific uniform variables.
 *
 * Glow Effect:
 * | Creates a soft bloom/glow around text by sampling neighboring pixels
 * | with gaussian-weighted blending. Produces a radiant, ethereal appearance.
 * | - Uniforms: (float) glowIntensity — controls glow brightness (default: 1.0)
 *
 * Rainbow Effect:
 * | Animated color cycling based on fragment coordinates and time.
 * | Creates a horizontal rainbow sweep across the text. Perfect for
 * | attention-grabbing UI elements or debug visualizations.
 * | - Uniforms: (float) time — drives color animation cycle
 *
 * Outline Effect:
 * | Generates a colored outline around glyphs by sampling adjacent pixels.
 * | Creates crisp boundaries around text for improved readability.
 * | - Uniforms: (vec3) outlineColor — RGB color of the outline (default: black)
 *
 * Shadow Effect:
 * | Renders a soft drop shadow by offsetting texture sampling.
 * | Adds depth and visual hierarchy to text elements.
 * | - Uniforms:
 * |   (vec2) shadowOffset — pixel offset vector (default: (0.005, -0.005))
 * |   (vec3) shadowColor  — RGB shadow color (default: black)
 *
 * Wave Effect:
 * | Applies sinusoidal distortion along the horizontal axis.
 * | Creates playful, animated wave motion through the text.
 * | - Uniforms:
 * |   (float) time          — animation time for wave motion
 * |   (float) waveAmplitude — vertical distortion strength (default: 0.001)
 *
 * Gradient Effect:
 * | Smooth vertical color interpolation from top to bottom.
 * | Creates elegant color transitions across text height.
 * | - Uniforms:
 * |   (vec3) gradientStart — RGB color at top of glyphs (default: red)
 * |   (vec3) gradientEnd   — RGB color at bottom of glyphs (default: blue)
 *
 * Neon Effect:
 * | Pulsating brightness animation simulating neon tube lighting.
 * | Creates dynamic, eye-catching glow effects.
 * | - Uniforms: (float) time — controls pulsing animation cycle
 *
 * =================================================================
 */

static const char* glyph__glow_vertex_shader = NULL;
static const char* glyph__get_glow_vertex_shader() {
    if (!glyph__glow_vertex_shader) {
        glyph__glow_vertex_shader = glyph__get_vertex_shader_source_cached();
    }
    return glyph__glow_vertex_shader;
}

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

/*
 * Creates a custom effect using user-provided GLSL shaders
 *
 * Allows advanced users to implement their own text effects by providing
 * custom vertex and fragment shader source code. The shaders must be
 * compatible with the GlyphGL pipeline and use the expected uniform names.
 *
 * Parameters:
 *   vertex_shader: Complete GLSL vertex shader source
 *   fragment_shader: Complete GLSL fragment shader source
 *
 * Returns: glyph_effect_t configured for custom rendering
 */
static inline glyph_effect_t glyph_effect_create_custom(const char* vertex_shader, const char* fragment_shader) {
    glyph_effect_t effect = {GLYPH_EFFECT_NONE, vertex_shader, fragment_shader};
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

/*
 * Creates a glow effect configuration
 *
 * Returns an effect that applies a soft gaussian blur around text
 * to create a radiant, glowing appearance. The glow intensity can
 * be controlled via the 'glowIntensity' uniform.
 *
 * Returns: glyph_effect_t configured for glow rendering
 */
static inline glyph_effect_t glyph_effect_create_glow() {
    glyph_effect_t effect = {GLYPH_EFFECT_GLOW, glyph__get_glow_vertex_shader(), glyph__get_glow_fragment_shader()};
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

/*
 * Creates a rainbow effect configuration
 *
 * Returns an effect that applies animated color cycling across the text
 * based on screen position and time. Creates a horizontal rainbow sweep.
 *
 * Returns: glyph_effect_t configured for rainbow rendering
 */
static inline glyph_effect_t glyph_effect_create_rainbow() {
    glyph_effect_t effect = {GLYPH_EFFECT_RAINBOW, glyph__get_glow_vertex_shader(), glyph__get_rainbow_fragment_shader()};
    return effect;
}

/*
 * Creates an outline effect configuration
 *
 * Returns an effect that draws a colored outline around text glyphs
 * by sampling neighboring pixels. Improves text readability and contrast.
 *
 * Returns: glyph_effect_t configured for outline rendering
 */
static inline glyph_effect_t glyph_effect_create_outline() {
    glyph_effect_t effect = {GLYPH_EFFECT_OUTLINE, glyph__get_glow_vertex_shader(), glyph__get_outline_fragment_shader()};
    return effect;
}

/*
 * Creates a shadow effect configuration
 *
 * Returns an effect that renders a soft drop shadow beneath the text
 * by offsetting texture sampling. Adds visual depth and hierarchy.
 *
 * Returns: glyph_effect_t configured for shadow rendering
 */
static inline glyph_effect_t glyph_effect_create_shadow() {
    glyph_effect_t effect = {GLYPH_EFFECT_SHADOW, glyph__get_glow_vertex_shader(), glyph__get_shadow_fragment_shader()};
    return effect;
}

/*
 * Creates a wave effect configuration
 *
 * Returns an effect that applies sinusoidal distortion along the horizontal
 * axis, creating animated wave motion through the text.
 *
 * Returns: glyph_effect_t configured for wave rendering
 */
static inline glyph_effect_t glyph_effect_create_wave() {
    glyph_effect_t effect = {GLYPH_EFFECT_WAVE, glyph__get_glow_vertex_shader(), glyph__get_wave_fragment_shader()};
    return effect;
}

/*
 * Creates a gradient effect configuration
 *
 * Returns an effect that applies smooth vertical color interpolation
 * from top to bottom of the text, creating elegant color transitions.
 *
 * Returns: glyph_effect_t configured for gradient rendering
 */
static inline glyph_effect_t glyph_effect_create_gradient() {
    glyph_effect_t effect = {GLYPH_EFFECT_GRADIENT, glyph__get_glow_vertex_shader(), glyph__get_gradient_fragment_shader()};
    return effect;
}

/*
 * Creates a neon effect configuration
 *
 * Returns an effect that simulates pulsating neon tube lighting
 * with animated brightness variations.
 *
 * Returns: glyph_effect_t configured for neon rendering
 */
static inline glyph_effect_t glyph_effect_create_neon() {
    glyph_effect_t effect = {GLYPH_EFFECT_NEON, glyph__get_glow_vertex_shader(), glyph__get_neon_fragment_shader()};
    return effect;
}

#endif