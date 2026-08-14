#include <queue>
#include <httptypes.h>

// every http request will be consumed by the httpengine
// every response will be consumed by the UI by a callback
class http_queue{
    std::queue<HTTP::Request> requestqueue;
    http_queue():requestqueue(){
        
    }
    

};