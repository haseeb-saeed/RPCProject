#include <utility>

#include <sys/socket.h>

#include "request.h"
using namespace std;

namespace request {

pair<int, Request> getHeader(int socket) {
    pair<int, Request> ret;

    // Get the length of the message
    int status = recv(socket, &ret.first, sizeof(ret.first), MSG_WAITALL);
    if (status <= 0) {
        ret.first = status;
        return ret;    
    }
    
    // Get the message type
    status = recv(socket, &ret.second, sizeof(ret.second), MSG_WAITALL);
    if (status <= 0) {
        ret.first = status;    
    }

    return ret;
}

}
