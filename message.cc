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
    this->arg_types = arg_types;    
    recalculateLength();
}

void Message::setArgs(void** args) {
    this->args = args;
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

int Message::recvServerIdentifier(const int& socket) {
    int status = recv(socket, server_identifier, sizeof(server_identifier), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv server identifier error" << endl;
    }
    return status;
}

int Message::recvPort(const int& socket) {
    int status = recv(socket, &port, sizeof(port), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv port error" << endl;
    }
    return status;
}

int Message::recvName(const int& socket) {
    int status = recv(socket, name, sizeof(name), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv name error" << endl;
    }
    return status;
}

int Message::recvArgTypes(const int& socket) {
    int status = recv(socket, &num_args, sizeof(num_args), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv num args error" << endl;    
    } else {
        arg_types = new int[num_args + 1];    // +1 for null terminator
        args = new void*[num_args];
        status = recv(socket, arg_types, num_args * sizeof(*arg_types), MSG_WAITALL);
        if (status <= 0) {
            delete [] arg_types;
            delete [] args;
            
            arg_types = nullptr;
            args = nullptr;
            cerr << "recv arg types error" << endl;        
        } else {
            arg_types[num_args] = 0;    
        }
    }

    return status;
}

int Message::recvArgs(const int& socket) {

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
            
            return status;
        }

        args[i] = buffer;
    }

    return 0;
}

int Message::recvReasonCode(const int& socket) {
    int status = recv(socket, &reason_code, sizeof(reason_code), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv code error" << endl;
    }
    return status;
}

int Message::recvHeader(const int& socket) {
    // Get the length of the message
    int status = recv(socket, &length, sizeof(length), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv length error" << endl;
    }
    
    // Get the message type
    status = recv(socket, &type, sizeof(type), MSG_WAITALL);
    if (status <= 0) {
        cerr << "recv message type error" << endl;
    }
    return status;
}

int Message::recvMessage(const int& socket) {
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
        case TERMINATE:
        case NONE:
        default:
            break;
    }

    return 0;
}

int Message::peek(const int& socket) {
    int bytes;
    if (ioctl(socket, FIONREAD, &bytes) < 0) {
        return -1;
    }
    return bytes;
}

int Message::sendBytes(const int& socket, const void* buffer, const int& buffer_size) {
    int sent = 0;
    do {
        int bytes = send(socket, (char*)buffer + sent, buffer_size - sent, 0);
        if (bytes < 0) {
            return bytes;
        }
        sent += bytes;
    }
    while (sent != buffer_size);
    
    return 0;
}

int Message::sendServerIdentifier(const int& socket) {
    return sendBytes(socket, server_identifier, sizeof(server_identifier));
}

int Message::sendPort(const int& socket) {
    return sendBytes(socket, &port, sizeof(port));
}

int Message::sendName(const int& socket) {
    return sendBytes(socket, name, sizeof(name));
}

int Message::sendArgTypes(const int& socket) {
    // Send # of arguments and then send arg types without the null
    int status = sendBytes(socket, &num_args, sizeof(num_args));
    if (status < 0) {
        return status;
    }
    return sendBytes(socket, arg_types, num_args * sizeof(int));
}

int Message::sendArgs(const int& socket) {

    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        int status = sendBytes(socket, args[i], buffer_size);
        if (status < 0) {
            return status;
        }
    }

    return 0;
}

int Message::sendReasonCode(const int& socket) {
    return sendBytes(socket, &reason_code, sizeof(reason_code));
}

int Message::sendHeader(const int& socket) {
    int status = sendBytes(socket, &length, sizeof(length));
    if (status < 0) {
        return status;
    }
    return sendBytes(socket, &type, sizeof(type));
}

int Message::sendMessage(const int& socket) {

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
        case TERMINATE:
        case NONE:
        default:
            break;
    }

    return 0;
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
        case TERMINATE:
        case NONE:
        default:
            length = 0;
            break;
    }
}

}
