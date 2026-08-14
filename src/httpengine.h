#pragma once

#include <curl/curl.h>
#include <string>
#include <thread>
#include "httptypes.h"
namespace HTTP
{

    void Init(){
        curl_global_init(CURL_GLOBAL_ALL);
    }
    void Free(){
        curl_global_cleanup();
    }
    class Engine
    {
    public:
        Engine()
        {
        }
        ~Engine()
        {
        }
        void dispatchrequest(Request *req)
        {
            std::thread thr(&Engine::RunRequest, this, req);
            thr.detach();
        }
        static size_t write_body(
            char *ptr,
            size_t size,
            size_t nmemb,
            void *userdata)
        {
            auto *response = static_cast<Response *>(userdata);

            response->body.append(ptr, size * nmemb);

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

            // Remove trailing CRLF
            while (!header.empty() &&
                   (header.back() == '\r' || header.back() == '\n'))
            {
                header.pop_back();
            }

            // Find ':'
            auto colon = header.find(':');

            if (colon == std::string::npos)
            {
                return size * nmemb;
            }

            std::string key = header.substr(0, colon);
            std::string value = header.substr(colon + 1);

            // Remove leading whitespace from value
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
        Response RunRequest(Request *req)
        {

            CURL *curl;

            CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
            if (result != CURLE_OK)
                return {"sdfa"};

            curl = curl_easy_init();
            Response response;
            if (curl)
            {
                curl_easy_setopt(curl, CURLOPT_URL, req->url.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body.c_str());

                /* if we do not provide POSTFIELDSIZE, libcurl calls strlen() by itself */
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req->body.size());

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);
                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);

                /* Perform the request, result gets the return code */
                result = curl_easy_perform(curl);
                /* Check for errors */
                if (result != CURLE_OK)
                    fprintf(stderr, "curl_easy_perform() failed: %s\n",
                            curl_easy_strerror(result));

                /* always cleanup */
                curl_easy_cleanup(curl);
            }
            curl_global_cleanup();
            req->SetResponse(response);
            return response;
        }
    };
}