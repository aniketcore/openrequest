#include "tab.h"
#include "collection.h"
#include "sidebar.h"
#include "native_window.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <imgui_stdlib.h>
#include "imgui_helpers.h"
#include <glaze/yaml.hpp>

namespace UI
{
    namespace
    {
        void DrawWindowControls(SDL_Window* window)
        {
            const float font_size = ImGui::GetFontSize();
            const float btn_h = ImGui::GetFrameHeight();
            const float btn_w = btn_h * 1.35f;
            const float icon_sz = font_size * 0.32f;
            const float stroke = std::max(1.0f, font_size * 0.08f);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

            // Seamless flush buttons
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.12f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.22f));

            // 1. Minimize Button
            if (ImGui::Button("##min_btn", ImVec2(btn_w, btn_h)))
            {
                UI::NativeMinimizeWindow(window);
            }
            {
                ImVec2 p0 = ImGui::GetItemRectMin();
                ImVec2 p1 = ImGui::GetItemRectMax();
                float mid_x = (p0.x + p1.x) * 0.5f;
                float mid_y = (p0.y + p1.y) * 0.5f;
                draw_list->AddLine(ImVec2(mid_x - icon_sz, mid_y), ImVec2(mid_x + icon_sz, mid_y), text_col, stroke);
            }

            ImGui::SameLine();

            // 2. Maximize / Restore Button
            bool is_max = UI::IsNativeWindowMaximized(window);
            if (ImGui::Button("##max_btn", ImVec2(btn_w, btn_h)))
            {
                UI::NativeMaximizeRestoreWindow(window);
            }
            {
                ImVec2 p0 = ImGui::GetItemRectMin();
                ImVec2 p1 = ImGui::GetItemRectMax();
                UI::g_MaxBtnMinX = p0.x;
                UI::g_MaxBtnMaxX = p1.x;

                // Highlight maximize button if hovered either via Windows 11 Snap Layouts (HTMAXBUTTON) or ImGui
                if (UI::g_MaxBtnHovered)
                {
                    draw_list->AddRectFilled(p0, p1, ImColor(255, 255, 255, 31));
                }

                float mid_x = (p0.x + p1.x) * 0.5f;
                float mid_y = (p0.y + p1.y) * 0.5f;
                if (is_max)
                {
                    float off = icon_sz * 0.35f;
                    draw_list->AddRect(ImVec2(mid_x - icon_sz + off, mid_y - icon_sz - off),
                                       ImVec2(mid_x + icon_sz + off, mid_y + icon_sz - off),
                                       text_col, 0.0f, 0, stroke * 0.8f);
                    draw_list->AddRect(ImVec2(mid_x - icon_sz - off, mid_y - icon_sz + off),
                                       ImVec2(mid_x + icon_sz - off, mid_y + icon_sz + off),
                                       text_col, 0.0f, 0, stroke);
                }
                else
                {
                    draw_list->AddRect(ImVec2(mid_x - icon_sz, mid_y - icon_sz),
                                       ImVec2(mid_x + icon_sz, mid_y + icon_sz),
                                       text_col, 0.0f, 0, stroke);
                }
            }

            ImGui::PopStyleColor(3);

            ImGui::SameLine();

            // 3. Close Button (red on hover/active, flush to top-right corner)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.12f, 0.12f, 1.0f));

            if (ImGui::Button("##close_btn", ImVec2(btn_w, btn_h)))
            {
                SDL_Event event = {};
                event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&event);
            }
            {
                ImVec2 p0 = ImGui::GetItemRectMin();
                ImVec2 p1 = ImGui::GetItemRectMax();
                float mid_x = (p0.x + p1.x) * 0.5f;
                float mid_y = (p0.y + p1.y) * 0.5f;
                ImU32 close_col = ImGui::IsItemHovered() ? IM_COL32(255, 255, 255, 255) : text_col;
                float cross_sz = icon_sz * 0.9f;
                draw_list->AddLine(ImVec2(mid_x - cross_sz, mid_y - cross_sz), ImVec2(mid_x + cross_sz, mid_y + cross_sz), close_col, stroke);
                draw_list->AddLine(ImVec2(mid_x + cross_sz, mid_y - cross_sz), ImVec2(mid_x - cross_sz, mid_y + cross_sz), close_col, stroke);
            }

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
        }
    } // namespace

    void DrawTopChrome(SDL_Window* window)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        bool menu_open = ImGui::BeginMainMenuBar();
        ImGui::PopStyleVar();

        if (!menu_open)
            return;

        const float bar_width = ImGui::GetWindowWidth();
        const float bar_height = ImGui::GetWindowHeight();
        const float btn_h = bar_height;
        const float btn_w = btn_h * 1.35f;
        const float window_ctrls_w = btn_w * 3.0f;
        const float ctrls_start_x = bar_width - window_ctrls_w;

        // 1. App Title (Leftmost with nice breathing room)
        ImGui::SetCursorPosX(ImGui::GetStyle().FramePadding.x);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("openrequest");
        ImGui::SameLine();

        // 2. Menus
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Collection", "Ctrl+O"))
            {
                g_AppState.LoadCollection();
            }
            if (ImGui::MenuItem("Save Collection", "Ctrl+S"))
            {
                g_AppState.SaveCollection();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                SDL_Event event = {};
                event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&event);
            }
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Copy", "Ctrl+C");
            ImGui::MenuItem("Paste", "Ctrl+V");
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Sidebar", nullptr, &g_AppState.show_sidebar);
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        if (ImGui::BeginMenu("Run"))
        {
            if (ImGui::MenuItem("Send Request", "Ctrl+Enter"))
            {
                Tab* active = g_AppState.GetActiveTab();
                if (active && active->resource && active->resource->GetType() == ResourceType::HTTP)
                {
                    auto* http = static_cast<HttpResource*>(active->resource.get());
                    http->SendRequest();
                }
            }
            ImGui::EndMenu();
        }

        float menus_end_x = ImGui::GetCursorPosX();

        // 3. Center Draggable Area (covers the entire span from menus to window controls)
        float drag_width = ctrls_start_x - menus_end_x;
        UI::g_TitleBarHeight = btn_h;
        UI::g_DragMinX = menus_end_x;
        UI::g_DragMaxX = ctrls_start_x;

        if (drag_width > 0.0f)
        {
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::InvisibleButton("##window_drag", ImVec2(drag_width, btn_h));
            if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                UI::NativeMaximizeRestoreWindow(window);
            }
        }

        // 4. Window Controls (pinned 100% flush to the right edge)
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetCursorPosX(ctrls_start_x);
        DrawWindowControls(window);

        // 5. Centered Document/Tab Title (rendered directly in the middle of the title bar)
        Tab* active_tab = g_AppState.GetActiveTab();
        std::string tab_name = active_tab ? active_tab->name : "";
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 bar_p0 = ImGui::GetWindowPos();

        if (!tab_name.empty())
        {
            float text_w = ImGui::CalcTextSize(tab_name.c_str()).x;
            float center_x = (bar_width - text_w) * 0.5f;

            // Only display if there's enough space between menus and window controls
            if (center_x > menus_end_x + 16.0f && (center_x + text_w) < ctrls_start_x - 16.0f)
            {
                ImVec2 text_pos = ImVec2(bar_p0.x + center_x, bar_p0.y + (bar_height - ImGui::GetFontSize()) * 0.5f);
                dl->AddText(text_pos, IM_COL32(180, 180, 180, 255), tab_name.c_str());
            }
        }

        // 6. Subtle bottom border separating titlebar from content
        dl->AddLine(ImVec2(bar_p0.x, bar_p0.y + bar_height - 1.0f),
                    ImVec2(bar_p0.x + bar_width, bar_p0.y + bar_height - 1.0f),
                    IM_COL32(50, 50, 54, 255));

        ImGui::EndMainMenuBar();
    }

    // Global active tab state references bound directly to g_AppState
    std::vector<std::unique_ptr<Tab>>& g_Tabs = g_AppState.tabs;
    int& g_ActiveTabId = g_AppState.active_tab_id;
    int& g_NextTabId = g_AppState.next_tab_id;
    int& g_SelectedTopTab = g_AppState.selected_top_tab;
    ImGuiID& g_MainDockId = g_AppState.main_dock_id;

    void FocusTab(int tab_id)
    {
        g_AppState.FocusTab(tab_id);
    }

    void CloseTab(int tab_id)
    {
        g_AppState.CloseTab(tab_id);
    }

    void OpenOrFocusResource(const std::shared_ptr<Resource>& res)
    {
        g_AppState.OpenOrFocusResource(res);
    }

    void DrawTabContent()
    {
        if (g_AppState.tabs.empty())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
            if (g_AppState.main_dock_id != 0)
                ImGui::SetNextWindowDockID(g_AppState.main_dock_id, ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Workspace"))
            {
                ImGui::Text("No open tabs.");
                ImGui::Text("Click a resource in the sidebar to open it, or '+' to create one.");
            }
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        int tab_to_close = -1;

        for (size_t i = 0; i < g_AppState.tabs.size(); ++i)
        {
            auto& tab = g_AppState.tabs[i];
            char window_title[128];
            snprintf(window_title, sizeof(window_title), "%s###tab_%d", tab->name.c_str(), tab->id);

            if (g_AppState.main_dock_id != 0)
                ImGui::SetNextWindowDockID(g_AppState.main_dock_id, ImGuiCond_FirstUseEver);

            if (g_AppState.tab_to_focus == tab->id)
            {
                ImGui::SetNextWindowFocus();
                g_AppState.tab_to_focus = -1;
            }

            bool open = true;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
            if (ImGui::Begin(window_title, &open))
            {
                if (ImGui::IsWindowDocked())
                {
                    g_AppState.main_dock_id = ImGui::GetWindowDockID();
                }

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                {
                    g_AppState.active_tab_id = tab->id;
                }
                tab->Draw();
            }
            ImGui::End();
            ImGui::PopStyleVar();

            if (!open)
            {
                tab_to_close = tab->id;
            }
        }

        if (tab_to_close != -1)
        {
            g_AppState.CloseTab(tab_to_close);
        }
    }
}
