#include "ui_loop.h"
#include "imgui.h"
#include "imgui_helpers.h"
#include "sidebar.h"
#include "tab.h"

namespace UI {

int RunUILoop()
{
    float main_scale = 1.0f;
    GLFWwindow *window = UI::Vulkan::Initialize(&main_scale);
    if (!window)
        return 1;

    // App State (Move this into tab.h / your app controller later)
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        UI::Vulkan::HandleResize(window);

        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start Frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Handle UI Zoom shortcuts (Ctrl+ / Ctrl- / Ctrl+MouseWheel)
        {
            static float current_scale = 1.0f;
            ImGuiIO& io = ImGui::GetIO();
            
            bool zoom_in = (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Equal)) || (io.KeyCtrl && io.MouseWheel > 0.0f);
            bool zoom_out = (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Minus)) || (io.KeyCtrl && io.MouseWheel < 0.0f);
            bool tab_toggle = (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B));

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
            if (tab_toggle && UI::g_SidebarCollapsed) {
                UI::g_SidebarRestoreRequested =1;
                UI::g_SidebarCollapsed = 0;
            }
            else if (tab_toggle && !UI::g_SidebarCollapsed) {
                UI::g_SidebarCollapsed = 1;
            }
        }

        // --- Render Application UI ---

        RenderSidebar();
        DrawTabContent();

        // End Frame & Submit Draw Calls
        ImGui::Render();
        UI::Vulkan::RenderFrame(ImGui::GetDrawData(), clear_color);
    }

    UI::Vulkan::Cleanup(window);
    return 0;
}

} // namespace UI
