#pragma once

struct GLFWwindow;

namespace UI
{
    // Initiates native OS window drag (e.g. _NET_WM_MOVERESIZE on X11, WM_NCLBUTTONDOWN on Win32).
    // Returns true if a native drag request was initiated.
    bool NativeStartWindowDrag(GLFWwindow* window);
}
