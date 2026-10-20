#include "app_state.h"
#include <random>
#include <chrono>

namespace UI {

uint64_t GenerateUID()
{
    thread_local std::mt19937_64 rng([]() {
        std::random_device rd;
        uint64_t seed = (static_cast<uint64_t>(rd()) << 32) ^ rd();
        auto now = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        return seed ^ now;
    }());

    // 15-digit random integer (within IEEE-754 double precision limit, collision-free)
    std::uniform_int_distribution<uint64_t> dist(100000000000000ULL, 999999999999999ULL);
    return dist(rng);
}

OPENREQUEST_CORE_API AppState g_AppState;

AppState& GetAppState()
{
    return g_AppState;
}

AppState::AppState()
{
    InitializeDefault();
}

void AppState::InitializeDefault()
{
    if (resources.empty())
    {
        auto res = std::make_shared<HttpResource>();
        res->id = GenerateUID();
        res->name = "Tab 1";
        resources.push_back(res);

        tabs.clear();
        tabs.push_back(std::make_unique<Tab>(1, "Tab 1", res));
        active_tab_id = 1;
        next_tab_id = 2;
    }
}

void AppState::AddResource(std::shared_ptr<Resource> res)
{
    if (res)
    {
        if (res->id == 0)
        {
            res->id = GenerateUID();
        }
        resources.push_back(std::move(res));
    }
}

void AppState::DeleteResource(const std::shared_ptr<Resource>& res)
{
    if (!res) return;

    // Close any tab currently viewing this resource
    for (auto it = tabs.begin(); it != tabs.end();)
    {
        if ((*it)->resource == res)
        {
            int tab_id = (*it)->id;
            it = tabs.erase(it);
            if (active_tab_id == tab_id)
            {
                active_tab_id = tabs.empty() ? -1 : tabs.back()->id;
                if (active_tab_id != -1)
                {
                    FocusTab(active_tab_id);
                }
            }
        }
        else
        {
            ++it;
        }
    }

    // Remove from collection master list
    auto it = std::find(resources.begin(), resources.end(), res);
    if (it != resources.end())
    {
        resources.erase(it);
    }
}

std::shared_ptr<Resource> AppState::FindResourceById(uint64_t id) const
{
    for (const auto& res : resources)
    {
        if (res && res->id == id)
        {
            return res;
        }
    }
    return nullptr;
}

int AppState::FindResourceIndexById(uint64_t id) const
{
    for (size_t i = 0; i < resources.size(); ++i)
    {
        if (resources[i] && resources[i]->id == id)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Tab* AppState::GetActiveTab() const
{
    for (const auto& tab : tabs)
    {
        if (tab && tab->id == active_tab_id)
        {
            return tab.get();
        }
    }
    return nullptr;
}

Tab* AppState::FindTabById(int tab_id) const
{
    for (const auto& tab : tabs)
    {
        if (tab && tab->id == tab_id)
        {
            return tab.get();
        }
    }
    return nullptr;
}

Tab* AppState::FindTabByResource(const std::shared_ptr<Resource>& res) const
{
    if (!res) return nullptr;
    for (const auto& tab : tabs)
    {
        if (tab && tab->resource == res)
        {
            return tab.get();
        }
    }
    return nullptr;
}

void AppState::FocusTab(int tab_id)
{
    active_tab_id = tab_id;
    tab_to_focus = tab_id;
}

void AppState::CloseTab(int tab_id)
{
    for (auto it = tabs.begin(); it != tabs.end(); ++it)
    {
        if ((*it)->id == tab_id)
        {
            tabs.erase(it);
            if (active_tab_id == tab_id)
            {
                active_tab_id = tabs.empty() ? -1 : tabs.back()->id;
                if (active_tab_id != -1)
                {
                    FocusTab(active_tab_id);
                }
            }
            break;
        }
    }
}

void AppState::OpenOrFocusResource(const std::shared_ptr<Resource>& res)
{
    if (!res) return;

    for (const auto& tab : tabs)
    {
        if (tab && tab->resource == res)
        {
            FocusTab(tab->id);
            return;
        }
    }

    int new_id = next_tab_id++;
    tabs.push_back(std::make_unique<Tab>(new_id, res->name, res));
    FocusTab(new_id);
}

Collection AppState::ToCollection() const
{
    Collection col;
    col.name = collection_name;
    col.version = collection_version;

    // Collect resources
    for (const auto& res : resources)
    {
        if (!res) continue;

        if (res->GetType() == ResourceType::HTTP)
        {
            auto* http = static_cast<HttpResource*>(res.get());

            HttpResource item_copy;
            item_copy.id = http->id;
            item_copy.name = http->name;
            item_copy.url = http->url;
            item_copy.method = http->method;
            item_copy.requestHeaders = http->requestHeaders;
            item_copy.body = http->body;
            item_copy.requestHeight = http->requestHeight;
            col.items.emplace_back(std::move(item_copy));
        }
    }

    // Save open tabs
    for (const auto& tab : tabs)
    {
        if (tab && tab->resource)
        {
            col.open_tabs.push_back(tab->resource->id);
            if (tab->id == active_tab_id)
            {
                col.active_tab = tab->resource->id;
            }
        }
    }

    return col;
}

void AppState::FromCollection(Collection col)
{
    collection_name = std::move(col.name);
    collection_version = std::move(col.version);

    for (auto& item : col.items)
    {
        std::visit([this](auto&& res)
        {
            using T = std::decay_t<decltype(res)>;
            if constexpr (std::is_same_v<T, HttpResource>)
            {
                if (res.id == 0)
                {
                    res.id = GenerateUID();
                }

                if (res.name.empty())
                {
                    res.name = "HTTP Request";
                }

                auto existing = FindResourceById(res.id);
                if (existing)
                {
                    auto* http = dynamic_cast<HttpResource*>(existing.get());
                    if (http)
                    {
                        http->name = res.name;
                        http->url = res.url;
                        http->method = res.method;
                        http->requestHeaders = res.requestHeaders;
                        http->body = res.body;
                        http->requestEditorInitialized = false;
                    }
                }
                else
                {
                    auto res_ptr = std::make_shared<HttpResource>(std::move(res));
                    resources.push_back(res_ptr);
                }
            }
        }, item);
    }

    // Restore open tabs if specified
    if (!col.open_tabs.empty())
    {
        for (uint64_t res_id : col.open_tabs)
        {
            auto res = FindResourceById(res_id);
            if (res)
            {
                OpenOrFocusResource(res);
            }
        }
        if (col.active_tab != 0)
        {
            auto active_res = FindResourceById(col.active_tab);
            if (active_res)
            {
                OpenOrFocusResource(active_res);
            }
        }
    }
    else if (!resources.empty() && tabs.empty())
    {
        OpenOrFocusResource(resources.front());
    }
}

bool AppState::SaveCollection(const std::string& path)
{
    std::string target_path = path.empty() ? collection_file_path : path;
    if (target_path.empty())
    {
        target_path = "collection.yaml";
    }
    collection_file_path = target_path;

    Collection col = ToCollection();
    std::string buffer;
    auto ec = glz::write_file_yaml(col, target_path, buffer);
    if (ec)
    {
        std::cerr << "Failed to save collection to " << target_path << ": "
                  << glz::format_error(ec, buffer) << std::endl;
        return false;
    }
    return true;
}

bool AppState::LoadCollection(const std::string& path)
{
    std::string target_path = path.empty() ? collection_file_path : path;
    if (target_path.empty())
    {
        target_path = "collection.yaml";
    }
    collection_file_path = target_path;

    Collection col;
    std::string buffer;
    auto ec = glz::read_file_yaml(col, target_path, buffer);
    if (ec)
    {
        std::cerr << "Failed to load collection from " << target_path << ": "
                  << glz::format_error(ec, buffer) << std::endl;
        return false;
    }

    FromCollection(std::move(col));
    return true;
}

std::string AppState::ToYamlString() const
{
    Collection col = ToCollection();
    std::string buffer;
    auto ec = glz::write_yaml(col, buffer);
    if (ec)
    {
        std::cerr << "Failed to serialize collection: " << glz::format_error(ec, buffer) << std::endl;
        return "";
    }
    return buffer;
}

bool AppState::FromYamlString(const std::string& yaml_str)
{
    Collection col;
    auto ec = glz::read_yaml(col, yaml_str);
    if (ec)
    {
        std::cerr << "Failed to deserialize collection: " << glz::format_error(ec, yaml_str) << std::endl;
        return false;
    }
    FromCollection(std::move(col));
    return true;
}

} // namespace UI
