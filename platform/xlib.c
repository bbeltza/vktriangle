#include "platform.h"
#include "graphics.h"

#include <X11/Xlib.h>

struct _platform_t
{
    Display* display;
    Window window;

    struct
    {
        int w;
        int h;
    } cache;
    
};
const int PLATFORM_T_SIZE = sizeof(platform_t);

bool PLT_startup(platform_t* plt, struct _graphicsplatform_t* gph_plt)
{
    plt->display = XOpenDisplay(NULL);
    if (!plt->display)
        return false;

    plt->cache.w = 800;
    plt->cache.h = 600;

    Window root_wnd = XDefaultRootWindow(plt->display);

    Visual* def_visual = XDefaultVisual(plt->display, DefaultScreen(plt->display));
    XSetWindowAttributes wnd_attribs = {
        .background_pixmap = None,
        .background_pixel = 0xFFFFFFFF,
        .border_pixmap = CopyFromParent,
        .border_pixel = 0L,
        .bit_gravity = ForgetGravity,
        .win_gravity = CenterGravity,
        .backing_store = NotUseful,
        .backing_planes = (unsigned long)(-1),
        .backing_pixel = 0L,
        .save_under = True,
        .event_mask = NoEventMask,
        .do_not_propagate_mask = 0,
        .override_redirect = False,
        .colormap = CopyFromParent,
        .cursor = None
    };
    plt->window = XCreateWindow(plt->display, root_wnd, 0, 0, plt->cache.w, plt->cache.h, 0, 0, InputOutput, def_visual, 0, &wnd_attribs);
    if (!plt->window)
        return false;
    
    Atom WMdestroy_atom = XInternAtom(plt->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(plt->display, plt->window, &WMdestroy_atom, 1);
    XSelectInput(plt->display, plt->window, StructureNotifyMask);
    XStoreName(plt->display, plt->window, "Vulkan Demo :D");
    XMapWindow(plt->display, plt->window);
    XSync(plt->display, False);
    
    gph_plt->display = plt->display;
    gph_plt->window = plt->window;
    return true;
}

int PLT_run(platform_t* plt, struct _graphics_t* gph)
{
    while (true)
    {
        XEvent ev;
        while (XCheckMaskEvent(plt->display, (long)(-1), &ev))
        {
            switch (ev.type)
            {
                case ConfigureNotify:
                    if (ev.xconfigure.width != plt->cache.w || ev.xconfigure.height != plt->cache.h)
                    {
                        GPH_size(gph, ev.xconfigure.width, ev.xconfigure.height);

                        plt->cache.w = ev.xconfigure.width;
                        plt->cache.h = ev.xconfigure.height;
                    } break;
            }
        }

        // Client messages (The one to close the window for this case) don't get recieved in XCheckMaskEvent, do that
        while (XPending(plt->display))
        {
            XNextEvent(plt->display, &ev);
            if (ev.type == ClientMessage)
                goto END;
        }

        GPH_render(gph);
    }

    END:
    XCloseDisplay(plt->display);
    return 0;
}