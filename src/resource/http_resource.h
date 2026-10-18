#pragma once
#include "resource.h"
#include <string>
#include <vector>
#include <memory>
#include "httptypes.h"
#include "httpengine.h"
#include "TextEditor.h"
#include <glaze/yaml.hpp>

class HttpResource : public Resource {
public:
    std::string url;
    HTTP::Method method;
    float requestHeight = -1.0f;
    std::string body;
    
    HTTP::Engine eng;
    HTTP::Request req;
    std::unique_ptr<TextEditor> requestEditor;
    std::unique_ptr<TextEditor> responseEditor;
    std::string lastResponseText;
    bool requestEditorInitialized = false;

    std::vector<std::pair<std::string, std::string>> requestHeaders;

    HttpResource() : method(HTTP::GET) {}
    explicit HttpResource(int id, std::string name) : Resource(id, std::move(name)), method(HTTP::GET) {}
    HttpResource(HttpResource&&) noexcept = default;
    HttpResource& operator=(HttpResource&&) noexcept = default;

    ResourceType GetType() const override { return ResourceType::HTTP; }
    void Draw() override;
};

template <>
struct glz::meta<HTTP::Method> {
    using enum HTTP::Method;
    static constexpr auto value = enumerate(
        "GET", GET,
        "POST", POST,
        "PUT", PUT,
        "DELETE", DELETE,
        "ERR", ERR
    );
};

template <>
struct glz::meta<HttpResource>
{
    using T = HttpResource;
    static constexpr auto value = object(
        "id", &T::id,
        "name", &T::name,
        "url", &T::url,
        "method", &T::method,
        "requestHeaders", &T::requestHeaders,
        "body", &T::body
    );
};
