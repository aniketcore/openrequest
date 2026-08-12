#include "imgui.h"

// Global or application-level active tab state
static int g_SelectedTopTab = 0; // 0 = Request, 1 = Headers, 2 = Settings

void RenderTopNavigationBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Pin bar to the very top of the window across full width
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 48.0f)); // 48px height top bar

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration 
                           | ImGuiWindowFlags_NoMove 
                           | ImGuiWindowFlags_NoResize 
                           | ImGuiWindowFlags_NoSavedSettings 
                           | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("##TopTabBarWindow", nullptr, flags)) {
        
        const char* tabNames[] = { "Request", "Headers", "Settings" };
        const float buttonWidth = 110.0f;
        const float buttonHeight = 32.0f;

        for (int i = 0; i < 3; ++i) {
            if (i > 0) ImGui::SameLine();

            bool isActive = (g_SelectedTopTab == i);

            // Active tab styling (darker highlight)
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.25f, 0.32f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.28f, 0.36f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
            }

            ImGui::PushID(i);
            if (ImGui::Button(tabNames[i], ImVec2(buttonWidth, buttonHeight))) {
                g_SelectedTopTab = i; // Switch tab on click
            }
            ImGui::PopID();

            ImGui::PopStyleColor(2);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}