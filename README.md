# GlyphGL

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A lightweight, header-only text rendering library for OpenGL applications. GlyphGL provides efficient TrueType font rendering with atlas-based texturing, supporting modern OpenGL contexts.

![logo](https://i.imgur.com/fKiiOrx.png)

## Features

- **Header-only**: No compilation required, just include and use
- **Zero dependencies**: Custom OpenGL loader for cross-platform compatibility
- **TrueType support**: Built-in font parsing and rasterization
- **Atlas-based rendering**: Efficient texture packing for optimal performance
- **Text styling**: Support for bold, italic, and underline effects
- **Signed Distance Field (SDF) rendering**: Smoother text rendering at various scales
- **Custom shader effects**: Support for rainbow, glow, and other visual effects
- **Minimal mode**: Compile-time flag to disable heavy features for reduced memory footprint
- **Configurable parameters**: Adjustable atlas/vertex-buffer sizes for different environments
- **Modern OpenGL**: Compatible with OpenGL 3.3+ contexts
- **Cross-platform**: Works on Windows, macOS, and Linux

## Quick Start

## Installation

1. Clone this repo:
```
git clone https://github.com/DareksCoffee/GlyphGL.git
```
3. Include `glyph.h` in your project
4. Link against OpenGL and any required system libraries

## Usage Example

```c
#include <GLFW/glfw3.h>
#include <glyph.h>

int main()
{
    // Initialize GLFW
    glfwInit();
    // Set the opengl context to be version 3.3+
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Creating GLFW window
    GLFWwindow* window = glfwCreateWindow(
        800, 800, "GLFW Glyph Example", NULL, NULL
    );
    glfwMakeContextCurrent(window);
    // Setting up Glyph renderer
    glyph_renderer_t renderer = glyph_renderer_create("font.ttf", 64.0f, NULL, GLYPH_UTF8, NULL, 0);
    // Setting up the glyph renderer projection 
    glyph_renderer_set_projection(&renderer, 800, 800);

    // Enabling alphha blend
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while(!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Rendering the text "Hello, GlyphGL!"
        glyph_renderer_draw_text(&renderer, "Hello, GlyphGL!", 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_NONE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glyph_renderer_free(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```
## API Reference

### Core Functions

- `glyph_renderer_create(const char* font_path, float pixel_height, const char* charset, uint32_t char_type, void* effect, int use_sdf)` - Initialize a text renderer
- `glyph_renderer_free(glyph_renderer_t* renderer)` - Clean up renderer resources
- `glyph_renderer_set_projection(glyph_renderer_t* renderer, int width, int height)` - Set projection matrix
- `glyph_renderer_update_projection(glyph_renderer_t* renderer, int width, int height)` - Update the projection matrix
- `glyph_renderer_draw_text(glyph_renderer_t* renderer, const char* text, float x, float y, float scale, float r, float g, float b, int effects)` - Render text

### Text Effects

- `GLYPHGL_BOLD` - Render text in bold
- `GLYPHGL_ITALIC` - Render text in italic
- `GLYPHGL_UNDERLINE` - Add underline to text
- `GLYPHGL_SDF` - Enable Signed Distance Field rendering for smoother scaling
- `GLYPH_NONE` - No effects

### Compile-time Flags

- `GLYPHGL_DEBUG` - Enable debug logging
- `GLYPHGL_MINIMAL` - Disable effects and reduce memory footprint
- `GLYPHGL_ATLAS_WIDTH` - Set atlas texture width (default: 2048)
- `GLYPHGL_ATLAS_HEIGHT` - Set atlas texture height (default: 2048)
- `GLYPHGL_VERTEX_BUFFER_SIZE` - Set vertex buffer size (default: 73728)

## Demo

![demo](https://i.imgur.com/esfelJe.gif)

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details or the license headers in the source files.
