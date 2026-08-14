#include "ui_loop.h"
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
