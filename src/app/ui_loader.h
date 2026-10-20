#pragma once
#include <filesystem>
#include <chrono>
#include <string>
#include "ui_interface.h"

class UILoader {
public:
    UILoader() = default;
    ~UILoader();

    static std::filesystem::path GetExecutableDir();

    bool Initialize(const std::filesystem::path& base_dir);
    bool CheckAndReload(const UIContextInfo& info, bool force = false);
    void Unload();

    bool IsLoaded() const { return m_handle != nullptr; }

    void CallInit(const UIContextInfo& info);
    void CallRenderFrame(UI::AppState* state, SDL_Window* window);
    void CallShutdown();

private:
    bool TryLoadVersion(const UIContextInfo* info_to_init, bool is_initial);
    void CleanupStaleFiles();

    void* m_handle = nullptr;
    std::filesystem::path m_base_dir;
    std::filesystem::path m_source_lib_path;
    std::filesystem::path m_live_lib_path;
    std::filesystem::file_time_type m_last_write_time{};
    std::chrono::steady_clock::time_point m_last_check_time{};
    uint64_t m_version = 0;

    UI_InitFn m_fnInit = nullptr;
    UI_RenderFrameFn m_fnRenderFrame = nullptr;
    UI_ShutdownFn m_fnShutdown = nullptr;
};
