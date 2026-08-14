# openrequest

> A high-performance, GPU-rendered API client alternative to Postman, written in C++ using Dear ImGui, Vulkan, and GLFW for the interface, and libcurl for asynchronous, non-blocking HTTP networking.

---

## Features

- **GPU-Accelerated UI**: Built on Vulkan and GLFW with Dear ImGui to deliver an extremely responsive, low-latency interface that remains fluid.
- **Non-Blocking Async Engine**: HTTP requests run on background worker threads.

---

## Build and Installation

### Prerequisites

To compile openrequest on Linux, you will need a C++23 compiler, CMake 4.0 (or newer), the Vulkan SDK, GLFW dependencies, and libcurl development headers.

#### Ubuntu/Debian Setup

Install the package dependencies with the following command:

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
    libcurl4-openssl-dev
```

### Compilation Steps

1. **Clone the repository** (along with submodules recursively):
   ```bash
   git clone https://github.com/aniketcore/openrequest --recursive
   cd openrequest
   ```
   *(If you cloned without `--recursive`, run `git submodule update --init --recursive` to pull down Dear ImGui and GLFW)*

2. **Configure with CMake**:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the executable**:
   ```bash
   cmake --build build -j$(nproc)
   ```

4. **Run openrequest**:
   ```bash
   ./build/openrequest
   ```

---

## Repository Structure

Below is an overview of the core project structure, with direct links to the code implementation:

- [src/](src/)
  - [main.cpp](src/main.cpp) — Application entry point, spawning the UI loop thread.
  - [ui_loop.cpp](src/ui_loop.cpp) / [ui_loop.h](src/ui_loop.h) — Manages the GLFW window lifecycle, Vulkan pipeline, and draws the primary frame.
  - [sidebar.cpp](src/sidebar.cpp) / [sidebar.h](src/sidebar.h) — Renders the sidebar with dynamic tab list additions, deletions, and rename fields.
  - [tab.cpp](src/tab.cpp) / [tab.h](src/tab.h) — Renders main layout sections, active tab views (HTTP details, WebSocket settings), and top sub-navigation bar.
  - [httpengine.h](src/httpengine.h) — Multi-threaded HTTP dispatcher built on libcurl.
  - [httptypes.h](src/httptypes.h) — Standardized structs for requests, responses, and HTTP methods.
  - [imgui_helpers.h](src/imgui_helpers.h) — Helper functions wrapping Vulkan instance, physical device, swapchain initialization, and frame rendering.
- [ext/](ext/) — External Git submodules:
  - [glfw](ext/glfw/) (Window/Input/OS abstraction)
  - [imgui](ext/imgui/) (Immediate mode GUI toolkit)

---

## Roadmap

- [ ] **Full WebSocket Client**: Implement connection management, frame send/receive loops, and JSON payload handling.
- [ ] **Custom Headers & Parameters**: Add interactive tables to customize HTTP request headers and query parameters dynamically.
- [ ] **Syntax Highlighting**: Integrate custom coloring or Markdown rendering for JSON, XML, and HTML response bodies.
- [ ] **Request History / Collections**: Enable filesystem storage or local SQLite DB to save past requests and organize collections.
- [ ] **Environment Variables**: Add environment scoping (e.g. `{{host}}`) for rapid endpoint testing.

---

## License

This project is open-source and licensed under the MIT License.