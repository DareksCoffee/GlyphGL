# GlyphGL


![logo](https://i.imgur.com/fKiiOrx.png)
<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-1.0.9-blue.svg)](https://github.com/DareksCoffee/GlyphGL/releases)
[![Language](https://img.shields.io/badge/Language-C/C++-brightgreen.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/Platform-Cross--platform-lightgrey.svg)](https://en.wikipedia.org/wiki/Cross_platform)
[![Build](https://img.shields.io/badge/Build-Header--only-orange.svg)](https://en.wikipedia.org/wiki/Header-only)

</div>

GlyphGL is a lightweight, header-only text rendering library designed for OpenGL applications. It provides efficient TrueType and OpenType font rendering with atlas-based texturing, supporting modern OpenGL contexts with comprehensive text styling capabilities.
## Features

**Core Capabilities:**
- Header-only design with no external dependencies
- Comprehensive font support including TrueType and OpenType formats
- Efficient atlas-based texture packing for optimal GPU memory usage
- Modern OpenGL 3.3+ compatibility with cross-platform support

**Text Rendering Features:**
- Unicode text rendering with configurable character sets
- Signed Distance Field (SDF) rendering for crisp text at any scale
- Sub-pixel positioning for precise text layout
- Automatic font hinting and anti-aliasing

**Visual Effects and Styling:**
- Text styling options including bold, italic, and underline
- Custom shader effects such as rainbow and glow animations
- Configurable text color, scale, and opacity
- Real-time animated text effects with time-based uniforms

**Performance and Optimization:**
- Minimal compilation mode for reduced memory footprint
- Configurable atlas dimensions (default: 2048×2048)
- Adjustable vertex buffer size for different application requirements
- Efficient glyph caching and texture management

**Development Features:**
- Debug logging capabilities for development workflows
- Dynamic projection matrix updates for responsive applications
- Simple API with comprehensive error handling
- Support for multiple windowing systems (GLFW, GLUT, WinAPI, X11)


## Quick Start

### Installation

1. Clone the repository:
```bash
git clone https://github.com/DareksCoffee/GlyphGL.git
```

2. Include the header in your project:
```c
#include <glyph.h>
```

3. Link against OpenGL and your preferred windowing system

### Basic Usage

```c
#include <GLFW/glfw3.h>
#include <glyph.h>

int main()
{
    // Initialize windowing system
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "GlyphGL Example", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    // Create text renderer
    glyph_renderer_t renderer = glyph_renderer_create("font.ttf", 64.0f,
                                                     NULL, GLYPH_UTF8, NULL, 0);
    glyph_renderer_set_projection(&renderer, 800, 600);

    // Enable blending for smooth text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while(!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render text
        glyph_renderer_draw_text(&renderer, "Hello, GlyphGL!",
                                50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_NONE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glyph_renderer_free(&renderer);
    glfwTerminate();
    return 0;
}
```

### Advanced Features

**Text Effects:**
```c
// Apply bold and italic styling
glyph_renderer_draw_text(&renderer, "Styled Text", x, y, scale, r, g, b, GLYPHGL_BOLD | GLYPHGL_ITALIC);

// Use Signed Distance Field rendering
glyph_renderer_t sdf_renderer = glyph_renderer_create("font.ttf", 64.0f,
                                                    NULL, GLYPH_UTF8, NULL, 1);
```

**Custom Shader Effects:**
```c
// Create and apply custom effect
glyph_effect_t rainbow_effect = glyph_effect_create_rainbow();
glyph_renderer_t effect_renderer = glyph_renderer_create("font.ttf", 64.0f,
                                                        NULL, GLYPH_UTF8, &rainbow_effect, 0);
```
## Library Dependencies

The following libraries are used in the provided demos and examples:

**Graphics and Window Management:**
- GLFW 3.x - Window creation and input handling
- OpenGL - Graphics rendering
- GLUT - Alternative window management (legacy)

**Platform-Specific:**
- Windows API (WinAPI) - Native Windows application development
- X11 - Unix/Linux window system interface

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details or the license headers in the source files.
