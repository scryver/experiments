#include "../libberdip/platform.h"
#include "../libberdip/linux_memory.h"
//#include "interface.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

global MemoryAPI gMemoryApi_;
global MemoryAPI *gMemoryApi = &gMemoryApi_;

#include "../libberdip/memory.cpp"
#include "../libberdip/linux_memory.cpp"

int main(int argc, char **argv)
{
    u32 winWidth = 800;
    u32 winHeight = 600;
    Display *display;
    Window   window;

    MemoryArena arena = {};
    MemoryAllocator arenaAlloc = {};
    initialize_arena_allocator(&arena, &arenaAlloc);
    linux_memory_api(gMemoryApi);

    display = XOpenDisplay(NULL);
    if (!display)
    {
        fprintf(stderr, "Couldn't open display!\n");
        return 1;
    }

    XVisualInfo visInfo;
    XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &visInfo);

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(display, DefaultRootWindow(display),
                                    visInfo.visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = 0;

    unsigned long white = WhitePixel(display, DefaultScreen(display));
    unsigned long black = WhitePixel(display, DefaultScreen(display));

    window = XCreateWindow(display, DefaultRootWindow(display),
                           0, 0, winWidth, winHeight, 0,
                           visInfo.depth, InputOutput, visInfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
    if (!window)
    {
        fprintf(stderr, "Failed to create a window!\n");
        return 1;
    }

    XSelectInput(display, window, StructureNotifyMask);

    XFlush(display);

    u32 bitmapW = 100;
    u32 bitmapH = 100;
    u32 bitmapDepth = 32;
    u32 pixelCount = bitmapW * bitmapH;
    u32 *pixels = allocate_array(&arenaAlloc, u32, pixelCount, default_memory_alloc());
    for (u32 y = 0; y < bitmapH; ++y) {
        u32 *pixRow = pixels + y * bitmapW;
        for (u32 x = 0; x < bitmapW; ++x) {
            f32 alphaF = (f32)y / (f32)bitmapH * (f32)x / (f32)bitmapW;
            u8 alpha = (u32)(alphaF * 255.0f) & 0xFF;
            u8 red = 0x3F;
            u8 green = 0x70;
            u8 blue = 0x20;
            pixRow[x] = ((alpha << 24) |
                         (red   << 16) |
                         (green <<  8) |
                         (blue  <<  0));
        }
    }

    XImage *xImage = XCreateImage(display, visInfo.visual, bitmapDepth, ZPixmap,
                                  0, (char *)pixels, bitmapW, bitmapH, bitmapDepth, 0);

    Pixmap pixmap = XCreatePixmap(display, window, bitmapW, bitmapH, bitmapDepth);
    GC pixmapGC = XCreateGC(display, pixmap, 0, NULL);

    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, white);
    XSetBackground(display, gc, black);

    XPutImage(display, pixmap, pixmapGC, xImage, 0, 0, 0, 0, bitmapW, bitmapH);

    XSelectInput(display, window, ExposureMask | ButtonPressMask | PointerMotionMask | StructureNotifyMask);

    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    XSetLineAttributes(display, gc, 2, LineSolid, CapButt, JoinBevel);
    XSetFillStyle(display, gc, FillSolid);

    XClearWindow(display, window);
    XMapWindow(display, window);

    b32 isRunning = true;

    s32 halfWidth = winWidth / 2;
    s32 halfHeight = winHeight / 2;

    v2s mouseP = V2S(halfWidth, halfHeight);
    {
        Window rootRet, childRet;
        s32 rootX, rootY;
        s32 winX, winY;
        u32 maskRet;
        if (XQueryPointer(display, window, &rootRet, &childRet, &rootX, &rootY,
                          &winX, &winY, &maskRet))
        {
            if ((winX >= 0) && (winY >= 0))
            {
                mouseP.x = winX;
                mouseP.y = winY;
            }
        }
    }

    u32 count = 0;
    while (isRunning)
    {
        XEvent event;
        while (XPending(display))
        {
            XNextEvent(display, &event);

            switch (event.type)
            {
                case Expose:
                {
                    XClearWindow(display, window);
                } break;

                case ClientMessage:
                {
                    if ((Atom)event.xclient.data.l[0] == wmDeleteMessage)
                    {
                        isRunning = false;
                    }
                } break;

                case MotionNotify:
                {
                    mouseP.x = event.xmotion.x;
                    mouseP.y = event.xmotion.y;
                } break;

                case ConfigureNotify:
                {
                    winWidth = event.xconfigure.width;
                    winHeight = event.xconfigure.height;
                    halfWidth = winWidth / 2;
                    halfHeight = winHeight / 2;
                } break;
            }
        }

        XClearWindow(display, window);
        XSetForeground(display, gc, 160);
        XDrawLine(display, window, gc, halfWidth, halfHeight, mouseP.x,
                  mouseP.y);


        XCopyArea(display, pixmap, window, gc, 0, 0, bitmapW, bitmapH, mouseP.x, mouseP.y);

        if ((count % 10) == 0) {
            for (u32 y = 0; y < bitmapH; ++y) {
                u32 *pixRow = pixels + y * bitmapW;
                for (u32 x = 0; x < bitmapW; ++x) {
                    f32 alphaF = (f32)y / (f32)bitmapH * (f32)x / (f32)bitmapW;
                    u8 alpha = (u32)(alphaF * 255.0f) & 0xFF;
                    u8 red = (3 * ((pixRow[x] >> 16) & 0xFF)) & 0xFF;
                    u8 green = (5 * ((pixRow[x] >> 8) & 0xFF)) & 0xFF;
                    u8 blue = (7 * ((pixRow[x] >> 0) & 0xFF)) & 0xFF;
                    pixRow[x] = ((alpha << 24) |
                                 (red   << 16) |
                                 (green <<  8) |
                                 (blue  <<  0));
                }
            }
            XPutImage(display, pixmap, pixmapGC, xImage, 0, 0, 0, 0, bitmapW, bitmapH);
        }

        ++count;

        XFlush(display);
        usleep(20000);
    }

    XFreePixmap(display, pixmap);
    XDestroyImage(xImage);

    return 0;
}
