#include "http_resource.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <imgui_stdlib.h>
#include "imgui_internal.h"

#include "httpengine.h"
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

void HttpResource::Draw()
{
    ImGui::PushID(this);
    static bool responseReadOnly = true;

    if (!requestEditor)
    {
        requestEditor = std::make_unique<TextEditor>();
        ConfigureTextEditor(*requestEditor, false, TextEditor::Language::Json());
    }
    if (!responseEditor)
    {
        responseEditor = std::make_unique<TextEditor>();
        ConfigureTextEditor(*responseEditor, true, TextEditor::Language::Json());
    }

    if (!requestEditorInitialized)
    {
        requestEditor->SetText(req.body);
        requestEditorInitialized = true;
    }

    if (req.gotresponse && req.getresponse().body.has_value())
    {
        const auto& body = req.getresponse().body.value();
        if (body != lastResponseText)
        {
            responseEditor->SetText(body);
            lastResponseText = body;
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    const float splitter_thickness = 6.0f;
    const float min_request_h = 100.0f;
    const float min_response_h = 100.0f;
    float total_avail_y = ImGui::GetContentRegionAvail().y;

    if (this->requestHeight < 0.0f)
    {
        this->requestHeight = total_avail_y * 0.50f;
    }

    float max_request_h = total_avail_y - min_response_h - splitter_thickness;
    if (max_request_h < min_request_h)
        max_request_h = min_request_h;

    this->requestHeight = std::clamp(this->requestHeight, min_request_h, max_request_h);
    float response_height = total_avail_y - this->requestHeight - splitter_thickness;
    if (response_height < min_response_h)
        response_height = min_response_h;

    // --- 1. Request Subwindow ---
    ImGui::BeginChild("##request_panel", ImVec2(0.0f, this->requestHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

    ImGui::Text("HTTP Request Settings");
    ImGui::Spacing();
    ImGui::InputText("URL", &this->url, 0, nullptr, nullptr);

    const char* methods[] = {"GET", "POST", "PUT", "DELETE"};
    int current_method = static_cast<int>(this->method);
    if (ImGui::Combo("Method", &current_method, methods, IM_ARRAYSIZE(methods)))
    {
        this->method = static_cast<HTTP::Method>(current_method);
    }

    ImGui::Spacing();
    ImGui::Text("Headers");
    ImGui::SameLine();
    if (ImGui::Button("+##add_header"))
    {
        requestHeaders.push_back({"", ""});
    }
    for (size_t i = 0; i < requestHeaders.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##key", &requestHeaders[i].first);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputText("##value", &requestHeaders[i].second);
        ImGui::SameLine();
        if (ImGui::Button("X"))
        {
            requestHeaders.erase(requestHeaders.begin() + i);
            --i;
        }
        ImGui::PopID();
    }
    ImGui::Spacing();
    ImGui::Text("Request Body");
    ImGui::Separator();

    ImFont* editorFont = UI::GPU::g_MonoFont ? UI::GPU::g_MonoFont : ImGui::GetFont();
    ImGui::PushFont(editorFont);
    requestEditor->Render("##request_body_editor", ImVec2(-1.0f, 160.0f), true);
    ImGui::PopFont();

    req.body = requestEditor->GetText();
    ImGui::Spacing();

    if (ImGui::Button("Send Request"))
    {
        req.url = this->url;
        req.method = this->method;
        req.gotresponse = false;
        req.body = requestEditor->GetText();

        req.headers.clear();
        for (const auto& h : requestHeaders)
        {
            if (!h.first.empty())
            {
                req.headers[h.first] = h.second;
            }
        }

        eng.dispatchrequest(&req);
        std::cout << "button pressed" << std::endl;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    // --- 2. Resizable Horizontal Splitter ---
    Splitter("##hsplitter", &this->requestHeight, &response_height, min_request_h, min_response_h, splitter_thickness);

    // --- 3. Response Subwindow ---
    ImGui::BeginChild("##response_panel", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

    // Subwindow Header
    ImGui::Text("Response");
    if (req.gotresponse)
    {
        auto& resp = req.getresponse();
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
            responseEditor->Copy();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            responseEditor->ClearText();
            if (resp.body.has_value()) resp.body.reset();
            lastResponseText.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Read-only", &responseReadOnly);
        ImGui::EndGroup();
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (req.gotresponse)
    {
        auto& resp = req.getresponse();
        if (resp.body.has_value())
        {
            responseEditor->SetReadOnlyEnabled(responseReadOnly);
            ImFont* editorFont = UI::GPU::g_MonoFont ? UI::GPU::g_MonoFont : ImGui::GetFont();
            ImGui::PushFont(editorFont);
            responseEditor->Render("##response_editor", ImGui::GetContentRegionAvail(), true);
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
