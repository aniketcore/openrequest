#include "tab.h"
#include "sidebar.h"
#include <iostream>
#include <algorithm>
#include "httpengine.h"
#include <imgui_stdlib.h>
namespace UI
{

    std::vector<Tab> g_Tabs = {
        {1, "Tab 1", Tab::HTTP, DrawHttpTab},
    };
    int g_ActiveTabId = 1;
    int g_NextTabId = 4;
    int g_SelectedTopTab = 0;

    void DrawHttpTab(Tab & active_tab)
    {
        static HTTP::Engine eng;
        static HTTP::Request req;
        ImGui::Text("HTTP Request Settings");
        ImGui::Spacing();
        ImGui::InputText("#URL", &active_tab.instance.url ,0,nullptr,nullptr);

        const char *methods[] = {"GET", "POST", "PUT", "DELETE"};
        int current_method = static_cast<int>(active_tab.instance.method);
        if (ImGui::Combo("Method", &current_method, methods, IM_ARRAYSIZE(methods)))
        {
            active_tab.instance.method = static_cast<HTTP::Method>(current_method);
        }

        ImGui::Spacing();
        if (ImGui::Button("Send Request"))
        {
            req.url = active_tab.instance.url;
            req.body = "Method: ";
            req.body += methods[active_tab.instance.method];
            req.gotresponse = false;

            eng.dispatchrequest(&req);
            std::cout << "button pressed" << std::endl;
        }

        if (req.gotresponse && ImGui::TreeNode("Text"))
        {
            static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

            // Make the input resizable by wrapping it inside a resizable child window
            if (ImGui::BeginChild("##source_child", ImVec2(500, 300), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY | ImGuiChildFlags_ResizeX))
            {
                ImGui::InputTextMultiline(
                    "##source",
                    &req.getresponse().body,
                    ImVec2(-FLT_MIN, -FLT_MIN),
                    flags,
                    nullptr,
                    nullptr);
            }
            ImGui::EndChild();

            ImGui::TreePop();
        }
    }

    void DrawWsTab(Tab & active_tab)
    {
        ImGui::Text("WebSocket Connection Settings");
        ImGui::Spacing();

        static char ws_buf[128] = "wss://echo.websocket.org";
        ImGui::InputText("Address", ws_buf, sizeof(ws_buf));

        ImGui::Spacing();
        if (ImGui::Button("Connect"))
        {
            // Dummy action
        }
    }

    void DrawTabContent()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();

        // Position the window directly to the right of the sidebar and let it occupy the space
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + g_SidebarWidth, viewport->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - g_SidebarWidth, viewport->Size.y));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;

        // Use clean padding
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("Tab Content Window", nullptr, flags))
        {
            Tab *active_tab = nullptr;
            for (auto &tab : g_Tabs)
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

                if (active_tab->drawFunc)
                {
                    active_tab->drawFunc(*active_tab);
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

    void RenderTopNavigationBar()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();

        // Pin bar to the very top of the window across full width
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 48.0f)); // 48px height top bar

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("##TopTabBarWindow", nullptr, flags))
        {

            const char *tabNames[] = {"Request", "Headers", "Settings"};
            const float buttonWidth = 110.0f;
            const float buttonHeight = 32.0f;

            for (int i = 0; i < 3; ++i)
            {
                if (i > 0)
                    ImGui::SameLine();

                bool isActive = (g_SelectedTopTab == i);

                // Active tab styling (darker highlight)
                if (isActive)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.25f, 0.32f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.28f, 0.36f, 1.0f));
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
                }

                ImGui::PushID(i);
                if (ImGui::Button(tabNames[i], ImVec2(buttonWidth, buttonHeight)))
                {
                    g_SelectedTopTab = i; // Switch tab on click
                }
                ImGui::PopID();

                ImGui::PopStyleColor(2);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

} // namespace UI
