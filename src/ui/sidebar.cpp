#include "sidebar.h"
#include "tab.h"
#include <cstdio>
#include <algorithm>
#include <memory>

namespace UI {


namespace
{
    void RenderTabList()
    {
        static int renaming_tab_id = -1;
        static int renaming_tab_id_prev = -1;
        static char rename_buf[64] = "";

        float close_btn_w = ImGui::GetFrameHeight();
        float item_spacing_x = ImGui::GetStyle().ItemSpacing.x;

        for (size_t item = 0; item < g_Tabs.size(); item++)
        {
            int tab_id = g_Tabs[item]->id;
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
                    g_Tabs[item]->name = rename_buf;
                    renaming_tab_id = -1;
                }
                if (ImGui::IsItemDeactivated())
                {
                    g_Tabs[item]->name = rename_buf;
                    renaming_tab_id = -1;
                }
            }
            else
            {
                bool is_active = (g_ActiveTabId == tab_id);
                if (is_active)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);

                if (ImGui::Button(g_Tabs[item]->name.c_str(), ImVec2(tab_btn_w, 0.0f)))
                {
                    FocusTab(tab_id);
                }

                if (is_active)
                    ImGui::PopStyleColor();

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    renaming_tab_id = tab_id;
                    snprintf(rename_buf, sizeof(rename_buf), "%s", g_Tabs[item]->name.c_str());
                }
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
            if (ImGui::Button("x", ImVec2(close_btn_w, 0.0f)))
            {
                if (renaming_tab_id == tab_id)
                    renaming_tab_id = -1;
                CloseTab(tab_id);
                ImGui::PopStyleColor(2);
                ImGui::PopID();
                break;
            }
            ImGui::PopStyleColor(2);

            ImGui::PopID();
        }
    }
}

void RenderSidebar()
{
    if (ImGui::Begin("Collections"))
    {
        ImGui::Text("Requests");
        ImGui::SameLine();

        float button_width = ImGui::GetFrameHeight();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - button_width);

        if (ImGui::Button("+", ImVec2(button_width, button_width)))
        {
            char default_name[32];
            snprintf(default_name, sizeof(default_name), "HTTP Tab %d", g_NextTabId);
            g_Tabs.push_back(std::make_unique<HttpTab>(g_NextTabId, default_name));
            FocusTab(g_NextTabId);
            g_NextTabId++;
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save##col")) UI::SaveCollectionToFile("collection.orq");
        ImGui::SameLine();
        if (ImGui::Button("Load##col")) UI::LoadCollectionFromFile("collection.orq");

        ImGui::Spacing();
        for (size_t i = 0; i < UI::g_Collection.size(); ++i)
        {
            if (ImGui::Selectable(UI::g_Collection[i].name.c_str()))
            {
                for (auto& t : g_Tabs)
                {
                    if (t->id == g_ActiveTabId)
                    {
                        if (auto* http_tab = dynamic_cast<UI::HttpTab*>(t.get()))
                        {
                            http_tab->DeserializeFromString(UI::g_Collection[i].serialized);
                        }
                        break;
                    }
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Open Tabs");
        ImGui::Spacing();
        RenderTabList();
    }
    ImGui::End();
}

} // namespace UI
