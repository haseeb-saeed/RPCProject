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

int getArgTypes(int socket, MessageInfo& info) {
    int status = recv(socket, &info.num_args, sizeof(info.num_args), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv num args error" << endl;    
    } else {
        info.arg_types = new int[info.num_args + 1];    // +1 for null terminator
        info.args = new void*[info.num_args];
        status = recv(socket, info.arg_types, info.num_args * sizeof(int), MSG_WAITALL);
        if (status <= 0) {
            info.length = status;
            delete [] info.arg_types;
            delete [] info.args;
            cerr << "recv arg types error" << endl;        
        } else {
            info.arg_types[info.num_args] = 0;    
        }
    }

    return status;
}

// TODO: This needs to be rewritten so that it correctly allocates memory
int getArgs(int socket, MessageInfo& info) {

    // Okay, here's the deal. I need to iterate through each argtype
    // and get the type and the number of args to read.
    // then I need to allocate memory for each arg and store the pointer
    // FUCK THIS IS GROSS
    return 0;
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
