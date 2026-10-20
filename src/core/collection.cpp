#include "collection.h"

namespace UI
{
    std::vector<std::shared_ptr<Resource>>& g_Resources = g_AppState.resources;

    void AddResource(std::shared_ptr<Resource> res)
    {
        g_AppState.AddResource(std::move(res));
    }

    void DeleteResource(const std::shared_ptr<Resource>& res)
    {
        g_AppState.DeleteResource(res);
    }

    std::shared_ptr<Resource> FindResourceById(uint64_t id)
    {
        return g_AppState.FindResourceById(id);
    }

    int FindResourceIndexById(uint64_t id)
    {
        return g_AppState.FindResourceIndexById(id);
    }

    void SaveCollectionToFile(const std::string &path)
    {
        g_AppState.SaveCollection(path);
    }

    void LoadCollectionFromFile(const std::string &path)
    {
        g_AppState.LoadCollection(path);
    }
}
