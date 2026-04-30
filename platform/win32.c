#include "platform.h"
#include "graphics.h"
#include "mem.h"

#include <stdio.h>
#include <assert.h>

#include <Windows.h>

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct _platform_t
{
    HWND hwnd;
};

const int PLATFORM_T_SIZE = sizeof(platform_t);
float DELTA_TIME = 0;

bool PLT_startup(platform_t* plt, graphicsplatform_t* gph_plt)
{
    WNDCLASS wndclass = {
        .lpszClassName = "Vulkan App",
        .lpfnWndProc = wnd_proc,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        // Extra bytes to store the graphics_t pointer right at the beginning of PLT_run
        .cbWndExtra = sizeof(graphics_t*)
    };
    RegisterClass(&wndclass);

    plt->hwnd = CreateWindowEx(0, wndclass.lpszClassName,"Vulkan Demo :D", WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                    NULL, NULL, NULL, NULL
    );
    if (!plt->hwnd)
    {
        UnregisterClass(wndclass.lpszClassName, NULL); // You don't have to do this since Windows will already unregister the class when closing but still
        return false;
    }

    SetWindowLongPtr(plt->hwnd, GWLP_USERDATA, (LONG_PTR)plt);

    gph_plt->hWnd = plt->hwnd;
    gph_plt->hInstance = GetModuleHandle(NULL);
    return true;
}

int PLT_run(platform_t* plt, graphics_t* gph)
{
    // WM_SIZE should be sent for the first time after ShowWindow's call
    SetWindowLongPtr(plt->hwnd, 0, (LONG_PTR)gph);
    ShowWindow(plt->hwnd, 1);

    int ret = -1;
    while (true)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                return msg.wParam;
        }
        
        GPH_render(gph);
    }

    GPH_close(gph);
    return ret;
}

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);

            graphics_t* graphics = (void*)GetWindowLongPtr(hWnd, 0);
            assert(graphics != NULL);

            GPH_size(graphics, w, h);
        } break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}