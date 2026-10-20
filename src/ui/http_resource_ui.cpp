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

    void ConfigureTextEditor(TextEditor& editor, bool readOnly, const TextEditor::Language* language = nullptr)
    {
        editor.SetTabSize(2);
        editor.SetInsertSpacesOnTabs(true);
        editor.SetAutoIndentEnabled(true);
        editor.SetReadOnlyEnabled(readOnly);
        editor.SetShowLineNumbersEnabled(true);
        editor.SetShowWhitespacesEnabled(false);
        editor.SetShowMatchingBrackets(true);
        editor.SetCompletePairedGlyphs(true);
        editor.SetShowPanScrollIndicatorEnabled(true);
        if (language != nullptr)
        {
            editor.SetLanguage(language);
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
        }

        if (http->req.gotresponse && http->req.getresponse().body.has_value())
        {
            const auto& body = http->req.getresponse().body.value();
            if (body != http->lastResponseText)
            {
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

        ImGui::Text("HTTP Request Settings");
        ImGui::Spacing();
        ImGui::InputText("URL", &http->url, 0, nullptr, nullptr);

        const char* methods[] = {"GET", "POST", "PUT", "DELETE"};
        int current_method = static_cast<int>(http->method);
        if (ImGui::Combo("Method", &current_method, methods, IM_ARRAYSIZE(methods)))
        {
            http->method = static_cast<HTTP::Method>(current_method);
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
        ImGui::Separator();

        ImFont* editorFont = UI::GPU::g_MonoFont ? UI::GPU::g_MonoFont : ImGui::GetFont();
        ImGui::PushFont(editorFont);
        http->requestEditor->Render("##request_body_editor", ImVec2(-1.0f, 160.0f), true);
        ImGui::PopFont();

        http->req.body = http->requestEditor->GetText();
        http->body = http->req.body;
        ImGui::Spacing();

        if (ImGui::Button("Send Request"))
        {
            http->SendRequest();
            std::cout << "button pressed" << std::endl;
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
            float btn_group_width = 230.0f;
            if (ImGui::GetContentRegionAvail().x > btn_group_width)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btn_group_width);
            }

            ImGui::BeginGroup();
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
                http->responseEditor->Render("##response_editor", ImGui::GetContentRegionAvail(), true);
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
