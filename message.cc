#include <cstring>
#include <iostream>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include "args.h"
#include "message.h"
using namespace args;
using namespace std;

namespace message {

Message::Message(): length(0), type(MessageType::NONE), name{0},
    server_identifier{0}, port(0), reason_code(0), num_args(0),
    arg_types(nullptr), args(nullptr), raw_index(0), total_bytes(0),
    flags(0), HEADER_SIZE(sizeof(length) + sizeof(type)) {
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
    const int len = min(sizeof(this->name), strlen(name) + 1);
    memcpy(this->name, name, len);
}

void Message::setServerIdentifier(const char* identifier) {
    const int len = min(sizeof(this->server_identifier), strlen(identifier) + 1);
    memcpy(this->server_identifier, identifier, len);
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

void Message::parse(void* dst, const int& buffer_size) {
    const char* buffer = raw_bytes.get() + raw_index;
    memcpy(dst, buffer, buffer_size);
    raw_index += buffer_size;
}

void Message::recvServerIdentifier() {
    parse(server_identifier, sizeof(server_identifier));
}

void Message::recvPort() {
    parse(&port, sizeof(port));
}

void Message::recvName() {
    parse(name, sizeof(name));
}

void Message::recvArgTypes() {

    // Clean up previous args
    cleanup();

    parse(&num_args, sizeof(num_args));
    arg_types = new int[num_args + 1];    // +1 for null terminator

    for (int i = 0; i < num_args; ++i) {
        parse(arg_types + i, sizeof(*arg_types));
    }
    arg_types[num_args] = 0;
}

void Message::recvArgs() {

    args = new void*[num_args];
    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        void* buffer = new char[buffer_size];
        parse(buffer, buffer_size);
        args[i] = buffer;
    }
}

void Message::recvReasonCode() {
    parse(&reason_code, sizeof(reason_code));
}

void Message::recvHeader() {
    raw_index = 0;
    parse(&length, sizeof(length));
    parse(&type, sizeof(type));
}

void Message::recvBytes(const int& socket, const int& max_bytes) {

    const int buffer_size = max_bytes - total_bytes;
    char* buffer = raw_bytes.get() + total_bytes;

    int num_bytes = recv(socket, buffer, 1/*buffer_size*/, 0);
    if (num_bytes <= 0) {
        throw RecvError();
    }
    
    total_bytes += num_bytes;
}

void Message::recvNonBlock(const int& socket) {
    
    if ((flags & END_OF_HEADER) == 0) {
        if (raw_bytes.get() == nullptr) {
            raw_bytes.reset(new char[HEADER_SIZE]);
        }

        recvBytes(socket, HEADER_SIZE);
        if (total_bytes < HEADER_SIZE) {
            return;
        }
       
        recvHeader();
        total_bytes = 0;
        flags |= END_OF_HEADER;

        if (length == 0) {
            raw_bytes.reset(nullptr);
            flags |= END_OF_MESSAGE;
        } else {
            raw_bytes.reset(new char[length]);
        }
    }

    if ((flags & END_OF_MESSAGE) == 0) {
        recvBytes(socket, length); 
        if (total_bytes < length) {
            return;
        }

        recvMessage();
        raw_bytes.reset(nullptr);
        total_bytes = 0;
        flags |= END_OF_MESSAGE; 
    }
}

void Message::recvBlock(const int& socket) {
    while (!eom()) {
        recvNonBlock(socket);
    }
}

bool Message::eom() const {
    return flags & END_OF_MESSAGE;
}

void Message::recvMessage() {
    raw_index = 0;
    switch (type) {
        case REGISTER:
            recvServerIdentifier();
            recvPort();
            recvName();
            recvArgTypes();
            break;
        case REGISTER_SUCCESS:
            recvReasonCode();
            break;
        case REGISTER_FAILURE:
            recvReasonCode();
            break;
        case LOC_REQUEST:
            recvName();
            recvArgTypes();
            break;
        case LOC_SUCCESS:
            recvServerIdentifier();
            recvPort();
            break;
        case LOC_FAILURE:
            recvReasonCode();
            break;
        case EXECUTE:
            recvName();
            recvArgTypes();
            recvArgs();
            break;
        case EXECUTE_SUCCESS:
            recvName();
            recvArgTypes();
            recvArgs();
            break;
        case EXECUTE_FAILURE:
            recvReasonCode();
            break;
        case LOC_CACHE:
            recvName();
            recvArgTypes();
            break;
        case LOC_CACHE_SUCCESS:
            recvArgTypes();
            recvArgs();
            break;
        case TERMINATE:
        case NONE:
        default:
            break;
    }
}

void Message::sendBytes(const int& socket, const void* buffer, const int& buffer_size) {
    int sent = 0;
    do {
        int bytes = send(socket, (char*)buffer + sent, 1 /*buffer_size - sent*/, 0);
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
