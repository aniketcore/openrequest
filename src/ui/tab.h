#pragma once
#include "app_state.h"

struct SDL_Window;

namespace UI {

// Alias for transition / backward compatibility
using HttpTab = ::HttpResource;

void FocusTab(int tab_id);
void CloseTab(int tab_id);
void OpenOrFocusResource(const std::shared_ptr<Resource>& res);

void DrawTopChrome(SDL_Window* window);
void DrawTabContent();
void RenderTopNavigationBar();

} // namespace UI
