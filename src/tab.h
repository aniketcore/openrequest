#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <memory>
#include "httptypes.h"

namespace UI {

class Tab {
public:
    int id;
    std::string name;

    Tab(int id, std::string name) : id(id), name(std::move(name)) {}
    virtual ~Tab() = default;

    virtual void Draw() = 0;
};

class HttpTab : public Tab {
public:
    std::string url;
    HTTP::Method method;
    float requestHeight = 150.0f;

    HttpTab(int id, std::string name) : Tab(id, std::move(name)), method(HTTP::GET) {}

    void Draw() override;
};

class WsTab : public Tab {
public:
    std::string address;

    WsTab(int id, std::string name) : Tab(id, std::move(name)), address("wss://echo.websocket.org") {}

    void Draw() override;
};

// Global active tab state declarations
extern std::vector<std::unique_ptr<Tab>> g_Tabs;
extern int g_ActiveTabId;
extern int g_NextTabId;

void DrawTabContent();

// Top navigation bar state & function
extern int g_SelectedTopTab;
void RenderTopNavigationBar();

} // namespace UI