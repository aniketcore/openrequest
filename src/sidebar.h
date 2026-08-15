#pragma once
#include "imgui.h"

namespace UI {

extern float g_SidebarWidth;
extern bool g_SidebarCollapsed;
extern bool g_SidebarRestoreRequested;
void RenderSidebar();

} // namespace UI
