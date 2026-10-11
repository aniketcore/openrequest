#include "native_window.h"

#if defined(__linux__) && !defined(__ANDROID__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace UI
{
    bool NativeStartWindowDrag(GLFWwindow* window)
    {
        Display* display = glfwGetX11Display();
        Window x11_window = glfwGetX11Window(window);
        if (!display || !x11_window)
            return false;

        Window root = DefaultRootWindow(display);
        Atom net_move_resize = XInternAtom(display, "_NET_WM_MOVERESIZE", False);
        if (net_move_resize == None)
            return false;

        // Query current pointer root position
        Window root_ret, child_ret;
        int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
        unsigned int mask = 0;
        if (!XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask))
            return false;

        // Release the mouse button grab so the Window Manager can take ownership of the drag
        XUngrabPointer(display, CurrentTime);

        // Send _NET_WM_MOVERESIZE client message to root window
        XEvent event = {};
        event.type = ClientMessage;
        event.xclient.window = x11_window;
        event.xclient.message_type = net_move_resize;
        event.xclient.format = 32;
        event.xclient.data.l[0] = root_x;          // Root X
        event.xclient.data.l[1] = root_y;          // Root Y
        event.xclient.data.l[2] = 8;               // _NET_WM_MOVERESIZE_MOVE
        event.xclient.data.l[3] = Button1;         // Button1 (left mouse button)
        event.xclient.data.l[4] = 1;               // Source indication (1 = normal application)

        XSendEvent(display, root, False,
                   SubstructureNotifyMask | SubstructureRedirectMask, &event);
        XFlush(display);
        return true;
    }
}
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <windows.h>

namespace UI
{
    bool NativeStartWindowDrag(GLFWwindow* window)
    {
        HWND hwnd = glfwGetWin32Window(window);
        if (!hwnd)
            return false;

        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return true;
    }
}
#else
namespace UI
{
    bool NativeStartWindowDrag(GLFWwindow* window)
    {
        return false;
    }
}
#endif
