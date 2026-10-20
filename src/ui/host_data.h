#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include "app_state.h"

struct HostData {
    ImGuiContext* imgui_context = nullptr;
    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;
    ImFont* mono_font = nullptr;
    UI::AppState* state = nullptr;
    float main_scale = 1.0f;
};
