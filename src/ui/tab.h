#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <memory>
#include "resource.h"
#include "http_resource.h"

struct SDL_Window;

namespace UI {

// Alias for transition / backward compatibility
using HttpTab = ::HttpResource;

// Tab is purely the UI view construct
class Tab {
public:
    int id;
    std::string name;
    std::shared_ptr<Resource> resource;

    Tab(int id, std::string name, std::shared_ptr<Resource> resource = nullptr)
        : id(id), name(std::move(name)), resource(std::move(resource)) {}

    void Draw() {
        if (resource) {
            resource->Draw();
        }
    }
};

// Global active tab state declarations
extern std::vector<std::unique_ptr<Tab>> g_Tabs;
extern int g_ActiveTabId;
extern int g_NextTabId;
extern ImGuiID g_MainDockId;

void FocusTab(int tab_id);
void CloseTab(int tab_id);
void OpenOrFocusResource(const std::shared_ptr<Resource>& res);

void DrawTopChrome(SDL_Window* window);
void DrawTabContent();

// Top navigation bar state & function
extern int g_SelectedTopTab;
void RenderTopNavigationBar();

} // namespace UI
