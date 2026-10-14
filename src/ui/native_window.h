#pragma once

struct SDL_Window;

namespace UI
{
    // Real-time titlebar metrics updated per-frame by DrawTopChrome (used for hit-testing)
    extern float g_TitleBarHeight;
    extern float g_DragMinX;
    extern float g_DragMaxX;
    extern float g_MaxBtnMinX;
    extern float g_MaxBtnMaxX;
    extern bool  g_MaxBtnHovered;

    // Initializes OS-specific window integration via SDL3 Hit Testing
    void InitializeNativeWindow(SDL_Window* window);

    // Minimize window with native OS animation
    void NativeMinimizeWindow(SDL_Window* window);

    // Maximize/restore window with native OS animation
    void NativeMaximizeRestoreWindow(SDL_Window* window);

    // Check if the window is currently maximized
    bool IsNativeWindowMaximized(SDL_Window* window);
}

