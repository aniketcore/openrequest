#include <iostream>
#include <filesystem>
#include <SDL3/SDL.h>

#define CR_HOST CR_UNSAFE
#define CR_DEBUG
#include "cr.h"

#include "app_state.h"
#include "imgui_helpers.h"
#include "host_data.h"

static std::filesystem::path GetExecutableDir()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#else
    return std::filesystem::current_path();
#endif
}

int main(int argc, char* argv[])
{
    float main_scale = 1.0f;
    SDL_Window* window = UI::GPU::Initialize(&main_scale);
    if (!window)
    {
        std::cerr << "Failed to initialize SDL/GPU window." << std::endl;
        return 1;
    }

    HostData host_data;
    host_data.imgui_context = ImGui::GetCurrentContext();
    host_data.window = window;
    host_data.gpu_device = UI::GPU::g_GpuDevice;
    host_data.mono_font = UI::GPU::g_MonoFont;
    host_data.state = &UI::g_AppState;
    host_data.main_scale = main_scale;

#if defined(_WIN32)
    const std::string lib_name = "openrequest_ui.dll";
#elif defined(__APPLE__)
    const std::string lib_name = "libopenrequest_ui.dylib";
#else
    const std::string lib_name = "libopenrequest_ui.so";
#endif

    std::filesystem::path plugin_path = GetExecutableDir() / lib_name;

    cr_plugin ctx = {};
    ctx.userdata = &host_data;
    if (!cr_plugin_open(ctx, plugin_path.string().c_str()))
    {
        std::cerr << "[CR] Failed to open plugin from " << plugin_path << std::endl;
        UI::GPU::Cleanup(window);
        return 1;
    }

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.12f, 1.00f);

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

        // Start new ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // cr_plugin_update checks timestamps, copies/reloads, and calls cr_main with CR_STEP
        int rc = cr_plugin_update(ctx);
        if (rc < 0)
        {
            static int last_rc = 0;
            if (last_rc != rc)
            {
                std::cerr << "[CR] Plugin returned error or requested close: " << rc << std::endl;
                last_rc = rc;
            }
        }
        else
        {
            static int last_rc = 0;
            last_rc = 0;
        }

        // End frame and present via SDL_GPU
        ImGui::Render();
        UI::GPU::RenderFrame(window, ImGui::GetDrawData(), clear_color);
    }

    cr_plugin_close(ctx);
    UI::GPU::Cleanup(window);
    return 0;
}
