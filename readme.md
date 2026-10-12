# openrequest

A fast, lightweight, native API client for developers who want a quick, responsive tool without the overhead of Electron.

Built in C++ with Dear ImGui, Vulkan, and libcurl. It starts up instantly, uses minimal memory, and stays completely responsive while executing requests.

---

## Highlights

- **Instant startup & low memory footprint**: Native desktop application rendered with Vulkan — no Chromium runtime or background service bloat.
- **Flexible dockable workspace**: Open multiple requests at once. Drag tabs to reorder them, dock them side-by-side, or split panels however you like using Dear ImGui's docking engine.
- **Async networking**: Requests run on background worker threads via `libcurl`, so slow endpoints or large downloads never freeze your UI.
- **HTTP & WebSocket support**: Test all standard HTTP methods (GET, POST, PUT, DELETE, PATCH, etc.) with configurable query parameters, headers, and request bodies. Includes tab support for WebSocket testing.
- **Local & private**: Runs entirely on your machine. No mandatory accounts, no cloud sync, and no analytics tracking.

---

## Getting Started

### Prerequisites (Ubuntu / Debian)

Make sure you have a C++23 capable compiler, CMake, Vulkan drivers/headers, GLFW dependencies, and libcurl:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libvulkan-dev \
    vulkan-tools \
    xorg-dev \
    libwayland-dev \
    libxkbcommon-dev \
    libcurl4-openssl-dev \
    libfreetype6-dev
```

### Build & Run

1. **Clone the repository** (including submodules for ImGui and GLFW):
   ```bash
   git clone --recursive https://github.com/aniketcore/openrequest.git
   cd openrequest
   ```
   *(If already cloned without `--recursive`, run `git submodule update --init --recursive`)*

2. **Configure with CMake**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build**:
   ```bash
   cmake --build build -j$(nproc)
   ```

4. **Launch**:
   ```bash
   ./build/openrequest
   ```

---

## Workspace & Tips

- **Tabs**: Click **+** or right-click in the sidebar to create new HTTP or WebSocket tabs. You can close tabs with the `x` button or right-click to rename them.
- **Split layouts**: Drag any tab header to dock it next to, above, or below another tab. This makes comparing request payloads and responses side-by-side effortless.
- **Sending requests**: Choose your method, type the endpoint, set your headers/params/body, and press **Send**.

---

## Roadmap

- [ ] Syntax highlighting & auto-formatting for JSON and XML responses
- [ ] Save / export request collections (with Postman collection import)
- [ ] Environment variable templating (e.g. `{{base_url}}`, `{{token}}`)
- [ ] Full WebSocket message framing & interactive streaming log
- [ ] Keyboard shortcuts (Ctrl+Enter to send, Ctrl+T for new tab, Ctrl+W to close)

---

## License

This project is licensed under the [MIT License](LICENSE).