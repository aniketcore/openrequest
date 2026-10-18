#pragma once
#include <string>

enum class ResourceType {
    HTTP,
    WebSocket
};

class Resource {
public:
    int id = 0;
    std::string name;

    Resource() = default;
    explicit Resource(int id, std::string name) : id(id), name(std::move(name)) {}
    virtual ~Resource() = default;

    virtual ResourceType GetType() const = 0;
    virtual void Draw() = 0;
};
