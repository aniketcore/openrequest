#pragma once

#include <curl/curl.h>

#ifdef DELETE
#undef DELETE
#endif

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <algorithm>
#include <cctype>
#include "httptypes.h"

namespace HTTP
{
    inline void Init() {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    inline void Free() {
        curl_global_cleanup();
    }

    struct RequestHandle {
        std::atomic<bool> is_running{false};
        std::atomic<bool> cancel_requested{false};

        void Cancel() {
            cancel_requested.store(true, std::memory_order_relaxed);
        }
    };

    class Engine
    {
    public:
        Engine() = default;
        ~Engine() = default;

        // Progress meter callback called by curl multiple times per second during transfer
        static int XferInfoCallback(void* clientp,
                                    curl_off_t /*dltotal*/, curl_off_t /*dlnow*/,
                                    curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
        {
            auto* cancel_flag = static_cast<std::atomic<bool>*>(clientp);
            if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) {
                // Non-zero return value aborts transfer immediately with CURLE_ABORTED_BY_CALLBACK
                return 1;
            }
            return 0;
        }

        static size_t write_body(
            char *ptr,
            size_t size,
            size_t nmemb,
            void *userdata)
        {
            auto *response = static_cast<Response *>(userdata);
            if (!response->body.has_value())
            {
                response->body = std::string();
            }
            response->body->append(ptr, size * nmemb);
            return size * nmemb;
        }

        static size_t write_header(
            char *ptr,
            size_t size,
            size_t nmemb,
            void *userdata)
        {
            auto *response = static_cast<Response *>(userdata);
            std::string header(ptr, size * nmemb);

            while (!header.empty() && (header.back() == '\r' || header.back() == '\n'))
            {
                header.pop_back();
            }

            auto colon = header.find(':');
            if (colon == std::string::npos)
            {
                return size * nmemb;
            }

            std::string key = header.substr(0, colon);
            std::string value = header.substr(colon + 1);

            value.erase(
                value.begin(),
                std::find_if(value.begin(), value.end(),
                             [](unsigned char c)
                             {
                                 return !std::isspace(c);
                             }));

            response->headers[key] = value;
            return size * nmemb;
        }

        static void Execute(Request *req, std::shared_ptr<RequestHandle> handle)
        {
            handle->is_running.store(true, std::memory_order_relaxed);
            handle->cancel_requested.store(false, std::memory_order_relaxed);

            CURL *curl = curl_easy_init();
            Response response;

            if (curl)
            {
                curl_easy_setopt(curl, CURLOPT_URL, req->url.c_str());

                if (req->method == GET) {
                    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                } else if (req->method == POST) {
                    curl_easy_setopt(curl, CURLOPT_POST, 1L);
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body.c_str());
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req->body.size());
                } else if (req->method == PUT) {
                    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body.c_str());
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req->body.size());
                } else if (req->method == DELETE) {
                    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                }

                struct curl_slist *chunk = nullptr;
                for (const auto& [k, v] : req->headers) {
                    std::string header_str = k + ": " + v;
                    chunk = curl_slist_append(chunk, header_str.c_str());
                }
                if (chunk) {
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                }

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);
                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);

                // Modern features: automatic decompression, follow redirects, HTTP/2, connection timeout
                curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

                // Cancellation hook
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, XferInfoCallback);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &handle->cancel_requested);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

                CURLcode result = curl_easy_perform(curl);

                long status_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
                response.response_code = static_cast<int>(status_code);

                if (result == CURLE_ABORTED_BY_CALLBACK) {
                    response.body = "[Request cancelled by user]";
                    response.response_code = 0;
                } else if (result != CURLE_OK) {
                    response.body = std::string("Error: ") + curl_easy_strerror(result);
                }

                if (chunk) curl_slist_free_all(chunk);
                curl_easy_cleanup(curl);
            }

            req->SetResponse(response);
            handle->is_running.store(false, std::memory_order_relaxed);
        }

        std::shared_ptr<RequestHandle> dispatchrequest(Request *req)
        {
            auto handle = std::make_shared<RequestHandle>();
            std::thread thr(&Engine::Execute, req, handle);
            thr.detach();
            return handle;
        }
    };
}
