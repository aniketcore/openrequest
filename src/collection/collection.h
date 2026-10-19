#pragma once
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <cstdint>
#include <glaze/yaml.hpp>
#include "resource.h"
#include "http_resource.h"

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
};

template <>
struct glz::meta<Collection> {
    using T = Collection;
    static constexpr auto value = object(
        "name", &T::name,
        "version", &T::version,
        "items", &T::items
    );
};

namespace UI {
    // Collection resources (master list owned by the collection)
    extern std::vector<std::shared_ptr<Resource>> g_Resources;

    uint64_t GenerateUID();

    void SaveCollectionToFile(const std::string& path = "collection.yaml");
    void LoadCollectionFromFile(const std::string& path = "collection.yaml");

    void AddResource(std::shared_ptr<Resource> res);
    void DeleteResource(const std::shared_ptr<Resource>& res);

    std::shared_ptr<Resource> FindResourceById(uint64_t id);
    int FindResourceIndexById(uint64_t id);
}
