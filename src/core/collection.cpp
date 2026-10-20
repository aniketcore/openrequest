#include "collection.h"
#include "tab.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>

namespace UI
{
    std::vector<std::shared_ptr<Resource>> g_Resources;

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

    void AddResource(std::shared_ptr<Resource> res)
    {
        if (res)
        {
            if (res->id == 0)
            {
                res->id = GenerateUID();
            }
            g_Resources.push_back(std::move(res));
        }
    }

    void DeleteResource(const std::shared_ptr<Resource>& res)
    {
        if (!res) return;

        // Close any tab currently viewing this resource
        for (auto it = g_Tabs.begin(); it != g_Tabs.end();)
        {
            if ((*it)->resource == res)
            {
                int tab_id = (*it)->id;
                it = g_Tabs.erase(it);
                if (g_ActiveTabId == tab_id)
                {
                    g_ActiveTabId = g_Tabs.empty() ? -1 : g_Tabs.back()->id;
                    if (g_ActiveTabId != -1)
                    {
                        FocusTab(g_ActiveTabId);
                    }
                }
            }
            else
            {
                ++it;
            }
        }

        // Remove from collection master list
        auto it = std::find(g_Resources.begin(), g_Resources.end(), res);
        if (it != g_Resources.end())
        {
            g_Resources.erase(it);
        }
    }

    std::shared_ptr<Resource> FindResourceById(uint64_t id)
    {
        for (const auto& res : g_Resources)
        {
            if (res && res->id == id)
            {
                return res;
            }
        }
        return nullptr;
    }

    int FindResourceIndexById(uint64_t id)
    {
        for (size_t i = 0; i < g_Resources.size(); ++i)
        {
            if (g_Resources[i] && g_Resources[i]->id == id)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void SaveCollectionToFile(const std::string &path)
    {
        Collection col;
        col.name = "OpenRequest Collection";
        col.version = "1.0";

        for (const auto &res : g_Resources)
        {
            if (!res)
                continue;

            if (res->GetType() == ResourceType::HTTP)
            {
                auto *http = static_cast<HttpResource *>(res.get());
                if (http->requestEditor)
                {
                    http->body = http->requestEditor->GetText();
                }

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

        std::string buffer;
        auto ec = glz::write_file_yaml(col, path, buffer);
        if (ec)
        {
            std::cerr << "Failed to save collection to " << path << ": "
                      << glz::format_error(ec, buffer) << std::endl;
        }
    }

    void LoadCollectionFromFile(const std::string &path)
    {
        Collection col;
        std::string buffer;
        auto ec = glz::read_file_yaml(col, path, buffer);
        if (ec)
        {
            std::cerr << "Failed to load collection from " << path << ": "
                      << glz::format_error(ec, buffer) << std::endl;
            return;
        }

        for (auto &item : col.items)
        {
            std::visit([&](auto &&res)
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
                            if (http->requestEditor)
                            {
                                http->requestEditor->SetText(res.body);
                            }
                        }
                    }
                    else
                    {
                        auto res_ptr = std::make_shared<HttpResource>(std::move(res));
                        g_Resources.push_back(res_ptr);
                    }
                }
            }, item);
        }

        // Open a tab for the first resource if available
        if (!g_Resources.empty())
        {
            OpenOrFocusResource(g_Resources.front());
        }
    }

} // namespace UI
