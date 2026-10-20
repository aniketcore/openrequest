#include "http_resource.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

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

    void ConfigureTextEditor(TextEditor& editor, bool readOnly, const TextEditor::Language* language = nullptr)
    {
        editor.SetPalette(TextEditor::GetDarkPalette());
        editor.SetTabSize(2);
        editor.SetInsertSpacesOnTabs(true);
        editor.SetAutoIndentEnabled(true);
        editor.SetReadOnlyEnabled(readOnly);
        editor.SetShowLineNumbersEnabled(true);
        editor.SetLineFoldingEnabled(true);
        editor.SetLineSpacing(1.25f);
        if (language != nullptr)
        {
            editor.SetLanguage(language);
        }
    }

    ImVec4 GetMethodColor(HTTP::Method method)
    {
        switch (method)
        {
            case HTTP::GET:    return ImVec4(0.18f, 0.80f, 0.44f, 1.0f); // Bright Green
            case HTTP::POST:   return ImVec4(0.85f, 0.72f, 0.38f, 1.0f); // Warm Gold / Yellow
            case HTTP::PUT:    return ImVec4(0.28f, 0.62f, 0.95f, 1.0f); // Blue
            case HTTP::DELETE: return ImVec4(0.92f, 0.32f, 0.32f, 1.0f); // Red
            default:           return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
        }
    }

    struct QueryParam {
        std::string key;
        std::string value;
    };

    std::vector<QueryParam> ParseQueryParams(const std::string& url)
    {
        std::vector<QueryParam> params;
        size_t qpos = url.find('?');
        if (qpos == std::string::npos) return params;
        std::string qstr = url.substr(qpos + 1);
        std::istringstream iss(qstr);
        std::string pair;
        while (std::getline(iss, pair, '&'))
        {
            if (pair.empty()) continue;
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
                params.push_back({pair.substr(0, eq), pair.substr(eq + 1)});
            else
                params.push_back({pair, ""});
        }
        return params;
    }

    std::string RebuildUrlWithParams(const std::string& url, const std::vector<QueryParam>& params)
    {
        size_t qpos = url.find('?');
        std::string base = (qpos != std::string::npos) ? url.substr(0, qpos) : url;
        if (params.empty()) return base;
        std::string new_url = base + "?";
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i > 0) new_url += "&";
            new_url += params[i].key;
            if (!params[i].value.empty())
            {
                new_url += "=";
                new_url += params[i].value;
            }
        }
        return new_url;
    }

    std::string Base64Encode(const std::string& in)
    {
        static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in)
        {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0)
            {
                out.push_back(lookup[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    void SetHeader(std::vector<std::pair<std::string, std::string>>& headers, const std::string& key, const std::string& val)
    {
        for (auto& h : headers)
        {
            if (h.first == key)
            {
                h.second = val;
                return;
            }
        }
        headers.push_back({key, val});
    }

    void RemoveHeader(std::vector<std::pair<std::string, std::string>>& headers, const std::string& key)
    {
        for (auto it = headers.begin(); it != headers.end(); ++it)
        {
            if (it->first == key)
            {
                headers.erase(it);
                return;
            }
        }
    }

    struct ResourceSubtabState {
        int authType = 0; // 0 = None, 1 = Bearer, 2 = Basic
        std::string bearerToken;
        std::string basicUser;
        std::string basicPass;
        std::string docs;
        bool followRedirects = true;
        bool verifySsl = true;
        int timeoutMs = 30000;
        bool defaultTabApplied = false;
    };
    static std::unordered_map<uint64_t, ResourceSubtabState> s_SubtabStates;

    void DrawRequestBar(HttpResource* http)
    {
        const float send_btn_w = 80.0f;
        const float method_w = 90.0f;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float avail_w = ImGui::GetContentRegionAvail().x;
        const float url_w = std::max(100.0f, avail_w - method_w - send_btn_w - (spacing * 2.0f));

        // 1. Built-in Method Combo
        const char* methods[] = { "GET", "POST", "PUT", "DELETE" };
        int current_method = static_cast<int>(http->method);

        ImGui::SetNextItemWidth(method_w);
        ImGui::PushStyleColor(ImGuiCol_Text, GetMethodColor(http->method));
        if (ImGui::BeginCombo("##method", methods[current_method]))
        {
            for (int n = 0; n < IM_ARRAYSIZE(methods); n++)
            {
                bool is_selected = (current_method == n);
                ImGui::PushStyleColor(ImGuiCol_Text, GetMethodColor(static_cast<HTTP::Method>(n)));
                if (ImGui::Selectable(methods[n], is_selected))
                {
                    http->method = static_cast<HTTP::Method>(n);
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                ImGui::PopStyleColor();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();

        // 2. Built-in URL Input
        ImGui::SameLine();
        ImGui::SetNextItemWidth(url_w);
        bool enter_pressed = ImGui::InputTextWithHint("##url_input", "Enter URL or paste text", &http->url, ImGuiInputTextFlags_EnterReturnsTrue);

        // 3. Built-in Send / Cancel Button
        ImGui::SameLine();
        bool is_running = http->IsRunning();
        bool send_clicked = false;

        if (is_running)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.82f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.30f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
            if (ImGui::Button("Cancel##send_btn", ImVec2(send_btn_w, 0.0f)))
            {
                http->CancelRequest();
            }
            ImGui::PopStyleColor(3);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.40f, 0.80f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.48f, 0.88f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.35f, 0.72f, 1.0f));
            if (ImGui::Button("Send##send_btn", ImVec2(send_btn_w, 0.0f)) || enter_pressed)
            {
                send_clicked = true;
            }
            ImGui::PopStyleColor(3);
        }

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

        ImGui::Dummy(ImVec2(0.0f, 4.0f));

        auto params = ParseQueryParams(http->url);
        auto& subtabState = s_SubtabStates[http->id];

        // Modern, clean tab strip matching API clients (Postman/Bruno)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 7.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_TabBarOverlineSize, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1.0f, 1.0f, 1.0f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));

        if (ImGui::BeginTabBar("##request_subtabs", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip))
        {
            // 1. Docs
            const char* docs_label = subtabState.docs.empty() ? "Docs###tab_docs" : "Docs *###tab_docs";
            if (ImGui::BeginTabItem(docs_label))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                ImGui::TextDisabled("Request Documentation & Notes");
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::InputTextMultiline("##doc_notes", &subtabState.docs, ImVec2(-FLT_MIN, -FLT_MIN));

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            // 2. Params
            char params_label[64];
            if (!params.empty())
                snprintf(params_label, sizeof(params_label), "Params %zu###tab_params", params.size());
            else
                snprintf(params_label, sizeof(params_label), "Params###tab_params");

            if (ImGui::BeginTabItem(params_label))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                if (ImGui::Button("+ Add Param##add_param"))
                {
                    params.push_back({"", ""});
                    http->url = RebuildUrlWithParams(http->url, params);
                }

                ImGui::Dummy(ImVec2(0.0f, 2.0f));

                if (!params.empty())
                {
                    if (ImGui::BeginTable("##params_table", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 0.40f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.52f);
                        ImGui::TableSetupColumn("##action", ImGuiTableColumnFlags_WidthFixed, 32.0f);
                        ImGui::TableHeadersRow();

                        bool modified = false;
                        for (size_t i = 0; i < params.size(); ++i)
                        {
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            if (ImGui::InputTextWithHint("##param_key", "Parameter name", &params[i].key))
                                modified = true;

                            ImGui::TableSetColumnIndex(1);
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            if (ImGui::InputTextWithHint("##param_val", "Value", &params[i].value))
                                modified = true;

                            ImGui::TableSetColumnIndex(2);
                            if (ImGui::Button("X##del", ImVec2(28.0f, 0.0f)))
                            {
                                params.erase(params.begin() + i);
                                --i;
                                modified = true;
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndTable();

                        if (modified)
                        {
                            http->url = RebuildUrlWithParams(http->url, params);
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("No query parameters. Click '+ Add Param' or add '?key=value' in URL.");
                }

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            // 3. Authorization
            const char* auth_label = (subtabState.authType == 0) ? "Authorization###tab_auth" : "Authorization *###tab_auth";
            if (ImGui::BeginTabItem(auth_label))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                ImGui::Text("Auth Type");
                ImGui::SameLine();
                static const char* auth_types[] = { "No Auth", "Bearer Token", "Basic Auth" };
                ImGui::SetNextItemWidth(180.0f);
                if (ImGui::Combo("##auth_type", &subtabState.authType, auth_types, IM_ARRAYSIZE(auth_types)))
                {
                    if (subtabState.authType == 0)
                        RemoveHeader(http->requestHeaders, "Authorization");
                    else if (subtabState.authType == 1 && !subtabState.bearerToken.empty())
                        SetHeader(http->requestHeaders, "Authorization", "Bearer " + subtabState.bearerToken);
                    else if (subtabState.authType == 2 && (!subtabState.basicUser.empty() || !subtabState.basicPass.empty()))
                        SetHeader(http->requestHeaders, "Authorization", "Basic " + Base64Encode(subtabState.basicUser + ":" + subtabState.basicPass));
                }

                ImGui::Dummy(ImVec2(0.0f, 4.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                if (subtabState.authType == 1) // Bearer
                {
                    ImGui::Text("Token");
                    ImGui::SetNextItemWidth(380.0f);
                    if (ImGui::InputTextWithHint("##bearer_token", "Enter bearer token...", &subtabState.bearerToken))
                    {
                        if (!subtabState.bearerToken.empty())
                            SetHeader(http->requestHeaders, "Authorization", "Bearer " + subtabState.bearerToken);
                        else
                            RemoveHeader(http->requestHeaders, "Authorization");
                    }
                }
                else if (subtabState.authType == 2) // Basic
                {
                    ImGui::Text("Username");
                    ImGui::SetNextItemWidth(320.0f);
                    if (ImGui::InputTextWithHint("##basic_user", "Username", &subtabState.basicUser))
                    {
                        SetHeader(http->requestHeaders, "Authorization", "Basic " + Base64Encode(subtabState.basicUser + ":" + subtabState.basicPass));
                    }
                    ImGui::Text("Password");
                    ImGui::SetNextItemWidth(320.0f);
                    if (ImGui::InputTextWithHint("##basic_pass", "Password", &subtabState.basicPass, ImGuiInputTextFlags_Password))
                    {
                        SetHeader(http->requestHeaders, "Authorization", "Basic " + Base64Encode(subtabState.basicUser + ":" + subtabState.basicPass));
                    }
                }
                else
                {
                    ImGui::TextDisabled("This request does not use any authorization.");
                }

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            // 4. Headers
            char headers_label[64];
            if (!http->requestHeaders.empty())
                snprintf(headers_label, sizeof(headers_label), "Headers %zu###tab_headers", http->requestHeaders.size());
            else
                snprintf(headers_label, sizeof(headers_label), "Headers###tab_headers");

            if (ImGui::BeginTabItem(headers_label))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                if (ImGui::Button("+ Add Header##add_header"))
                {
                    http->requestHeaders.push_back({"", ""});
                }

                ImGui::Dummy(ImVec2(0.0f, 2.0f));

                if (!http->requestHeaders.empty())
                {
                    if (ImGui::BeginTable("##headers_table", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 0.40f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.52f);
                        ImGui::TableSetupColumn("##action", ImGuiTableColumnFlags_WidthFixed, 32.0f);
                        ImGui::TableHeadersRow();

                        for (size_t i = 0; i < http->requestHeaders.size(); ++i)
                        {
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            ImGui::InputTextWithHint("##key", "Header name", &http->requestHeaders[i].first);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            ImGui::InputTextWithHint("##value", "Value", &http->requestHeaders[i].second);

                            ImGui::TableSetColumnIndex(2);
                            if (ImGui::Button("X##del", ImVec2(28.0f, 0.0f)))
                            {
                                http->requestHeaders.erase(http->requestHeaders.begin() + i);
                                --i;
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                }
                else
                {
                    ImGui::TextDisabled("No headers configured. Click '+ Add Header' to add one.");
                }

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            // 5. Body
            const char* body_label = !http->body.empty() ? "Body *###tab_body" : "Body###tab_body";
            ImGuiTabItemFlags body_flags = 0;
            if (!subtabState.defaultTabApplied)
            {
                subtabState.defaultTabApplied = true;
                if (http->method == HTTP::POST || http->method == HTTP::PUT)
                {
                    body_flags |= ImGuiTabItemFlags_SetSelected;
                }
            }

            if (ImGui::BeginTabItem(body_label, nullptr, body_flags))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                if (ImGui::Button("Format JSON##req_fmt"))
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
                if (ImGui::Checkbox("Wrap Lines##req_wrap", &reqWrap))
                {
                    http->requestEditor->SetWordWrapEnabled(reqWrap);
                }

                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                ImFont* editorFont = UI::GPU::g_MonoFont ? UI::GPU::g_MonoFont : ImGui::GetFont();
                ImGui::PushFont(editorFont);
                http->requestEditor->Render("##request_body_editor", ImVec2(0.0f, -FLT_MIN), ImGuiChildFlags_Borders);
                ImGui::PopFont();

                if (http->requestEditor->GetUndoIndex() != http->requestEditorVersion)
                {
                    http->requestEditorVersion = http->requestEditor->GetUndoIndex();
                    http->req.body = http->requestEditor->GetText();
                    http->body = http->req.body;
                }

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            // 6. Scripts
            if (ImGui::BeginTabItem("Scripts###tab_scripts"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                ImGui::TextDisabled("Pre-request & Test Scripts");
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::TextWrapped("Write automated test assertions and pre-request scripts here (coming soon).");

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            // 7. Settings
            if (ImGui::BeginTabItem("Settings###tab_settings"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                ImGui::Checkbox("Follow Redirects", &subtabState.followRedirects);
                ImGui::Checkbox("Verify SSL Certificate", &subtabState.verifySsl);
                ImGui::Dummy(ImVec2(0.0f, 4.0f));
                ImGui::Text("Request Timeout");
                ImGui::SetNextItemWidth(160.0f);
                ImGui::InputInt("##timeout_ms", &subtabState.timeoutMs, 1000, 5000);
                ImGui::SameLine();
                ImGui::TextDisabled("(ms)");

                ImGui::PopStyleVar(2);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(6);
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
            if (ImGui::SmallButton("Format JSON##resp_fmt"))
            {
                std::string formatted = glz::prettify_json(http->responseEditor->GetText());
                if (!formatted.empty())
                {
                    http->responseEditor->SetText(formatted);
                }
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Copy"))
            {
                http->responseEditor->Copy();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear"))
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
