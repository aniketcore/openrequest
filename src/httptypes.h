#pragma once

#include <string>
#include <unordered_map>
namespace HTTP{

    enum Method{
        GET, POST, PUT, DELETE, ERR
    };
    inline Method StringToMethod(const std::string & str){
        // Method method = ERR;
        if (str == "GET")
        {
            return GET;
        }
        else if(str == "POST"){
             return POST;
        }
        else if(str == "PUT"){
            return DELETE;
        }
        return ERR;
        
    }
    struct Instance{
        public:
        std::string url;
        Method method;
    
    };
    struct Response
    {
        std::string body;
        std::unordered_map <std::string, std::string> headers;
    };
    struct Request
    {
        std::string url;
        std::unordered_map<std::string,std::string> headers;
        std::string body;
        Response res={"sdfs"};
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