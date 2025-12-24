#include <GLFW/glfw3.h>
#include <glyph.h>
#include <string>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(
        800, 800, "GLFW Glyph Timer Demo", NULL, NULL
    );
    glfwMakeContextCurrent(window);
    glyph_renderer_t renderer = glyph_renderer_create("font.ttf", 32.0f, NULL, GLYPH_ENCODING_UTF8, NULL, 1);

    glyph_renderer_set_projection(&renderer, 800, 800);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double totalFrameTime = 0.0;
    int frameCount = 0;

    while(!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        double frameEnd = glfwGetTime();
        double frameTime = frameEnd - frameStart;
        totalFrameTime += frameTime;
        frameCount++;
        double averageFrameTime = totalFrameTime / frameCount;

        std::string timerText = "Average frame time: " + std::to_string(averageFrameTime * 1000.0) + " ms";
        glyph_renderer_draw_text(&renderer, timerText.c_str(), 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPHGL_SDF);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glyph_renderer_free(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}