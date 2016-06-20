#include <utility>
#include <iostream>

#include <sys/socket.h>

#include "message.h"
using namespace std;

namespace message {

int getServerIdentifier(int socket, MessageInfo& info) {
    int status = recv(socket, &info.server_identifier, sizeof(info.server_identifier), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv server identifier error" << endl;
    }
    return status;
}

int getPort(int socket, MessageInfo& info) {
    int status = recv(socket, &info.port, sizeof(info.port), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv port error" << endl;
    }
    return status;
}

int getName(int socket, MessageInfo& info) {
    int status = recv(socket, &info.name, sizeof(info.name), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv name error" << endl;
    }
    return status;
}

int getArgs(int socket, MessageInfo& info) {
    int status = recv(socket, &info.args, sizeof(info.args), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv args error" << endl;
    }
    return status;
}

int getArgTypes(int socket, MessageInfo& info) {
    int status = recv(socket, &info.arg_types, sizeof(info.arg_types), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv arg types error" << endl;
    }
    return status;
}

int getReasonCode(int socket, MessageInfo& info) {
    int status = recv(socket, &info.reason_code, sizeof(info.reason_code), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv code error" << endl;
    }
    return status;
}

int getHeader(int socket, MessageInfo& info) {
    // Get the length of the message
    int status = recv(socket, &info.length, sizeof(info.length), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv length error" << endl;
    }
    
    // Get the message type
    status = recv(socket, &info.type, sizeof(info.type), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv message type error" << endl;
    }
    return status;
}

int getMessage(int socket, MessageInfo& info) {
    // TODO: Get message body and place into info
    // Info uses smart pointers so we don't have to
    // worry about freeing memory

    switch (info.type) {
        case REGISTER:
            getServerIdentifier(socket, info);
            getPort(socket, info);
            getName(socket, info);
            getArgTypes(socket, info);
            break;
        case REGISTER_SUCCESS:
            getReasonCode(socket, info);
            break;
        case REGISTER_FAILURE:
            getReasonCode(socket, info);
            break;
        case LOC_REQUEST:
            getName(socket, info);
            getArgTypes(socket, info);
            break;
        case LOC_SUCCESS:
            getServerIdentifier(socket, info);
            getPort(socket, info);
            break;
        case LOC_FAILURE:
            getReasonCode(socket, info);
            break;
        case EXECUTE:
            getName(socket, info);
            getArgTypes(socket, info);
            getArgs(socket, info);
            break;
        case EXECUTE_SUCCESS:
            getName(socket, info);
            getArgTypes(socket, info);
            getArgs(socket, info);
            break;
        case EXECUTE_FAILURE:
            getReasonCode(socket, info);
            break;
        case TERMINATE:
            break;
    }

    return 0;
}

}
