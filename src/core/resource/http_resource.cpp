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
    if (IsRunning())
        return;

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

    active_handle = eng.dispatchrequest(&req);
}

void HttpResource::CancelRequest()
{
    if (active_handle)
    {
        active_handle->Cancel();
    }
}

void HttpResource::Draw()
{
    if (UI::g_HttpResourceDrawer)
    {
        UI::g_HttpResourceDrawer(this);
    }
}
