#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <variant>
#include <algorithm>
#include <iostream>
#include <imgui.h>
#include <glaze/yaml.hpp>
#include "resource.h"
#include "http_resource.h"

#ifdef DELETE
#undef DELETE
#endif

using ResourceVariant = std::variant<HttpResource>;

template <>
struct glz::meta<ResourceVariant>
{
    static constexpr std::string_view tag = "$kind";
    static constexpr auto ids = std::array{"HTTP"};
};

struct Collection {
    std::string name = "OpenRequest Collection";
    std::string version = "1.0";
    std::vector<ResourceVariant> items;
    std::vector<uint64_t> open_tabs;
    uint64_t active_tab = 0;
};

template <>
struct glz::meta<Collection> {
    using T = Collection;
    static constexpr auto value = object(
        "name", &T::name,
        "version", &T::version,
        "items", &T::items,
        "open_tabs", &T::open_tabs,
        "active_tab", &T::active_tab
    );
};

namespace UI {

uint64_t GenerateUID();

class Tab {
public:
    int id = 0;
    std::string name;
    std::shared_ptr<Resource> resource;

    Tab(int id, std::string name, std::shared_ptr<Resource> resource = nullptr)
        : id(id), name(std::move(name)), resource(std::move(resource)) {}

    void Draw() {
        if (resource) {
            resource->Draw();
        }
    }
};

struct AppState {
    // Collection metadata
    std::string collection_name = "OpenRequest Collection";
    std::string collection_version = "1.0";
    std::string collection_file_path = "collection.yaml";

    // Master list of resources owned by the active collection
    std::vector<std::shared_ptr<Resource>> resources;

    // Open editor tabs
    std::vector<std::unique_ptr<Tab>> tabs;
    int active_tab_id = -1;
    int next_tab_id = 1;
    int tab_to_focus = -1;

    // UI & Layout state
    ImGuiID main_dock_id = 0;
    int selected_top_tab = 0;
    bool show_sidebar = true;
    // Window chrome & drag area metrics (for borderless window hit testing)
    float title_bar_height = 35.0f;
    float drag_min_x = 0.0f;
    float drag_max_x = 0.0f;
    float max_btn_min_x = 0.0f;
    float max_btn_max_x = 0.0f;
    bool  max_btn_hovered = false;

    // Inline renaming in sidebar
    uint64_t renaming_resource_id = 0;
    uint64_t renaming_resource_id_prev = 0;

    AppState();

    // Reset or seed default state
    void InitializeDefault();

    // Resource operations
    void AddResource(std::shared_ptr<Resource> res);
    void DeleteResource(const std::shared_ptr<Resource>& res);
    std::shared_ptr<Resource> FindResourceById(uint64_t id) const;
    int FindResourceIndexById(uint64_t id) const;

    // Tab operations
    Tab* GetActiveTab() const;
    Tab* FindTabById(int tab_id) const;
    Tab* FindTabByResource(const std::shared_ptr<Resource>& res) const;
    void FocusTab(int tab_id);
    void CloseTab(int tab_id);
    void OpenOrFocusResource(const std::shared_ptr<Resource>& res);

    // Serialization & Persistence
    Collection ToCollection() const;
    void FromCollection(Collection col);
    bool SaveCollection(const std::string& path = "");
    bool LoadCollection(const std::string& path = "");
    std::string ToYamlString() const;
    bool FromYamlString(const std::string& yaml_str);
};

extern AppState g_AppState;

inline AppState& GetAppState() { return g_AppState; }

} // namespace UI
