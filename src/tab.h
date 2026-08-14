#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include "httptypes.h"

namespace UI {

struct Tab {
    int id;
    std::string name;
    enum TabType {
        HTTP, WS
    };
    TabType type;
    std::function<void(Tab &)> drawFunc;\
    HTTP::Instance instance;

    Tab(int id, std::string name, TabType type,std::function<void(Tab &)>drawFunc):id(id),name(name),type(type),drawFunc(drawFunc){
    }
};
// Global active tab state declarations
extern std::vector<Tab> g_Tabs;
extern int g_ActiveTabId;
extern int g_NextTabId;

void DrawHttpTab(Tab &tab);
void DrawWsTab(Tab &tab);
void DrawTabContent();

// Top navigation bar state & function
extern int g_SelectedTopTab;
void RenderTopNavigationBar();

} // namespace UI