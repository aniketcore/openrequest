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
    explicit HttpResource(uint64_t id, std::string name) : Resource(id, std::move(name)), method(HTTP::GET) {}
    HttpResource(HttpResource&&) noexcept = default;
    HttpResource& operator=(HttpResource&&) noexcept = default;

    ResourceType GetType() const override { return ResourceType::HTTP; }
    void Draw() override;
    void SendRequest();
};

namespace UI {
    using HttpResourceDrawerFn = void (*)(HttpResource*);
    extern HttpResourceDrawerFn g_HttpResourceDrawer;
    void RegisterHttpResourceDrawer(HttpResourceDrawerFn fn);
    void DrawHttpResource(HttpResource* http);
}

#ifdef DELETE
#undef DELETE
#endif

template <>
struct glz::meta<HTTP::Method> {
    static constexpr auto value = enumerate(
        "GET", HTTP::GET,
        "POST", HTTP::POST,
        "PUT", HTTP::PUT,
        "DELETE", HTTP::DELETE,
        "ERR", HTTP::ERR
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
