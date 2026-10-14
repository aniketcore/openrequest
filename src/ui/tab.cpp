#include "tab.h"
#include "sidebar.h"
#include "native_window.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <imgui_stdlib.h>
#include "imgui_helpers.h"

namespace UI
{
    namespace
    {
        Tab* GetActiveTab()
        {
            for (auto& tab : g_Tabs)
            {
                if (tab->id == g_ActiveTabId)
                {
                    return tab.get();
                }
            }
            return nullptr;
        }

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
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 7.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));

        if (!ImGui::BeginMainMenuBar())
        {
            ImGui::PopStyleVar(3);
            return;
        }

        // 1. App Title (Leftmost) - aligned to menu frame padding
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("openrequest");
        ImGui::SameLine();

        // 2. Menus
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Save");
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Sidebar");
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        if (ImGui::BeginMenu("Run"))
        {
            ImGui::MenuItem("Send Request");
            ImGui::EndMenu();
        }

        // 3. Calculate right section width & dynamic center drag width
        Tab* active_tab = GetActiveTab();
        std::string tab_name = active_tab ? active_tab->name : "";
        float tab_text_w = tab_name.empty() ? 0.0f : ImGui::CalcTextSize(tab_name.c_str()).x + 16.0f;

        const float btn_h = ImGui::GetFrameHeight();
        const float btn_w = btn_h * 1.35f;
        const float window_ctrls_w = btn_w * 3.0f;
        const float right_section_w = tab_text_w + window_ctrls_w;

        float current_x = ImGui::GetCursorPosX();
        float bar_width = ImGui::GetWindowWidth();
        float drag_width = bar_width - current_x - right_section_w;

        UI::g_TitleBarHeight = btn_h;
        UI::g_DragMinX = current_x;
        UI::g_DragMaxX = current_x + drag_width;

        // 4. Center Draggable Area (fills the remaining empty space)
        if (drag_width > 10.0f)
        {
            ImGui::SameLine();
            ImGui::InvisibleButton("##window_drag", ImVec2(drag_width, btn_h));
            if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                UI::NativeMaximizeRestoreWindow(window);
            }
        }

        // 5. Right Section: Active Tab + Window Controls (flush to window edge)
        ImGui::SameLine();
        ImGui::SetCursorPosX(bar_width - right_section_w);

        if (!tab_name.empty())
        {
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextUnformatted(tab_name.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }

        DrawWindowControls(window);

        // Subtle bottom border separating titlebar from content
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 bar_p0 = ImGui::GetWindowPos();
        float bar_h = ImGui::GetWindowHeight();
        dl->AddLine(ImVec2(bar_p0.x, bar_p0.y + bar_h - 1.0f),
                    ImVec2(bar_p0.x + bar_width, bar_p0.y + bar_h - 1.0f),
                    IM_COL32(50, 50, 54, 255));

        ImGui::EndMainMenuBar();
        ImGui::PopStyleVar(3);
    }

    // Helper function to initialize global tabs vector
    std::vector<std::unique_ptr<Tab>> InitializeTabs()
    {
        std::vector<std::unique_ptr<Tab>> tabs;
        tabs.push_back(std::make_unique<HttpTab>(1, "Tab 1"));
        return tabs;
    }

    std::vector<std::unique_ptr<Tab>> g_Tabs = InitializeTabs();
    int g_ActiveTabId = 1;
    int g_NextTabId = 2;
    int g_SelectedTopTab = 0;
    
    std::vector<SavedRequest> g_Collection;

    void SaveCollectionToFile(const std::string& path) {
        std::ofstream out(path);
        if (!out) return;
        out << g_Collection.size() << "\n";
        for (const auto& req : g_Collection) {
            out << req.serialized.size() << "\n" << req.serialized;
        }
    }

    void LoadCollectionFromFile(const std::string& path) {
        std::ifstream in(path);
        if (!in) return;
        g_Collection.clear();
        std::string line;
        if (std::getline(in, line)) {
            try {
                int count = std::stoi(line);
                for (int i=0; i<count; ++i) {
                    if (std::getline(in, line)) {
                        int size = std::stoi(line);
                        std::string serialized;
                        serialized.resize(size);
                        in.read(&serialized[0], size);
                        
                        // Extract name from first line of serialized string
                        std::string name = "Unnamed";
                        size_t pos = serialized.find('\n');
                        if (pos != std::string::npos) {
                            name = serialized.substr(0, pos);
                        }
                        g_Collection.push_back({name, serialized});
                    }
                }
            } catch (...) {}
        }
    }


    ImGuiID g_MainDockId = 0;
    static int g_TabToFocus = -1;

    void FocusTab(int tab_id)
    {
        g_ActiveTabId = tab_id;
        g_TabToFocus = tab_id;
    }

    void CloseTab(int tab_id)
    {
        for (auto it = g_Tabs.begin(); it != g_Tabs.end(); ++it)
        {
            if ((*it)->id == tab_id)
            {
                g_Tabs.erase(it);
                if (g_ActiveTabId == tab_id)
                {
                    g_ActiveTabId = g_Tabs.empty() ? -1 : g_Tabs.back()->id;
                    if (g_ActiveTabId != -1)
                        FocusTab(g_ActiveTabId);
                }
                break;
            }
        }
    }

    void DrawTabContent()
    {
        if (g_Tabs.empty())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
            if (g_MainDockId != 0)
                ImGui::SetNextWindowDockID(g_MainDockId, ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Workspace"))
            {
                ImGui::Text("No open tabs.");
                ImGui::Text("Click the '+' button in the sidebar to create an HTTP request.");
            }
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        int tab_to_close = -1;

        for (size_t i = 0; i < g_Tabs.size(); ++i)
        {
            auto& tab = g_Tabs[i];
            char window_title[128];
            snprintf(window_title, sizeof(window_title), "%s###tab_%d", tab->name.c_str(), tab->id);

            if (g_MainDockId != 0)
                ImGui::SetNextWindowDockID(g_MainDockId, ImGuiCond_FirstUseEver);

            if (g_TabToFocus == tab->id)
            {
                ImGui::SetNextWindowFocus();
                g_TabToFocus = -1;
            }

            bool open = true;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
            if (ImGui::Begin(window_title, &open))
            {
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                {
                    g_ActiveTabId = tab->id;
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
            CloseTab(tab_to_close);
        }
    }
} // namespace UI
