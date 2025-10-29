#include <GLFW/glfw3.h>
#include <glyph.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(
        800, 800, "GLFW Glyph Example", NULL, NULL
    );
    glfwMakeContextCurrent(window);
    glyph_renderer_t renderer = glyph_renderer_create("font.ttf", 64.0f, NULL, GLYPH_UTF8, NULL);
    
    glyph_renderer_set_projection(&renderer, 800, 800);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while(!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glyph_renderer_draw_text(&renderer, "Hello, World!", 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_NONE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glyph_renderer_free(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}