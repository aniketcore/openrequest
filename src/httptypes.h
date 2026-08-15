#pragma once

#include <string>
#include <unordered_map>
#include <optional>
namespace HTTP{

    enum Method{
        GET, POST, PUT, DELETE, ERR
    };
    struct Response
    {
        std::unordered_map <std::string, std::string> headers;
        std::optional<std::string> body;
        int response_code;
        Response(int response_code, std::unordered_map<std::string, std::string> headers,std::string body)
        :response_code(response_code),headers(headers),body(body)
        {
            
        }
        Response(int response_code, std::unordered_map<std::string, std::string> headers)
        :response_code(response_code),headers(headers)
        {
            
        }
        Response(){

        }
        std::string get_body(){
            if(body.has_value()){
                return body.value();
            }
            return "empty";
        }
    };
    struct Request
    {
        std::string url;
        std::unordered_map<std::string,std::string> headers;
        std::string body;
        Response res;
        bool gotheaders = 0;
        bool gotbody = 0;
        bool gotresponse =0;
        void SetResponse(Response r){
            gotresponse = 1;
            res = r;
        }
        Response& getresponse() {
            return res;
        }
    };

    
}