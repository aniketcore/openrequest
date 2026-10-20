#include "http_resource.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <imgui.h>
#include <imgui_stdlib.h>
#include "imgui_internal.h"

#include "imgui_helpers.h"
#include "TextEditor.h"
#include <glaze/json/prettify.hpp>

namespace
{
    bool Splitter(const char* id_str, float* size1, float* size2, float min_size1, float min_size2, float thickness = 6.0f)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID(id_str);
        ImVec2 pos = window->DC.CursorPos;
        float width = ImGui::GetContentRegionAvail().x;
        ImRect bb(pos, ImVec2(pos.x + width, pos.y + thickness));

        ImGui::ItemSize(ImVec2(width, thickness));

        return ImGui::SplitterBehavior(bb, id, ImGuiAxis_Y, size1, size2, min_size1, min_size2, 3.0f, 0.0f);
    }

    void SetupEditorContextMenu(TextEditor& editor)
    {
        editor.SetTextContextMenuCallback([&editor](const TextEditor::PopupData&) {
            bool ro = editor.IsReadOnlyEnabled();
            if (!ro && ImGui::MenuItem("Cut", "Ctrl+X", nullptr, editor.AnyCursorHasSelection()))
            {
                editor.Cut();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, editor.AnyCursorHasSelection()))
            {
                editor.Copy();
            }
            if (!ro && ImGui::MenuItem("Paste", "Ctrl+V", nullptr, ImGui::GetClipboardText() != nullptr))
            {
                editor.Paste();
            }
            if (ImGui::MenuItem("Select All", "Ctrl+A", nullptr, !editor.IsEmpty()))
            {
                editor.SelectAll();
            }
            ImGui::Separator();
            if (!ro && ImGui::MenuItem("Format JSON"))
            {
                std::string formatted = glz::prettify_json(editor.GetText());
                if (!formatted.empty())
                {
                    editor.SetText(formatted);
                }
            }
            bool wrap = editor.IsWordWrapEnabled();
            if (ImGui::MenuItem("Word Wrap", nullptr, &wrap))
            {
                editor.SetWordWrapEnabled(wrap);
            }
            if (ImGui::BeginMenu("Line Spacing"))
            {
                if (ImGui::MenuItem("1.0x", nullptr, editor.GetLineSpacing() == 1.0f)) { editor.SetLineSpacing(1.0f); }
                if (ImGui::MenuItem("1.25x (Default)", nullptr, editor.GetLineSpacing() == 1.25f)) { editor.SetLineSpacing(1.25f); }
                if (ImGui::MenuItem("1.5x", nullptr, editor.GetLineSpacing() == 1.5f)) { editor.SetLineSpacing(1.5f); }
                ImGui::EndMenu();
            }
        });
    }

    void ConfigureTextEditor(TextEditor& editor, bool readOnly, const TextEditor::Language* language = nullptr)
    {
        editor.SetPalette(TextEditor::GetDarkPalette());
        editor.SetTabSize(2);
        editor.SetInsertSpacesOnTabs(true);
        editor.SetAutoIndentEnabled(true);
        editor.SetReadOnlyEnabled(readOnly);
        editor.SetShowLineNumbersEnabled(true);
        editor.SetShowWhitespacesEnabled(false);
        editor.SetShowMatchingBrackets(true);
        editor.SetCompletePairedGlyphs(true);
        editor.SetShowCurrentLineHighlightEnabled(true);
        editor.SetLineFoldingEnabled(true);
        editor.SetFindReplaceEnabled(true);
        editor.SetShowScrollbarMiniMapEnabled(true);
        editor.SetShowMiniMapEnabled(false);
        editor.SetShowPanScrollIndicatorEnabled(false);
        editor.SetLineSpacing(1.25f);
        editor.SetLineNumberLeftMargin(1);
        editor.SetTextLeftMargin(1);
        if (language != nullptr)
        {
            editor.SetLanguage(language);
        }
        SetupEditorContextMenu(editor);
    }

    ImVec4 GetMethodColor(HTTP::Method method)
    {
        switch (method)
        {
            case HTTP::GET:    return ImVec4(0.18f, 0.80f, 0.44f, 1.0f); // Bright Green
            case HTTP::POST:   return ImVec4(0.85f, 0.72f, 0.38f, 1.0f); // Warm Gold / Yellow (from screenshot: rgb(205, 181, 109))
            case HTTP::PUT:    return ImVec4(0.28f, 0.62f, 0.95f, 1.0f); // Blue
            case HTTP::DELETE: return ImVec4(0.92f, 0.32f, 0.32f, 1.0f); // Red
            default:           return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
        }
    }

    const char* GetMethodName(HTTP::Method method)
    {
        switch (method)
        {
            case HTTP::GET:    return "GET";
            case HTTP::POST:   return "POST";
            case HTTP::PUT:    return "PUT";
            case HTTP::DELETE: return "DELETE";
            default:           return "GET";
        }
    }

    void DrawRequestBar(HttpResource* http)
    {
        const float bar_h = 34.0f;
        const float send_btn_w = 96.0f;
        const float spacing = 8.0f;
        const float avail_w = ImGui::GetContentRegionAvail().x;
        const float url_box_w = std::max(120.0f, avail_w - send_btn_w - spacing);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 start_pos = ImGui::GetCursorScreenPos();
        ImVec2 box_max = ImVec2(start_pos.x + url_box_w, start_pos.y + bar_h);

        // Reserve space in layout
        ImGui::Dummy(ImVec2(avail_w, bar_h));

        // 1. Draw outer background & subtle border for the unified URL input bar
        ImU32 bg_col = ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.13f, 1.0f));       // #1f1f21
        ImU32 border_col = ImGui::GetColorU32(ImVec4(0.24f, 0.24f, 0.26f, 1.0f));   // #3e3e42
        draw_list->AddRectFilled(start_pos, box_max, bg_col, 5.0f);
        draw_list->AddRect(start_pos, box_max, border_col, 5.0f);

        // 2. Method button inside the box
        const char* current_method_name = GetMethodName(http->method);
        ImVec4 method_col = GetMethodColor(http->method);

        ImGui::SetCursorScreenPos(ImVec2(start_pos.x + 4.0f, start_pos.y + 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.06f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.12f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, (bar_h - 4.0f - ImGui::GetFontSize()) * 0.5f));

        std::string method_btn_str = std::string(current_method_name) + "   v##method_btn";
        ImGui::PushStyleColor(ImGuiCol_Text, method_col);
        if (ImGui::Button(method_btn_str.c_str()))
        {
            ImGui::OpenPopup("##method_dropdown_popup");
        }
        ImGui::PopStyleColor(); // ImGuiCol_Text

        float method_btn_w = ImGui::GetItemRectSize().x;

        if (ImGui::BeginPopup("##method_dropdown_popup"))
        {
            HTTP::Method all_methods[] = { HTTP::GET, HTTP::POST, HTTP::PUT, HTTP::DELETE };
            for (auto m : all_methods)
            {
                ImVec4 col = GetMethodColor(m);
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                if (ImGui::MenuItem(GetMethodName(m), nullptr, http->method == m))
                {
                    http->method = m;
                }
                ImGui::PopStyleColor();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(); // FramePadding
        ImGui::PopStyleColor(3); // Button colors

        // 3. Vertical divider line
        float divider_x = start_pos.x + 4.0f + method_btn_w + 4.0f;
        draw_list->AddLine(
            ImVec2(divider_x, start_pos.y + 6.0f),
            ImVec2(divider_x, start_pos.y + bar_h - 6.0f),
            border_col, 1.0f);

        // 4. URL text input
        float url_input_x = divider_x + 6.0f;
        float url_input_w = std::max(20.0f, box_max.x - url_input_x - 6.0f);
        ImGui::SetCursorScreenPos(ImVec2(url_input_x, start_pos.y + 2.0f));
        ImGui::SetNextItemWidth(url_input_w);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, (bar_h - 4.0f - ImGui::GetFontSize()) * 0.5f));
        bool enter_pressed = ImGui::InputTextWithHint("##url_input", "Enter URL or paste text", &http->url, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // 5. Send / Cancel button on the right
        bool is_running = http->IsRunning();
        ImVec2 btn_pos = ImVec2(start_pos.x + url_box_w + spacing, start_pos.y);
        ImGui::SetCursorScreenPos(btn_pos);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, (bar_h - ImGui::GetFontSize()) * 0.5f));

        bool send_clicked = false;
        if (is_running)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.82f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.30f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
            if (ImGui::Button("Cancel##send_btn", ImVec2(send_btn_w, bar_h)))
            {
                http->CancelRequest();
            }
            ImGui::PopStyleColor(3);
        }
        else
        {
            // Blue Send button matching screenshot
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.40f, 0.80f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.48f, 0.88f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.35f, 0.72f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            if (ImGui::Button("Send   v##send_btn", ImVec2(send_btn_w, bar_h)) || enter_pressed)
            {
                send_clicked = true;
            }
            // Draw subtle split-button divider inside Send button
            draw_list->AddLine(
                ImVec2(btn_pos.x + send_btn_w - 24.0f, btn_pos.y + 6.0f),
                ImVec2(btn_pos.x + send_btn_w - 24.0f, btn_pos.y + bar_h - 6.0f),
                ImGui::GetColorU32(ImVec4(0.12f, 0.30f, 0.65f, 0.7f)), 1.0f);

            ImGui::PopStyleColor(4);
        }

        ImGui::PopStyleVar(2); // FrameRounding, FramePadding

        // Restore cursor position for items below the bar
        ImGui::SetCursorScreenPos(ImVec2(start_pos.x, start_pos.y + bar_h + spacing));

        if (send_clicked && !is_running)
        {
            if (http->requestEditor)
            {
                http->body = http->requestEditor->GetText();
                http->req.body = http->body;
                http->requestEditorVersion = http->requestEditor->GetUndoIndex();
            }
            http->SendRequest();
        }
    }
}

namespace UI
{
    void DrawHttpResource(HttpResource* http)
    {
        if (!http) return;

        ImGui::PushID(http);
        static bool responseReadOnly = true;

        if (!http->requestEditor)
        {
            http->requestEditor = std::make_unique<TextEditor>();
            ConfigureTextEditor(*http->requestEditor, false, TextEditor::Language::Json());
        }
        if (!http->responseEditor)
        {
            http->responseEditor = std::make_unique<TextEditor>();
            ConfigureTextEditor(*http->responseEditor, true, TextEditor::Language::Json());
        }

        if (!http->requestEditorInitialized)
        {
            http->requestEditor->SetText(http->body.empty() ? http->req.body : http->body);
            http->requestEditorInitialized = true;
            http->requestEditorVersion = http->requestEditor->GetUndoIndex();
        }

        if (http->req.gotresponse && http->req.getresponse().body.has_value())
        {
            const auto& body = http->req.getresponse().body.value();
            if (body != http->lastResponseText)
            {
                auto firstNonWs = body.find_first_not_of(" \t\r\n");
                bool isJson = (firstNonWs != std::string::npos && (body[firstNonWs] == '{' || body[firstNonWs] == '['));
                http->responseEditor->SetLanguage(isJson ? TextEditor::Language::Json() : nullptr);
                http->responseEditor->SetText(body);
                http->lastResponseText = body;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        const float splitter_thickness = 6.0f;
        const float min_request_h = 100.0f;
        const float min_response_h = 100.0f;
        float total_avail_y = ImGui::GetContentRegionAvail().y;

        if (http->requestHeight < 0.0f)
        {
            http->requestHeight = total_avail_y * 0.50f;
        }

        float max_request_h = total_avail_y - min_response_h - splitter_thickness;
        if (max_request_h < min_request_h)
            max_request_h = min_request_h;

        http->requestHeight = std::clamp(http->requestHeight, min_request_h, max_request_h);
        float response_height = total_avail_y - http->requestHeight - splitter_thickness;
        if (response_height < min_response_h)
            response_height = min_response_h;

        // --- 1. Request Subwindow ---
        ImGui::BeginChild("##request_panel", ImVec2(0.0f, http->requestHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

        DrawRequestBar(http);

        if (http->IsRunning())
        {
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.4f, 1.0f), "Sending request...");
        }

        ImGui::Spacing();
        ImGui::Text("Headers");
        ImGui::SameLine();
        if (ImGui::Button("+##add_header"))
        {
            http->requestHeaders.push_back({"", ""});
        }
        for (size_t i = 0; i < http->requestHeaders.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            ImGui::SetNextItemWidth(150.0f);
            ImGui::InputText("##key", &http->requestHeaders[i].first);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(250.0f);
            ImGui::InputText("##value", &http->requestHeaders[i].second);
            ImGui::SameLine();
            if (ImGui::Button("X"))
            {
                http->requestHeaders.erase(http->requestHeaders.begin() + i);
                --i;
            }
            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::Text("Request Body");
        ImGui::SameLine();
        if (ImGui::SmallButton("Format JSON##req_fmt"))
        {
            std::string text = http->requestEditor->GetText();
            std::string formatted = glz::prettify_json(text);
            if (!formatted.empty())
            {
                http->requestEditor->SetText(formatted);
                http->req.body = formatted;
                http->body = formatted;
                http->requestEditorVersion = http->requestEditor->GetUndoIndex();
            }
        }
        ImGui::SameLine();
        bool reqWrap = http->requestEditor->IsWordWrapEnabled();
        if (ImGui::Checkbox("Wrap##req_wrap", &reqWrap))
        {
            http->requestEditor->SetWordWrapEnabled(reqWrap);
        }
        ImGui::Separator();

        ImFont* editorFont = UI::GPU::g_MonoFont ? UI::GPU::g_MonoFont : ImGui::GetFont();
        ImGui::PushFont(editorFont);
        http->requestEditor->Render("##request_body_editor", ImVec2(0.0f, 160.0f), ImGuiChildFlags_Borders);
        ImGui::PopFont();

        if (http->requestEditor->GetUndoIndex() != http->requestEditorVersion)
        {
            http->requestEditorVersion = http->requestEditor->GetUndoIndex();
            http->req.body = http->requestEditor->GetText();
            http->body = http->req.body;
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();

        // --- 2. Resizable Horizontal Splitter ---
        Splitter("##hsplitter", &http->requestHeight, &response_height, min_request_h, min_response_h, splitter_thickness);

        // --- 3. Response Subwindow ---
        ImGui::BeginChild("##response_panel", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

        // Subwindow Header
        ImGui::Text("Response");
        if (http->req.gotresponse)
        {
            auto& resp = http->req.getresponse();
            ImGui::SameLine();
            if (resp.response_code >= 200 && resp.response_code < 300)
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.0f), "[%d OK]", resp.response_code);
            else if (resp.response_code >= 400)
                ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.4f, 1.0f), "[%d Error]", resp.response_code);
            else if (resp.response_code > 0)
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "[%d]", resp.response_code);

            ImGui::SameLine();
            float btn_group_width = 330.0f;
            if (ImGui::GetContentRegionAvail().x > btn_group_width)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btn_group_width);
            }

            ImGui::BeginGroup();
            if (ImGui::Button("Format JSON##resp_fmt"))
            {
                std::string formatted = glz::prettify_json(http->responseEditor->GetText());
                if (!formatted.empty())
                {
                    http->responseEditor->SetText(formatted);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy"))
            {
                http->responseEditor->Copy();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear"))
            {
                http->responseEditor->ClearText();
                if (resp.body.has_value()) resp.body.reset();
                http->lastResponseText.clear();
            }
            ImGui::SameLine();
            bool respWrap = http->responseEditor->IsWordWrapEnabled();
            if (ImGui::Checkbox("Wrap##resp_wrap", &respWrap))
            {
                http->responseEditor->SetWordWrapEnabled(respWrap);
            }
            ImGui::SameLine();
            ImGui::Checkbox("Read-only", &responseReadOnly);
            ImGui::EndGroup();
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (http->req.gotresponse)
        {
            auto& resp = http->req.getresponse();
            if (resp.body.has_value())
            {
                http->responseEditor->SetReadOnlyEnabled(responseReadOnly);
                ImFont* editorFont = UI::GPU::g_MonoFont ? UI::GPU::g_MonoFont : ImGui::GetFont();
                ImGui::PushFont(editorFont);
                http->responseEditor->Render("##response_editor", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders);
                ImGui::PopFont();
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Response body is empty.");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No response yet. Send a request to see output.");
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopID();
    }

    struct AutoRegisterHttpDrawer
    {
        AutoRegisterHttpDrawer()
        {
            RegisterHttpResourceDrawer(DrawHttpResource);
        }
    };
    static AutoRegisterHttpDrawer s_autoRegister;
}
