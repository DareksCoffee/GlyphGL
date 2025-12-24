#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <glyph.h>
#include <cstring>

int main()
{
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask;
    Window window = XCreateWindow(display, root, 0, 0, 800, 800, 0,
                                  DefaultDepth(display, screen), InputOutput,
                                  DefaultVisual(display, screen),
                                  CWEventMask, &swa);

    XMapWindow(display, window);
    XStoreName(display, window, "X11 Glyph Example");

    int attribs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo* vi = glXChooseVisual(display, screen, attribs);
    if (!vi) {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        return 1;
    }

    GLXContext context = glXCreateContext(display, vi, NULL, GL_TRUE);
    glXMakeCurrent(display, window, context);

    glyph_renderer_t renderer = glyph_renderer_create("font.ttf", 64.0f, NULL, GLYPH_ENCODING_UTF8, NULL, 0);
    glyph_renderer_set_projection(&renderer, 800, 800);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    XEvent event;
    bool exposed = false;
    while (true) {
        while (XPending(display)) {
            XNextEvent(display, &event);
            if (event.type == KeyPress) goto exit;
            if (event.type == Expose) exposed = true;
        }

        if (exposed) {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glyph_renderer_draw_text(&renderer, "Hello, GlyphGL!", 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_EFFECT_NONE);

            glXSwapBuffers(display, window);
        }

    }
exit:

    glyph_renderer_free(&renderer);
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, context);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}