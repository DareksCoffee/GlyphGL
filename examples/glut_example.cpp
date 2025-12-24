#include <GL/glut.h>
#include <glyph.h>

glyph_renderer_t renderer;

void display() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glyph_renderer_draw_text(&renderer, "Hello, GlyphGL!", 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_EFFECT_NONE);

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 800);
    glutCreateWindow("GLUT Glyph Example");

    renderer = glyph_renderer_create("font.ttf", 64.0f, NULL, GLYPH_ENCODING_UTF8, NULL, 0);
    glyph_renderer_set_projection(&renderer, 800, 800);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(display);
    glutMainLoop();

    glyph_renderer_free(&renderer);
    return 0;
}