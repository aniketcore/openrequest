#include "sidebar.h"
#include "tab.h"
#include <cstdio>
#include <algorithm>

namespace UI {

float g_SidebarWidth = 100.0f;

void RenderSidebar()
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
        // Update sidebar width dynamically for DrawTabContent() positioning
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
                g_Tabs.emplace_back(Tab( g_NextTabId, default_name, Tab::HTTP, DrawHttpTab ));
                g_NextTabId++;
            }
            if (ImGui::MenuItem("New WebSocket Tab"))
            {
                char default_name[32];
                snprintf(default_name, sizeof(default_name), "WS Tab %d", g_NextTabId);
                g_Tabs.emplace_back(Tab( g_NextTabId, default_name, Tab::WS, DrawWsTab ));
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

} // namespace UI
