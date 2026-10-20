#pragma once
#include "app_state.h"

struct SDL_Window;

namespace UI {

// Alias for transition / backward compatibility
using HttpTab = ::HttpResource;

// Global active tab state references (pointing directly into g_AppState)
extern std::vector<std::unique_ptr<Tab>>& g_Tabs;
extern int& g_ActiveTabId;
extern int& g_NextTabId;
extern ImGuiID& g_MainDockId;
extern int& g_SelectedTopTab;

void FocusTab(int tab_id);
void CloseTab(int tab_id);
void OpenOrFocusResource(const std::shared_ptr<Resource>& res);

void DrawTopChrome(SDL_Window* window);
void DrawTabContent();
void RenderTopNavigationBar();

} // namespace UI
