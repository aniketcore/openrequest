# openrequest v0.1.0 — Initial Release

I built openrequest because I got tired of waiting several seconds for heavy, multi-gigabyte Electron API clients to load just to test a single HTTP endpoint.

This is the first public release. It is a native desktop application written in modern C++ (C++23), rendered directly with Vulkan and Dear ImGui, with networking handled by libcurl. It launches almost instantly, consumes very little RAM, and keeps the UI responsive even during long-running requests.

---

### What works right now

* **Fast and lightweight**: No embedded Chromium, no background node processes, and zero analytics or account requirements.
* **Dockable layout**: You can split tabs horizontally or vertically, drag them side-by-side, or dock them anywhere in the window to compare endpoints and responses.
* **Non-blocking requests**: All HTTP requests run on background worker threads so the interface never stutters or freezes while waiting for a response.
* **Core HTTP testing**: Supports standard methods (GET, POST, PUT, DELETE, PATCH, etc.) with custom headers, query params, and body payloads.
* **Integrated code view**: Syntax editor for viewing responses and editing request bodies.
* **Collection saving**: Save and reload your requests locally to disk (`.orq` files).
* **UI Scaling**: Supports high-DPI displays and runtime zooming via `Ctrl +` / `Ctrl -` or `Ctrl + MouseWheel`.

---

### Downloads and Running

Portable tarballs for both platforms are attached below under Assets.

#### Linux (`openrequest-linux-x86_64.tar.gz`)
Requires standard Vulkan drivers installed on your system. Extract and run the bundled launcher:
```bash
tar -xzf openrequest-linux-x86_64.tar.gz
cd openrequest
./openrequest
```

#### Windows (`openrequest-windows-x86_64.tar.gz`)
Requires Windows 10 or 11 with Vulkan-capable graphics drivers:
```powershell
tar -xzf openrequest-windows-x86_64.tar.gz
cd openrequest
.\openrequest.exe
```

---

### Building from Source

If you prefer building from source, the project uses CMake and vcpkg:
* Windows (MSVC + vcpkg): `cmake --preset windows-vcpkg && cmake --build --preset windows-vcpkg --config Release`
* Linux (system packages): `cmake --preset linux-system && cmake --build --preset linux-system`

See the README for prerequisite package lists.

---

### What is coming next

A few features were intentionally left out of this initial build to keep it stable:
* Full WebSocket testing (the stub tab was removed for this release and will return once properly implemented)
* JSON and XML auto-formatting
* Postman collection import
* Environment variable templating (e.g. `{{base_url}}`)

Feedback and bug reports are welcome on the issues page.

