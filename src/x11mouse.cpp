#include "../libberdip/platform.h"
#include "interface.h"

#include <unistd.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main(int argc, char **argv)
{
    u32 winWidth = 800;
    u32 winHeight = 600;
    Display *display;
    Window   window;
    
    display = XOpenDisplay(NULL);
    if (!display)
    {
        fprintf(stderr, "Couldn't open display!\n");
        return 1;
    }
    
    unsigned long white = WhitePixel(display, DefaultScreen(display));
    unsigned long black = WhitePixel(display, DefaultScreen(display));
    
    window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                 0, 0, winWidth, winHeight, 0, black, white);
    if (!window)
    {
        fprintf(stderr, "Failed to create a window!\n");
        return 1;
    }
    
    XSelectInput(display, window, StructureNotifyMask);
    
    XFlush(display);
    
    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, white);
    XSetBackground(display, gc, black);
    
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
    v2s prevMouseP = mouseP;
    
    v2s mousedP = {};
    v2s prevMousedP = {};
    
    v2s mouseddP = {};
    
    while (isRunning)
    {
        if (0)
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
        
        XEvent event;
        while (XPending(display))
               {
            XNextEvent(display, &event);
            
            if ((event.type == Expose) &&
                (event.xexpose.count == 0))
            {
                XClearWindow(display, window);
            }
            if ((event.type == ClientMessage) &&
                ((Atom)event.xclient.data.l[0] == wmDeleteMessage))
            {
                isRunning = false;
            }
            else if (event.type == MotionNotify)
            {
                mouseP.x = event.xmotion.x;
                mouseP.y = event.xmotion.y;
                
                //XWarpPointer(display, None, window, 0, 0, 0, 0, mouseP.x, mouseP.y);
                
                mousedP = mouseP - prevMouseP;
                mouseddP = mousedP - prevMousedP;
                
                prevMouseP = mouseP;
                prevMousedP = mousedP;
                
                XClearWindow(display, window);
                XSetForeground(display, gc, 160);
                XDrawLine(display, window, gc, halfWidth, halfHeight, mouseP.x,
                          mouseP.y);
                XSetForeground(display, gc, 80);
                XDrawLine(display, window, gc, halfWidth, halfHeight / 2, halfWidth + mousedP.x,
                          halfHeight / 2 + mousedP.y);
                XSetForeground(display, gc, 40);
                XDrawLine(display, window, gc, halfWidth, halfHeight + halfHeight / 2, halfWidth + mouseddP.x,
                          halfHeight + halfHeight / 2 + mouseddP.y);
            }
               }
        
        XFlush(display);
        //usleep(10000);
    }
    
    return 0;
}
