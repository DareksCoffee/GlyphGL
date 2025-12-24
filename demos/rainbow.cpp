// Demo showcasing rainbow effect (requires full build without GLYPHGL_MINIMAL)
#include <GLFW/glfw3.h>
#include <glyph.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(
        800, 800, "GLFW Glyph Rainbow Demo", NULL, NULL
    );
    glfwMakeContextCurrent(window);

    // Create rainbow effect
    glyph_effect_t rainbow_effect = glyph_effect_create_rainbow();
    glyph_renderer_t renderer = glyph_renderer_create("font.ttf", 64.0f, NULL, GLYPH_ENCODING_UTF8, &rainbow_effect, 0);

    glyph_renderer_set_projection(&renderer, 800, 800);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double startTime = glfwGetTime();

    while(!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glyph__glUseProgram(renderer.shader);
        glyph__glUniform1f(glyph__glGetUniformLocation(renderer.shader, "time"), (float)(glfwGetTime() - startTime));
        glyph__glUseProgram(0);

        glyph_renderer_draw_text(&renderer, "Rainbow Text Effect!", 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_EFFECT_NONE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glyph_renderer_free(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}