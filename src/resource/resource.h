#pragma once
#include <string>
#include <cstdint>

enum class ResourceType {
    HTTP,
    WebSocket
};

class Resource {
public:
    uint64_t id = 0;
    std::string name;

    Resource() = default;
    explicit Resource(uint64_t id, std::string name) : id(id), name(std::move(name)) {}
    virtual ~Resource() = default;

    virtual ResourceType GetType() const = 0;
    virtual void Draw() = 0;
};
