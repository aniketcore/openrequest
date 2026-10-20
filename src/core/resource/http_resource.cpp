#include "http_resource.h"
#include <iostream>

namespace UI {
    HttpResourceDrawerFn g_HttpResourceDrawer = nullptr;

    void RegisterHttpResourceDrawer(HttpResourceDrawerFn fn)
    {
        g_HttpResourceDrawer = fn;
    }
}

void HttpResource::SendRequest()
{
    req.url = this->url;
    req.method = this->method;
    req.gotresponse = false;
    req.body = this->body;

    req.headers.clear();
    for (const auto& h : requestHeaders)
    {
        if (!h.first.empty())
        {
            req.headers[h.first] = h.second;
        }
    }

    eng.dispatchrequest(&req);
}

void HttpResource::Draw()
{
    if (UI::g_HttpResourceDrawer)
    {
        UI::g_HttpResourceDrawer(this);
    }
}
