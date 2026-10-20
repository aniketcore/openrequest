#pragma once
#include "app_state.h"

namespace UI {
    // Collection resources (reference to g_AppState.resources for backward compatibility)
    extern std::vector<std::shared_ptr<Resource>>& g_Resources;

    void SaveCollectionToFile(const std::string& path = "collection.yaml");
    void LoadCollectionFromFile(const std::string& path = "collection.yaml");

    void AddResource(std::shared_ptr<Resource> res);
    void DeleteResource(const std::shared_ptr<Resource>& res);

    std::shared_ptr<Resource> FindResourceById(uint64_t id);
    int FindResourceIndexById(uint64_t id);
}
