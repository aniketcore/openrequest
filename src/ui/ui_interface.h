#pragma once
#include <imgui.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "app_state.h"

#if defined(_WIN32)
  #if defined(OPENREQUEST_UI_BUILD)
    #define OPENREQUEST_UI_API __declspec(dllexport)
  #else
    #define OPENREQUEST_UI_API __declspec(dllimport)
  #endif
#else
  #define OPENREQUEST_UI_API __attribute__((visibility("default")))
#endif

struct UIContextInfo {
    ImGuiContext* imgui_context = nullptr;
    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;
    ImFont* mono_font = nullptr;
    float main_scale = 1.0f;
};

// Function pointer signatures for dynamic library interface
using UI_InitFn = void (*)(const UIContextInfo* info);
using UI_RenderFrameFn = void (*)(UI::AppState* state, SDL_Window* window);
using UI_ShutdownFn = void (*)();

extern "C" {
    OPENREQUEST_UI_API void UI_Init(const UIContextInfo* info);
    OPENREQUEST_UI_API void UI_RenderFrame(UI::AppState* state, SDL_Window* window);
    OPENREQUEST_UI_API void UI_Shutdown();
}
