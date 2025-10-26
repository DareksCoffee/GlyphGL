# GlyphGL
A lightweight, header-only text rendering library for OpenGL applications.

---

## Example
```c
#include <GLFW/glfw3.h>
#include "glyph.h"
#include <math.h>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(800, 600, "GlyphGL Showcase", NULL, NULL);
    glfwMakeContextCurrent(window);

    glyph_renderer_t renderer = glyph_renderer_create("tests/font.ttf", 64.0f, NULL);
    glyph_renderer_set_projection(&renderer, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float time = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float r = 0.5f + 0.5f * sin(time * 1.3f);
        float g = 0.5f + 0.5f * sin(time * 1.3f + 2.0f);
        float b = 0.5f + 0.5f * sin(time * 1.3f + 4.0f);

        glyph_renderer_draw_text(&renderer, "GlyphGL", 260.0f, 320.0f, 1.0f, r, g, b);
        glyph_renderer_draw_text(&renderer, "Simple. Fast. Beautiful.", 200.0f, 250.0f, 0.6f, 1.0f, 1.0f, 1.0f);

        glfwSwapBuffers(window);
        glfwPollEvents();
        time += 0.01f;
    }

    glyph_renderer_free(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```
---
## Preview
The preview of the example above

![demo](https://i.imgur.com/esfelJe.gif)

---
## Files

- `glyph.h` - Main header with OpenGL renderer functions
- `glyph_truetype.h` - TrueType font parsing and glyph rasterization
- `glyph_atlas.h` - Font atlas creation and glyph packing
- `glyph_image.h` - Image handling and file I/O (PNG/BMP)
  
---
## License

MIT License - see the license headers in the source files for details.
