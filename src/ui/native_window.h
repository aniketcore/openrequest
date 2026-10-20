#pragma once

#include <SDL3/SDL.h>
#include "app_state.h"

namespace UI
{
    // Real-time titlebar metrics bound to central AppState
    inline float& g_TitleBarHeight = g_AppState.title_bar_height;
    inline float& g_DragMinX = g_AppState.drag_min_x;
    inline float& g_DragMaxX = g_AppState.drag_max_x;
    inline float& g_MaxBtnMinX = g_AppState.max_btn_min_x;
    inline float& g_MaxBtnMaxX = g_AppState.max_btn_max_x;
    inline bool&  g_MaxBtnHovered = g_AppState.max_btn_hovered;

    inline SDL_HitTestResult SDLCALL WindowHitTestCallback(SDL_Window* win, const SDL_Point* area, void* data)
    {
        auto* state = static_cast<UI::AppState*>(data);
        if (!state)
            state = &UI::g_AppState;

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
        if (area->y >= 0 && area->y < state->title_bar_height)
        {
            // Draggable caption space
            if (area->x >= state->drag_min_x && area->x < state->drag_max_x)
            {
                return SDL_HITTEST_DRAGGABLE;
            }
        }

        return SDL_HITTEST_NORMAL;
    }

    // Initializes OS-specific window integration via SDL3 Hit Testing
    inline void InitializeNativeWindow(SDL_Window* window, AppState* state = &g_AppState)
    {
        SDL_SetWindowHitTest(window, WindowHitTestCallback, state ? state : &g_AppState);
    }

    // Minimize window with native OS animation
    inline void NativeMinimizeWindow(SDL_Window* window)
    {
        SDL_MinimizeWindow(window);
    }

    // Maximize/restore window with native OS animation
    inline void NativeMaximizeRestoreWindow(SDL_Window* window)
    {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
            SDL_RestoreWindow(window);
        else
            SDL_MaximizeWindow(window);
    }

    // Check if the window is currently maximized
    inline bool IsNativeWindowMaximized(SDL_Window* window)
    {
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
    }
}
