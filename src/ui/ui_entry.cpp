#include "cr.h"
#include "tab.h"
#include "sidebar.h"
#include "imgui_helpers.h"
#include "http_resource.h"
#include <imgui_internal.h>
#include <iostream>

#include "host_data.h"

CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation)
{
    if (!ctx || !ctx->userdata)
        return 0;

    auto* data = static_cast<HostData*>(ctx->userdata);

    switch (operation) {
        case CR_LOAD:
            if (data->imgui_context)
            {
                ImGui::SetCurrentContext(data->imgui_context);
            }
            UI::GPU::g_GpuDevice = data->gpu_device;
            UI::GPU::g_MonoFont = data->mono_font;
            UI::RegisterHttpResourceDrawer(UI::DrawHttpResource);
            return 0;

        case CR_STEP:
        {
            auto* state = data->state;
            auto* window = data->window;
            if (!state || !window) return 0;

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
                    
                    io.MouseWheel = 0.0f;
                }
            }

            // 1. Top Chrome (Custom Titlebar, Menus, Window controls)
            UI::DrawTopChrome(window);

            // 2. Default dock layout setup
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
                for (auto& tab : state->tabs)
                {
                    char win_title[128];
                    snprintf(win_title, sizeof(win_title), "%s###tab_%d", tab->name.c_str(), tab->id);
                    ImGui::DockBuilderDockWindow(win_title, dock_main);
                }
                ImGui::DockBuilderFinish(dockspace_id);
                state->main_dock_id = dock_main;
            }
            else if (state->main_dock_id == 0)
            {
                ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspace_id);
                if (central)
                    state->main_dock_id = central->ID;
            }

            ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

            // 3. Sidebar
            if (state->show_sidebar)
            {
                UI::RenderSidebar();
            }

            // 4. Tab Content
            UI::DrawTabContent();
            return 0;
        }

        case CR_UNLOAD:
            return 0;

        case CR_CLOSE:
            return 0;
    }

    return 0;
}
