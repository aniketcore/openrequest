#include "native_window.h"

#if defined(__linux__) && !defined(__ANDROID__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace UI
{
    float g_TitleBarHeight = 35.0f;
    float g_DragMinX = 0.0f;
    float g_DragMaxX = 0.0f;
    float g_MaxBtnMinX = 0.0f;
    float g_MaxBtnMaxX = 0.0f;
    bool  g_MaxBtnHovered = false;

    void InitializeNativeWindow(GLFWwindow* window)
    {
        // Linux X11 uses standard window management
    }

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

        Window root_ret, child_ret;
        int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
        unsigned int mask = 0;
        if (!XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask))
            return false;

        XUngrabPointer(display, CurrentTime);

        XEvent event = {};
        event.type = ClientMessage;
        event.xclient.window = x11_window;
        event.xclient.message_type = net_move_resize;
        event.xclient.format = 32;
        event.xclient.data.l[0] = root_x;
        event.xclient.data.l[1] = root_y;
        event.xclient.data.l[2] = 8; // _NET_WM_MOVERESIZE_MOVE
        event.xclient.data.l[3] = Button1;
        event.xclient.data.l[4] = 1;

        XSendEvent(display, root, False,
                   SubstructureNotifyMask | SubstructureRedirectMask, &event);
        XFlush(display);
        return true;
    }

    void NativeMinimizeWindow(GLFWwindow* window)
    {
        glfwIconifyWindow(window);
    }

    void NativeMaximizeRestoreWindow(GLFWwindow* window)
    {
        if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
            glfwRestoreWindow(window);
        else
            glfwMaximizeWindow(window);
    }

    bool IsNativeWindowMaximized(GLFWwindow* window)
    {
        return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) != 0;
    }
}
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

namespace UI
{
    float g_TitleBarHeight = 35.0f;
    float g_DragMinX = 0.0f;
    float g_DragMaxX = 0.0f;
    float g_MaxBtnMinX = 0.0f;
    float g_MaxBtnMaxX = 0.0f;
    bool  g_MaxBtnHovered = false;

    static WNDPROC s_OriginalWndProc = nullptr;

    static LRESULT CALLBACK FramelessWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_NCCALCSIZE:
        {
            if (wParam == TRUE)
            {
                if (IsZoomed(hwnd))
                {
                    // When maximized, adjust the client rect to match the monitor's work area
                    // so the invisible resize borders do not push titlebar content off-screen.
                    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO mi = { sizeof(mi) };
                    if (GetMonitorInfo(hMonitor, &mi))
                    {
                        NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                        params->rgrc[0] = mi.rcWork;
                    }
                }
                return 0; // Return 0 removes the native titlebar while keeping DWM animations & drop shadow
            }
            return 0;
        }

        case WM_NCHITTEST:
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);

            RECT rc;
            GetClientRect(hwnd, &rc);

            // 1. Native edge & corner resizing (only when not maximized)
            if (!IsZoomed(hwnd))
            {
                const int border = 8;
                bool top = pt.y < border;
                bool bottom = pt.y >= rc.bottom - border;
                bool left = pt.x < border;
                bool right = pt.x >= rc.right - border;

                if (top && left) return HTTOPLEFT;
                if (top && right) return HTTOPRIGHT;
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;
                if (top) return HTTOP;
                if (bottom) return HTBOTTOM;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
            }

            // 2. Titlebar area
            if (pt.y >= 0 && pt.y < g_TitleBarHeight)
            {
                // Hovering over Maximize button (triggers Windows 11 Snap Layouts flyout)
                if (pt.x >= g_MaxBtnMinX && pt.x < g_MaxBtnMaxX)
                {
                    return HTMAXBUTTON;
                }

                // In draggable caption space (triggers native DWM move, snap, and corner snapping)
                if (pt.x >= g_DragMinX && pt.x < g_DragMaxX)
                {
                    return HTCAPTION;
                }
            }

            return HTCLIENT;
        }

        case WM_NCMOUSEMOVE:
        {
            g_MaxBtnHovered = (wParam == HTMAXBUTTON);
            break;
        }

        case WM_NCMOUSELEAVE:
        {
            g_MaxBtnHovered = false;
            break;
        }

        case WM_NCLBUTTONDOWN:
        {
            if (wParam == HTMAXBUTTON)
            {
                return 0;
            }
            break;
        }

        case WM_NCLBUTTONUP:
        {
            if (wParam == HTMAXBUTTON)
            {
                if (IsZoomed(hwnd))
                    ShowWindow(hwnd, SW_RESTORE);
                else
                    ShowWindow(hwnd, SW_MAXIMIZE);
                return 0;
            }
            break;
        }

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (hMonitor)
            {
                MONITORINFO mi = { sizeof(mi) };
                if (GetMonitorInfo(hMonitor, &mi))
                {
                    mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                    mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
                    mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
                    mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
                }
            }
            return 0;
        }

        default:
            break;
        }

        return CallWindowProc(s_OriginalWndProc, hwnd, uMsg, wParam, lParam);
    }

    void InitializeNativeWindow(GLFWwindow* window)
    {
        HWND hwnd = glfwGetWin32Window(window);
        if (!hwnd)
            return;

        // Subclass window procedure to intercept non-client sizing and hit-testing
        s_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FramelessWndProc)));

        // Ensure the window has full top-level application styles so DWM enables animations & snapping
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SYSMENU);
        SetWindowLongPtr(hwnd, GWL_STYLE, style);

        // Extend DWM frame 1 pixel to preserve native Windows drop shadow & rounded corners
        MARGINS margins = { 0, 0, 1, 0 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);

        // Trigger immediate frame recalculation
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    bool NativeStartWindowDrag(GLFWwindow* window)
    {
        HWND hwnd = glfwGetWin32Window(window);
        if (!hwnd)
            return false;

        ReleaseCapture();
        SendMessage(hwnd, WM_SYSCOMMAND, 0xF012 /* SC_MOVE | HTCAPTION */, 0);
        return true;
    }

    void NativeMinimizeWindow(GLFWwindow* window)
    {
        HWND hwnd = glfwGetWin32Window(window);
        if (hwnd)
            ShowWindow(hwnd, SW_MINIMIZE);
        else
            glfwIconifyWindow(window);
    }

    void NativeMaximizeRestoreWindow(GLFWwindow* window)
    {
        HWND hwnd = glfwGetWin32Window(window);
        if (hwnd)
        {
            if (IsZoomed(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
            else
                ShowWindow(hwnd, SW_MAXIMIZE);
        }
        else
        {
            if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
                glfwRestoreWindow(window);
            else
                glfwMaximizeWindow(window);
        }
    }

    bool IsNativeWindowMaximized(GLFWwindow* window)
    {
        HWND hwnd = glfwGetWin32Window(window);
        if (hwnd)
            return IsZoomed(hwnd) != FALSE;
        return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) != 0;
    }
}
#else
namespace UI
{
    float g_TitleBarHeight = 35.0f;
    float g_DragMinX = 0.0f;
    float g_DragMaxX = 0.0f;
    float g_MaxBtnMinX = 0.0f;
    float g_MaxBtnMaxX = 0.0f;
    bool  g_MaxBtnHovered = false;

    void InitializeNativeWindow(GLFWwindow* window) {}
    bool NativeStartWindowDrag(GLFWwindow* window) { return false; }
    void NativeMinimizeWindow(GLFWwindow* window) { glfwIconifyWindow(window); }
    void NativeMaximizeRestoreWindow(GLFWwindow* window)
    {
        if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
            glfwRestoreWindow(window);
        else
            glfwMaximizeWindow(window);
    }
    bool IsNativeWindowMaximized(GLFWwindow* window) { return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) != 0; }
}
#endif
