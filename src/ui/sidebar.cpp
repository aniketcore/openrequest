#include "sidebar.h"
#include "tab.h"
#include "collection.h"
#include "http_resource.h"
#include <cstdio>
#include <algorithm>
#include <memory>

namespace UI {

namespace
{
    void RenderResourceList()
    {
        static uint64_t renaming_id = 0;
        static uint64_t renaming_id_prev = 0;
        static char rename_buf[64] = "";

        float close_btn_w = ImGui::GetFrameHeight();
        float item_spacing_x = ImGui::GetStyle().ItemSpacing.x;

        for (size_t i = 0; i < g_Resources.size(); ++i)
        {
            auto& res = g_Resources[i];
            if (!res) continue;

            float res_btn_w = ImGui::GetContentRegionAvail().x - close_btn_w - item_spacing_x;
            if (res_btn_w < 1.0f)
                res_btn_w = 1.0f;

            ImGui::PushID(static_cast<int>(res->id ^ (res->id >> 32)));

            bool is_renaming = (renaming_id == res->id && res->id != 0);
            if (is_renaming)
            {
                static bool set_focus = false;
                if (renaming_id_prev != renaming_id)
                {
                    set_focus = true;
                    renaming_id_prev = renaming_id;
                }
                if (set_focus)
                {
                    ImGui::SetKeyboardFocusHere();
                    set_focus = false;
                }

                ImGui::SetNextItemWidth(res_btn_w);
                if (ImGui::InputText("##rename", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    res->name = rename_buf;
                    for (auto& tab : g_Tabs)
                    {
                        if (tab->resource == res)
                            tab->name = rename_buf;
                    }
                    renaming_id = 0;
                }
                if (ImGui::IsItemDeactivated())
                {
                    res->name = rename_buf;
                    for (auto& tab : g_Tabs)
                    {
                        if (tab->resource == res)
                            tab->name = rename_buf;
                    }
                    renaming_id = 0;
                }
            }
            else
            {
                // Check if the currently active tab displays this resource
                bool is_active = false;
                for (auto& tab : g_Tabs)
                {
                    if (tab->id == g_ActiveTabId && tab->resource == res)
                    {
                        is_active = true;
                        break;
                    }
                }

                if (is_active)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);

                if (ImGui::Button(res->name.c_str(), ImVec2(res_btn_w, 0.0f)))
                {
                    OpenOrFocusResource(res);
                }

                if (is_active)
                    ImGui::PopStyleColor();

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    renaming_id = res->id;
                    snprintf(rename_buf, sizeof(rename_buf), "%s", res->name.c_str());
                }
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
            if (ImGui::Button("x", ImVec2(close_btn_w, 0.0f)))
            {
                if (renaming_id == res->id)
                    renaming_id = 0;

                auto to_delete = res;
                DeleteResource(to_delete);
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
        ImGui::Text("Collection");
        ImGui::SameLine();

        float button_width = ImGui::GetFrameHeight();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - button_width);

        if (ImGui::Button("+", ImVec2(button_width, button_width)))
        {
            auto res = std::make_shared<HttpResource>();
            res->id = UI::GenerateUID();
            res->name = "HTTP Request";
            g_Resources.push_back(res);
            OpenOrFocusResource(res);
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save##col")) UI::SaveCollectionToFile("collection.yaml");
        ImGui::SameLine();
        if (ImGui::Button("Load##col")) UI::LoadCollectionFromFile("collection.yaml");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Resources");
        ImGui::Spacing();
        RenderResourceList();
    }
    ImGui::End();
}

} // namespace UI
