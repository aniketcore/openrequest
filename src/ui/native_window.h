#pragma once

struct GLFWwindow;

namespace UI
{
    // Real-time titlebar metrics updated per-frame by DrawTopChrome (used for Windows hit-testing)
    extern float g_TitleBarHeight;
    extern float g_DragMinX;
    extern float g_DragMaxX;
    extern float g_MaxBtnMinX;
    extern float g_MaxBtnMaxX;
    extern bool  g_MaxBtnHovered;

    // Initializes OS-specific window integration (e.g. DWM frameless subclassing on Windows)
    void InitializeNativeWindow(GLFWwindow* window);

    // Initiates native OS window drag (e.g. _NET_WM_MOVERESIZE on X11, WM_SYSCOMMAND on Win32)
    bool NativeStartWindowDrag(GLFWwindow* window);

    // Minimize window with native OS animation
    void NativeMinimizeWindow(GLFWwindow* window);

    // Maximize/restore window with native OS animation
    void NativeMaximizeRestoreWindow(GLFWwindow* window);

    // Check if the window is currently maximized
    bool IsNativeWindowMaximized(GLFWwindow* window);
}
