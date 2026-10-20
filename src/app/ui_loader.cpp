#include "ui_loader.h"
#include <iostream>
#include <fstream>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace
{
    bool IsFileFullyWritten(const std::filesystem::path& path)
    {
        std::error_code ec;
        auto sz = std::filesystem::file_size(path, ec);
        if (ec || sz < 16)
            return false;

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;

        char magic[4] = {0};
        file.read(magic, 4);
        if (file.gcount() < 4)
            return false;

#if defined(_WIN32)
        return (magic[0] == 'M' && magic[1] == 'Z');
#elif defined(__APPLE__)
        uint32_t m = *reinterpret_cast<uint32_t*>(magic);
        return (m == 0xfeedface || m == 0xfeedfacf || m == 0xcafebabe || m == 0xbebafeca);
#else
        // ELF magic: 0x7f, 'E', 'L', 'F'
        return (magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F');
#endif
    }
}

std::filesystem::path UILoader::GetExecutableDir()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#else
    return std::filesystem::current_path();
#endif
}

UILoader::~UILoader()
{
    Unload();
    // Clean up current live shadow file if any
    std::error_code ec;
    if (!m_live_lib_path.empty())
    {
        std::filesystem::remove(m_live_lib_path, ec);
    }
}

void UILoader::CleanupStaleFiles()
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(m_base_dir, ec))
    {
        if (entry.is_regular_file())
        {
            std::string name = entry.path().filename().string();
            if (name.find("openrequest_ui_v") != std::string::npos ||
                name.find("openrequest_ui_live") != std::string::npos)
            {
                std::filesystem::remove(entry.path(), ec);
            }
        }
    }
}

bool UILoader::Initialize(const std::filesystem::path& base_dir)
{
    m_base_dir = base_dir;

#if defined(_WIN32)
    const std::string lib_name = "openrequest_ui.dll";
#elif defined(__APPLE__)
    const std::string lib_name = "libopenrequest_ui.dylib";
#else
    const std::string lib_name = "libopenrequest_ui.so";
#endif

    m_source_lib_path = base_dir / lib_name;

    if (!std::filesystem::exists(m_source_lib_path))
    {
        std::cerr << "[UILoader] Dynamic library not found at: " << m_source_lib_path << std::endl;
        return false;
    }

    // Clean up any stale live files left from previous abnormal exits
    CleanupStaleFiles();

    m_last_write_time = std::filesystem::last_write_time(m_source_lib_path);
    m_last_check_time = std::chrono::steady_clock::now();

    return TryLoadVersion(nullptr, /*is_initial=*/true);
}

bool UILoader::TryLoadVersion(const UIContextInfo* info_to_init, bool is_initial)
{
    // Ensure the binary is completely written by the linker before attempting to copy
    if (!IsFileFullyWritten(m_source_lib_path))
    {
        return false;
    }

    // Generate a fresh unique versioned filename (e.g. libopenrequest_ui_v1.so)
    // Using a new file and inode prevents SIGBUS from truncating mapped memory pages
    uint64_t next_version = m_version + 1;
#if defined(_WIN32)
    std::string next_filename = "openrequest_ui_v" + std::to_string(next_version) + ".dll";
#elif defined(__APPLE__)
    std::string next_filename = "libopenrequest_ui_v" + std::to_string(next_version) + ".dylib";
#else
    std::string next_filename = "libopenrequest_ui_v" + std::to_string(next_version) + ".so";
#endif

    std::filesystem::path next_live_path = m_base_dir / next_filename;

    std::error_code ec;
    std::filesystem::copy_file(m_source_lib_path, next_live_path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
    {
        std::cerr << "[UILoader] Failed to create shadow copy: " << ec.message() << std::endl;
        return false;
    }

    // Attempt to load the new library FIRST into temporary variables
    void* new_handle = nullptr;
    UI_InitFn new_init = nullptr;
    UI_RenderFrameFn new_render = nullptr;
    UI_ShutdownFn new_shutdown = nullptr;

#if defined(_WIN32)
    new_handle = LoadLibraryA(next_live_path.string().c_str());
    if (!new_handle)
    {
        std::cerr << "[UILoader] LoadLibrary failed, error code: " << GetLastError() << std::endl;
        std::filesystem::remove(next_live_path, ec);
        return false;
    }
    new_init = reinterpret_cast<UI_InitFn>(GetProcAddress(static_cast<HMODULE>(new_handle), "UI_Init"));
    new_render = reinterpret_cast<UI_RenderFrameFn>(GetProcAddress(static_cast<HMODULE>(new_handle), "UI_RenderFrame"));
    new_shutdown = reinterpret_cast<UI_ShutdownFn>(GetProcAddress(static_cast<HMODULE>(new_handle), "UI_Shutdown"));
#else
    new_handle = dlopen(next_live_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!new_handle)
    {
        std::cerr << "[UILoader] dlopen failed: " << dlerror() << std::endl;
        std::filesystem::remove(next_live_path, ec);
        return false;
    }
    new_init = reinterpret_cast<UI_InitFn>(dlsym(new_handle, "UI_Init"));
    new_render = reinterpret_cast<UI_RenderFrameFn>(dlsym(new_handle, "UI_RenderFrame"));
    new_shutdown = reinterpret_cast<UI_ShutdownFn>(dlsym(new_handle, "UI_Shutdown"));
#endif

    if (!new_init || !new_render)
    {
        std::cerr << "[UILoader] Missing required export symbols in new binary." << std::endl;
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(new_handle));
#else
        dlclose(new_handle);
#endif
        std::filesystem::remove(next_live_path, ec);
        return false;
    }

    // Now safely retire the old library
    if (m_handle)
    {
        if (m_fnShutdown)
        {
            m_fnShutdown();
        }
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(m_handle));
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
    }

    // Clean up previous shadow file
    if (!m_live_lib_path.empty())
    {
        std::filesystem::remove(m_live_lib_path, ec);
    }

    // Commit new version
    m_handle = new_handle;
    m_live_lib_path = next_live_path;
    m_version = next_version;
    m_fnInit = new_init;
    m_fnRenderFrame = new_render;
    m_fnShutdown = new_shutdown;

    if (info_to_init)
    {
        CallInit(*info_to_init);
    }

    return true;
}

bool UILoader::CheckAndReload(const UIContextInfo& info, bool force)
{
    auto now = std::chrono::steady_clock::now();
    // Throttle checks to 150ms
    if (!force && std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_check_time).count() < 150)
    {
        return false;
    }
    m_last_check_time = now;

    std::error_code ec;
    auto current_write_time = std::filesystem::last_write_time(m_source_lib_path, ec);
    if (ec)
    {
        return false;
    }

    if (force || current_write_time > m_last_write_time)
    {
        // Check if file is completely flushed and valid
        if (!IsFileFullyWritten(m_source_lib_path))
        {
            return false;
        }

        std::cout << "[HotReload] Loading updated UI dynamic library..." << std::endl;

        if (TryLoadVersion(&info, /*is_initial=*/false))
        {
            m_last_write_time = current_write_time;
            std::cout << "[HotReload] Successfully live-reloaded UI (version " << m_version << ")!" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "[HotReload] Reload aborted, continuing with current live version." << std::endl;
        }
    }

    return false;
}

void UILoader::Unload()
{
    if (m_handle)
    {
        if (m_fnShutdown)
        {
            m_fnShutdown();
        }
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(m_handle));
#else
        dlclose(m_handle);
#endif
        m_handle = nullptr;
    }
    m_fnInit = nullptr;
    m_fnRenderFrame = nullptr;
    m_fnShutdown = nullptr;
}

void UILoader::CallInit(const UIContextInfo& info)
{
    if (m_fnInit)
    {
        m_fnInit(&info);
    }
}

void UILoader::CallRenderFrame(UI::AppState* state, SDL_Window* window)
{
    if (m_fnRenderFrame)
    {
        m_fnRenderFrame(state, window);
    }
}

void UILoader::CallShutdown()
{
    if (m_fnShutdown)
    {
        m_fnShutdown();
    }
}
