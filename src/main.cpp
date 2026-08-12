#include "imgui_helpers.h"
#include <vector>
#include <algorithm>
#include <string>
#include <functional>

struct Tab {
    int id;
    std::string name;
    enum tabtype {
        HTTP, WS
    };
    tabtype type;
    std::function<void(void)> drawfunc;
};

void drawhttp() {
    ImGui::Text("HTTP Request Settings");
    ImGui::Spacing();
    
    static char url_buf[128] = "https://api.example.com/v1/users";
    ImGui::InputText("URL", url_buf, sizeof(url_buf));
    
    static int current_method = 0;
    const char* methods[] = { "GET", "POST", "PUT", "DELETE" };
    ImGui::Combo("Method", &current_method, methods, IM_ARRAYSIZE(methods));
    
    ImGui::Spacing();
    if (ImGui::Button("Send Request")) {
        // Dummy action
    }
}

void drawws() {
    ImGui::Text("WebSocket Connection Settings");
    ImGui::Spacing();
    
    static char ws_buf[128] = "wss://echo.websocket.org";
    ImGui::InputText("Address", ws_buf, sizeof(ws_buf));
    
    ImGui::Spacing();
    if (ImGui::Button("Connect")) {
        // Dummy action
    }
}

// Global UI State
static float g_SidebarWidth = 100.0f;
static std::vector<Tab> g_Tabs = {
    { 1, "Tab 1", Tab::HTTP, drawhttp },
    { 2, "Tab 2", Tab::HTTP, drawhttp },
    { 3, "Tab 3", Tab::HTTP, drawhttp }
};
static int g_ActiveTabId = 1;
static int g_NextTabId = 4;

void tabwindow()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Position the window directly to the right of the sidebar and let it occupy the rest of the space
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + g_SidebarWidth, viewport->Pos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - g_SidebarWidth, viewport->Size.y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar 
                           | ImGuiWindowFlags_NoMove 
                           | ImGuiWindowFlags_NoResize 
                           | ImGuiWindowFlags_NoSavedSettings 
                           | ImGuiWindowFlags_NoScrollbar;

    // Use clean padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    if (ImGui::Begin("Tab Content Window", nullptr, flags))
    {
        Tab* active_tab = nullptr;
        for (auto& tab : g_Tabs)
        {
            if (tab.id == g_ActiveTabId)
            {
                active_tab = &tab;
                break;
            }
        }

        if (active_tab)
        {
            ImGui::Text("Active Tab: %s", active_tab->name.c_str());
            ImGui::Separator();
            ImGui::Spacing();
            
            if (active_tab->drawfunc)
            {
                active_tab->drawfunc();
            }
            else
            {
                ImGui::Text("No draw function specified for this tab.");
                
            }
        }
        else
        {
            ImGui::Text("No active tab.");
            ImGui::Text("Click the '+' button in the sidebar to create an HTTP or WebSocket tab.");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// Helper to encapsulate ImGui & GLFW Frame Initialization
void smth()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSizeConstraints(ImVec2(50.0f, viewport->Size.y), ImVec2(viewport->Size.x, viewport->Size.y));
    ImGui::SetNextWindowSize(ImVec2(100.0f, viewport->Size.y), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar 
                           | ImGuiWindowFlags_NoMove 
                           | ImGuiWindowFlags_NoSavedSettings 
                           | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.22f, 0.27f, 1.0f));
    if (ImGui::Begin("Scrolling List", nullptr, flags))
    {
        // Update sidebar width dynamically for tabwindow() positioning
        g_SidebarWidth = ImGui::GetWindowWidth();

        ImGui::Text("Tabs");
        ImGui::SameLine();

        float button_width = ImGui::GetFrameHeight();
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x - button_width - ImGui::GetStyle().WindowPadding.x);

        if (ImGui::Button("+", ImVec2(button_width, button_width)) || ImGui::IsItemHovered())
        {
            ImGui::OpenPopup("AddTabPopup");
        }

        if (ImGui::BeginPopup("AddTabPopup"))
        {
            if (ImGui::MenuItem("New HTTP Tab"))
            {
                char default_name[32];
                snprintf(default_name, sizeof(default_name), "HTTP Tab %d", g_NextTabId);
                g_Tabs.push_back({ g_NextTabId, default_name, Tab::HTTP, drawhttp });
                g_NextTabId++;
            }
            if (ImGui::MenuItem("New WebSocket Tab"))
            {
                char default_name[32];
                snprintf(default_name, sizeof(default_name), "WS Tab %d", g_NextTabId);
                g_Tabs.push_back({ g_NextTabId, default_name, Tab::WS, drawws });
                g_NextTabId++;
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar))
        {
            static int renaming_tab_id = -1;
            static int renaming_tab_id_prev = -1;
            static char rename_buf[64] = "";
            
            float close_btn_w = ImGui::GetFrameHeight();
            float item_spacing_x = ImGui::GetStyle().ItemSpacing.x;

            for (size_t item = 0; item < g_Tabs.size(); item++)
            {
                int tab_id = g_Tabs[item].id;
                float tab_btn_w = ImGui::GetContentRegionAvail().x - close_btn_w - item_spacing_x;
                if (tab_btn_w < 1.0f)
                    tab_btn_w = 1.0f;

                ImGui::PushID(tab_id);

                bool is_renaming = (renaming_tab_id == tab_id);
                if (is_renaming)
                {
                    static bool set_focus = false;
                    if (renaming_tab_id_prev != renaming_tab_id)
                    {
                        set_focus = true;
                        renaming_tab_id_prev = renaming_tab_id;
                    }
                    if (set_focus)
                    {
                        ImGui::SetKeyboardFocusHere();
                        set_focus = false;
                    }

                    ImGui::SetNextItemWidth(tab_btn_w);
                    if (ImGui::InputText("##rename", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        g_Tabs[item].name = rename_buf;
                        renaming_tab_id = -1;
                    }
                    if (ImGui::IsItemDeactivated())
                    {
                        g_Tabs[item].name = rename_buf;
                        renaming_tab_id = -1;
                    }
                }
                else
                {
                    bool is_active = (g_ActiveTabId == tab_id);
                    if (is_active)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
                    }

                    if (ImGui::Button(g_Tabs[item].name.c_str(), ImVec2(tab_btn_w, 0.0f)))
                    {
                        g_ActiveTabId = tab_id;
                    }

                    if (is_active)
                    {
                        ImGui::PopStyleColor();
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        renaming_tab_id = tab_id;
                        snprintf(rename_buf, sizeof(rename_buf), "%s", g_Tabs[item].name.c_str());
                    }
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
                if (ImGui::Button("x", ImVec2(close_btn_w, 0.0f)))
                {
                    g_Tabs.erase(g_Tabs.begin() + item);
                    if (g_ActiveTabId == tab_id)
                    {
                        if (!g_Tabs.empty())
                        {
                            g_ActiveTabId = g_Tabs[std::max(0, (int)item - 1)].id;
                        }
                        else
                        {
                            g_ActiveTabId = -1;
                        }
                    }
                    if (renaming_tab_id == tab_id)
                    {
                        renaming_tab_id = -1;
                    }
                    item--;
                }
                ImGui::PopStyleColor(2);

                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}

// Main code
int main(int, char **)
{
    float main_scale = 1.0f;
    GLFWwindow *window = InitWindowAndBackends(&main_scale);
    if (!window)
        return 1;

    // App State (Move this into tab.h / your app controller later)
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        HandleSwapchainResize(window);

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

        smth();
        tabwindow();

        // End Frame & Submit Draw Calls
        ImGui::Render();
        RenderFrame(ImGui::GetDrawData(), clear_color);
    }

    CleanupBackends(window);
    return 0;
}