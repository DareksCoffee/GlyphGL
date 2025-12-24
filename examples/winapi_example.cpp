#include <windows.h>
#include <glyph.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static glyph_renderer_t renderer = {0};
    static bool initialized = false;

    switch (uMsg) {
        case WM_CREATE: {
            PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR), 1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0,
                PFD_MAIN_PLANE, 0, 0, 0, 0
            };

            HDC hdc = GetDC(hwnd);
            int pixelFormat = ChoosePixelFormat(hdc, &pfd);
            SetPixelFormat(hdc, pixelFormat, &pfd);

            HGLRC hglrc = wglCreateContext(hdc);
            wglMakeCurrent(hdc, hglrc);

            renderer = glyph_renderer_create("font.ttf", 64.0f, NULL, GLYPH_ENCODING_UTF8, NULL, 0);
            glyph_renderer_set_projection(&renderer, 800, 800);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            initialized = true;
            break;
        }
        case WM_PAINT: {
            if (!initialized) break;

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glyph_renderer_draw_text(&renderer, "Hello, GlyphGL!", 50.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f, GLYPH_EFFECT_NONE);

            SwapBuffers(hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_SIZE: {
            if (!initialized) break;
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            glyph_renderer_update_projection(&renderer, width, height);
            break;
        }
        case WM_DESTROY: {
            if (initialized) {
                glyph_renderer_free(&renderer);
                HGLRC hglrc = wglGetCurrentContext();
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(hglrc);
            }
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "GlyphGLWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "GlyphGLWindow", "WinAPI Glyph Example",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               800, 800, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}