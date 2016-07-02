#include <cstring>
#include <iostream>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include "args.h"
#include "message.h"
using namespace args;
using namespace std;

namespace message {

Message::Message(): length(0), type(MessageType::NONE), port(0),
    reason_code(0), num_args(0), arg_types(nullptr), args(nullptr),
    HEADER_SIZE(sizeof(length) + sizeof(type)) {
}

Message::~Message() {
    // Free any allocated memory
    cleanup();
}

void Message::setType(const MessageType& type) {
    this->type = type;
    recalculateLength();
}

void Message::setName(const char* name) {
    memcpy(this->name, name, sizeof(this->name));
}

void Message::setServerIdentifier(const char* identifier) {
    memcpy(this->server_identifier, identifier, sizeof(this->server_identifier));
}

void Message::setPort(const int& port) {
    this->port = port;    
}

void Message::setReasonCode(const int& reason_code) {
    this->reason_code = reason_code;    
}

void Message::setArgTypes(int* arg_types) {

    this->num_args = args::numArgs(arg_types);
    this->arg_types = new int[this->num_args + 1];
    copyArgTypes(this->arg_types, arg_types);

    recalculateLength();
}

void Message::setArgs(void** args) {

    this->args = new void*[num_args];
    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        this->args[i] = new char[buffer_size];
    }

    copyArgs(this->args, args, arg_types);
}

MessageType Message::getType() const {
    return type;    
}

const char* Message::getName() const {
    return name;
}

const char* Message::getServerIdentifier() const {
    return server_identifier;
}
                                            
int Message::getPort() const {
    return port;    
}

int Message::getReasonCode() const {
    return reason_code;
}

int* Message::getArgTypes() const {
    return arg_types;    
}

void** Message::getArgs() const {
    return args;    
}

int Message::getLength() const {
    return length;    
}

int Message::numArgs() const {
    return num_args;    
}

void Message::recvServerIdentifier(const int& socket) {
    if (recv(socket, server_identifier, sizeof(server_identifier), MSG_WAITALL) <= 0) {
        cerr << "recv server identifier error" << endl;
        throw RecvError();
    }
}

void Message::recvPort(const int& socket) {
    if (recv(socket, &port, sizeof(port), MSG_WAITALL) <= 0) {
        cerr << "recv port error" << endl;
        throw RecvError();
    }
}

void Message::recvName(const int& socket) {
    if (recv(socket, name, sizeof(name), MSG_WAITALL) <= 0) {
        cerr << "recv name error" << endl;
        throw RecvError();
    }
}

void Message::recvArgTypes(const int& socket) {
    if (recv(socket, &num_args, sizeof(num_args), MSG_WAITALL) <= 0) {
        cerr << "recv num args error" << endl;
        throw RecvError();
    } else {
        // Free any old memory before we proceed
        cleanup();

        arg_types = new int[num_args + 1];    // +1 for null terminator
        if (recv(socket, arg_types, num_args * sizeof(*arg_types), MSG_WAITALL) <= 0) {
            delete [] arg_types;
            arg_types = nullptr;

            cerr << "recv arg types error" << endl;        
            throw RecvError();
        } else {
            arg_types[num_args] = 0;    
        }
    }
}

void Message::recvArgs(const int& socket) {

    args = new void*[num_args];
    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        void* buffer = (void*) new char[buffer_size];
        int status = recv(socket, buffer, buffer_size, MSG_WAITALL);
        if (status <= 0) {
            for (int j = 0; j <= i; ++j) {
                delete [] (char*)args[j];
            }

            delete [] arg_types;
            delete [] args;

            arg_types = nullptr;
            args = nullptr;

            cerr << "recv arg error" << endl;  
            throw RecvError();
        }

        args[i] = buffer;
    }
}

void Message::recvReasonCode(const int& socket) {
    if (recv(socket, &reason_code, sizeof(reason_code), MSG_WAITALL) <= 0) {
        cerr << "recv code error" << endl;
        throw RecvError();
    }
}

void Message::recvHeader(const int& socket) {
    // Get the length of the message
    if (recv(socket, &length, sizeof(length), MSG_WAITALL) <= 0) {
        cerr << "recv length error" << endl;
        throw RecvError();
    }
    
    // Get the message type
    if (recv(socket, &type, sizeof(type), MSG_WAITALL) <= 0) {
        cerr << "recv message type error" << endl;
        throw RecvError();
    }
}

void Message::recvMessage(const int& socket) {
    switch (type) {
        case REGISTER:
            this->recvServerIdentifier(socket);
            this->recvPort(socket);
            this->recvName(socket);
            this->recvArgTypes(socket);
            break;
        case REGISTER_SUCCESS:
            this->recvReasonCode(socket);
            break;
        case REGISTER_FAILURE:
            this->recvReasonCode(socket);
            break;
        case LOC_REQUEST:
            this->recvName(socket);
            this->recvArgTypes(socket);
            break;
        case LOC_SUCCESS:
            this->recvServerIdentifier(socket);
            this->recvPort(socket);
            break;
        case LOC_FAILURE:
            this->recvReasonCode(socket);
            break;
        case EXECUTE:
            this->recvName(socket);
            this->recvArgTypes(socket);
            this->recvArgs(socket);
            break;
        case EXECUTE_SUCCESS:
            this->recvName(socket);
            this->recvArgTypes(socket);
            this->recvArgs(socket);
            break;
        case EXECUTE_FAILURE:
            this->recvReasonCode(socket);
            break;
        case LOC_CACHE:
            this->recvName(socket);
            this->recvArgTypes(socket);
            break;
        case LOC_CACHE_SUCCESS:
            this->recvArgTypes(socket);
            this->recvArgs(socket);
            break;
        case TERMINATE:
        case NONE:
        default:
            break;
    }
}

int Message::peek(const int& socket) {
    int bytes;
    if ((ioctl(socket, FIONREAD, &bytes) < 0) || bytes <= 0) {
        throw PeekError();
    }
    return bytes;
}

void Message::sendBytes(const int& socket, const void* buffer, const int& buffer_size) {
    int sent = 0;
    do {
        int bytes = send(socket, (char*)buffer + sent, buffer_size - sent, 0);
        if (bytes < 0) {
            throw SendError();
        }
        sent += bytes;
    }
    while (sent != buffer_size);
}

void Message::sendServerIdentifier(const int& socket) {
    sendBytes(socket, server_identifier, sizeof(server_identifier));
}

void Message::sendPort(const int& socket) {
    sendBytes(socket, &port, sizeof(port));
}

void Message::sendName(const int& socket) {
    sendBytes(socket, name, sizeof(name));
}

void Message::sendArgTypes(const int& socket) {
    // Send # of arguments and then send arg types without the null
    sendBytes(socket, &num_args, sizeof(num_args));
    sendBytes(socket, arg_types, num_args * sizeof(int));
}

void Message::sendArgs(const int& socket) {

    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        sendBytes(socket, args[i], buffer_size);
    }
}

void Message::sendReasonCode(const int& socket) {
    sendBytes(socket, &reason_code, sizeof(reason_code));
}

void Message::sendHeader(const int& socket) {
    sendBytes(socket, &length, sizeof(length));
    sendBytes(socket, &type, sizeof(type));
}

void Message::sendMessage(const int& socket) {

    this->sendHeader(socket);
    switch (type) {
        case REGISTER:
            this->sendServerIdentifier(socket);
            this->sendPort(socket);
            this->sendName(socket);
            this->sendArgTypes(socket);
            break;
        case REGISTER_SUCCESS:
            this->sendReasonCode(socket);
            break;
        case REGISTER_FAILURE:
            this->sendReasonCode(socket);
            break;
        case LOC_REQUEST:
            this->sendName(socket);
            this->sendArgTypes(socket);
            break;
        case LOC_SUCCESS:
            this->sendServerIdentifier(socket);
            this->sendPort(socket);
            break;
        case LOC_FAILURE:
            this->sendReasonCode(socket);
            break;
        case EXECUTE:
            this->sendName(socket);
            this->sendArgTypes(socket);
            this->sendArgs(socket);
            break;
        case EXECUTE_SUCCESS:
            this->sendName(socket);
            this->sendArgTypes(socket);
            this->sendArgs(socket);
            break;
        case EXECUTE_FAILURE:
            this->sendReasonCode(socket);
            break;
        case LOC_CACHE:
            this->sendName(socket);
            this->sendArgTypes(socket);
            break;
        case LOC_CACHE_SUCCESS:
            this->sendArgTypes(socket);
            this->sendArgs(socket);
            break;
        case TERMINATE:
        case NONE:
        default:
            break;
    }
}

void Message::recalculateLength() {
    switch (type) {
        case REGISTER:
            length = sizeof(server_identifier) + sizeof(port) + sizeof(name)
                + sizeof(num_args) + sizeof(*arg_types) * num_args;
            break;
        case REGISTER_SUCCESS:
        case REGISTER_FAILURE:
        case LOC_FAILURE:
        case EXECUTE_FAILURE:
            length = sizeof(reason_code);
            break;
        case LOC_REQUEST:
        case LOC_CACHE:
            length = sizeof(name) + sizeof(num_args)
                + sizeof(*arg_types) * num_args;
            break;
        case LOC_SUCCESS:
            length = sizeof(server_identifier) + sizeof(port);
            break;
        case EXECUTE:
        case EXECUTE_SUCCESS:
            length = sizeof(name) + sizeof(num_args)
                + sizeof(*arg_types) * num_args;

            // Add total arg size to length
            for (int i = 0; i < num_args; ++i) {
                length += argSize(arg_types[i]);
            }

            break;
        case LOC_CACHE_SUCCESS:
            length = sizeof(num_args) + sizeof(*arg_types) * num_args;

            // Add total arg size to length
            for (int i = 0; i < num_args; ++i) {
                length += argSize(arg_types[i]);
            }
    
            break;
        case TERMINATE:
        case NONE:
        default:
            length = 0;
            break;
    }
}

void Message::cleanup() {

    if (args != nullptr) {
        for (int i = 0; i < num_args; ++i) {
            delete [] (char*)(args[i]);
        }
    }

    delete [] args;
    delete [] arg_types;

    args = nullptr;
    arg_types = nullptr;
}

}
