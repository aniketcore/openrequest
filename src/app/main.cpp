#include "ui_loader.h"
#include "app_state.h"
#include "imgui_helpers.h"
#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[])
{
    float main_scale = 1.0f;
    SDL_Window* window = UI::GPU::Initialize(&main_scale);
    if (!window)
    {
        std::cerr << "Failed to initialize SDL/GPU window." << std::endl;
        return 1;
    }

    // Context info for the UI dynamic library
    UIContextInfo context_info;
    context_info.imgui_context = ImGui::GetCurrentContext();
    context_info.window = window;
    context_info.gpu_device = UI::GPU::g_GpuDevice;
    context_info.mono_font = UI::GPU::g_MonoFont;
    context_info.main_scale = main_scale;

    // Initialize UI dynamic library loader
    UILoader ui_loader;
    std::filesystem::path app_dir = UILoader::GetExecutableDir();
    if (!ui_loader.Initialize(app_dir))
    {
        std::cerr << "Failed to load UI dynamic library from " << app_dir << std::endl;
        UI::GPU::Cleanup(window);
        return 1;
    }
    ui_loader.CallInit(context_info);

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.12f, 1.00f);

    // Main frame loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        UI::GPU::HandleResize(window);

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Check for dynamic library hot-reload (auto on build or manual Ctrl+R / F5)
        ImGuiIO& io = ImGui::GetIO();
        bool reload_hotkey = (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R)) || ImGui::IsKeyPressed(ImGuiKey_F5);
        ui_loader.CheckAndReload(context_info, reload_hotkey);

        // Start new ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Delegate rendering to UI dynamic library
        ui_loader.CallRenderFrame(&UI::g_AppState, window);

        // End frame and present via SDL_GPU
        ImGui::Render();
        UI::GPU::RenderFrame(window, ImGui::GetDrawData(), clear_color);
    }

    ui_loader.Unload();
    UI::GPU::Cleanup(window);
    return 0;
}
