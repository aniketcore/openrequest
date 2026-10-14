#include "native_window.h"
#include <SDL3/SDL.h>

namespace UI
{
    float g_TitleBarHeight = 35.0f;
    float g_DragMinX = 0.0f;
    float g_DragMaxX = 0.0f;
    float g_MaxBtnMinX = 0.0f;
    float g_MaxBtnMaxX = 0.0f;
    bool  g_MaxBtnHovered = false;

    static SDL_HitTestResult SDLCALL WindowHitTestCallback(SDL_Window* win, const SDL_Point* area, void* data)
    {
        bool is_max = (SDL_GetWindowFlags(win) & SDL_WINDOW_MAXIMIZED) != 0;

        // 1. Edge & corner resizing (only when not maximized)
        if (!is_max)
        {
            const int border = 8;
            int w = 0, h = 0;
            SDL_GetWindowSize(win, &w, &h);

            bool top = area->y < border;
            bool bottom = area->y >= h - border;
            bool left = area->x < border;
            bool right = area->x >= w - border;

            if (top && left) return SDL_HITTEST_RESIZE_TOPLEFT;
            if (top && right) return SDL_HITTEST_RESIZE_TOPRIGHT;
            if (bottom && left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
            if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
            if (top) return SDL_HITTEST_RESIZE_TOP;
            if (bottom) return SDL_HITTEST_RESIZE_BOTTOM;
            if (left) return SDL_HITTEST_RESIZE_LEFT;
            if (right) return SDL_HITTEST_RESIZE_RIGHT;
        }

        // 2. Titlebar area
        if (area->y >= 0 && area->y < g_TitleBarHeight)
        {
            // Draggable caption space
            if (area->x >= g_DragMinX && area->x < g_DragMaxX)
            {
                return SDL_HITTEST_DRAGGABLE;
            }
        }

        return SDL_HITTEST_NORMAL;
    }

    void InitializeNativeWindow(SDL_Window* window)
    {
        SDL_SetWindowHitTest(window, WindowHitTestCallback, nullptr);
    }

    void NativeMinimizeWindow(SDL_Window* window)
    {
        SDL_MinimizeWindow(window);
    }

    void NativeMaximizeRestoreWindow(SDL_Window* window)
    {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
            SDL_RestoreWindow(window);
        else
            SDL_MaximizeWindow(window);
    }

    bool IsNativeWindowMaximized(SDL_Window* window)
    {
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
    }
}
