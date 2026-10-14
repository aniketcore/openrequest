#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <memory>
#include "httptypes.h"

struct SDL_Window;


#include "httpengine.h"
#include "TextEditor.h"

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
    float requestHeight = -1.0f;
    
    HTTP::Engine eng;
    HTTP::Request req;
    std::unique_ptr<TextEditor> requestEditor;
    std::unique_ptr<TextEditor> responseEditor;
    std::string lastResponseText;
    bool requestEditorInitialized = false;

    std::vector<std::pair<std::string, std::string>> requestHeaders;

    HttpTab(int id, std::string name) : Tab(id, std::move(name)), method(HTTP::GET) {}

    void Draw() override;
    
    std::string SerializeToString(const std::string& name = "Unnamed");
    void DeserializeFromString(const std::string& serialized);
};

struct SavedRequest {
    std::string name;
    std::string serialized;
};
extern std::vector<SavedRequest> g_Collection;
void SaveCollectionToFile(const std::string& path);
void LoadCollectionFromFile(const std::string& path);

// Global active tab state declarations
extern std::vector<std::unique_ptr<Tab>> g_Tabs;
extern int g_ActiveTabId;
extern int g_NextTabId;
extern ImGuiID g_MainDockId;

void FocusTab(int tab_id);
void CloseTab(int tab_id);

void DrawTopChrome(SDL_Window* window);
void DrawTabContent();

// Top navigation bar state & function
extern int g_SelectedTopTab;
void RenderTopNavigationBar();

} // namespace UI