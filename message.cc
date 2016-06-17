#include <utility>

#include <sys/socket.h>

#include "message.h"
using namespace std;

namespace message {

pair<int, MessageType> getHeader(int socket) {
    pair<int, MessageType> ret;

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

int getMessage(int socket, MessageInfo& info) {
    // TODO: Get message body and place into info
    // Info uses smart pointers so we don't have to
    // worry about freeing memory
    return 0;    
}

}
