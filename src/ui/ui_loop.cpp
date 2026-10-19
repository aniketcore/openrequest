#include "ui_loop.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_helpers.h"
#include "sidebar.h"
#include "tab.h"

namespace UI {

int RunUILoop()
{
    float main_scale = 1.0f;
    SDL_Window *window = UI::GPU::Initialize(&main_scale);
    if (!window)
        return 1;

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.12f, 1.00f);

    // Main loop
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

        // Start Frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Handle UI Zoom shortcuts (Ctrl+ / Ctrl- / Ctrl+MouseWheel)
        {
            static float current_scale = 1.0f;
            ImGuiIO& io = ImGui::GetIO();
            
            bool zoom_in = (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Equal)) || (io.KeyCtrl && io.MouseWheel > 0.0f);
            bool zoom_out = (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Minus)) || (io.KeyCtrl && io.MouseWheel < 0.0f);

            if (zoom_in || zoom_out)
            {
                float change = zoom_in ? 0.1f : -0.1f;
                float new_scale = current_scale + change;
                if (new_scale < 0.5f) new_scale = 0.5f;
                if (new_scale > 3.0f) new_scale = 3.0f;

                static ImGuiStyle style_backup = ImGui::GetStyle();
                ImGui::GetStyle() = style_backup;
                ImGui::GetStyle().ScaleAllSizes(new_scale);
                io.FontGlobalScale = new_scale;
                current_scale = new_scale;
                
                // Consume mouse wheel event so the active window/panels do not scroll
                io.MouseWheel = 0.0f;
            }
        }

        // --- Render Application UI ---
        DrawTopChrome(window);

        // --- Build default dock layout once ---
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            ImGuiID dock_left;
            ImGuiID dock_main = dockspace_id;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.22f, &dock_left, &dock_main);

            ImGui::DockBuilderDockWindow("Collections", dock_left);
            for (auto& tab : g_Tabs)
            {
                char win_title[128];
                snprintf(win_title, sizeof(win_title), "%s###tab_%d", tab->name.c_str(), tab->id);
                ImGui::DockBuilderDockWindow(win_title, dock_main);
            }
            ImGui::DockBuilderFinish(dockspace_id);
            g_MainDockId = dock_main;
        }
        else if (g_MainDockId == 0)
        {
            ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspace_id);
            if (central)
                g_MainDockId = central->ID;
        }

        ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

        RenderSidebar();
        DrawTabContent();

        // End Frame & Submit Draw Calls
        ImGui::Render();
        UI::GPU::RenderFrame(window, ImGui::GetDrawData(), clear_color);

    }

    UI::GPU::Cleanup(window);
    return 0;
}

} // namespace UI
