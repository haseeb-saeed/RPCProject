#include <cstring>
#include <iostream>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include "args.h"
#include "message.h"
using namespace args;
using namespace std;

namespace message {

// Constructor
Message::Message(): length(0), type(MessageType::NONE), name{0},
    server_identifier{0}, port(0), reason_code(0), num_args(0),
    arg_types(nullptr), args(nullptr), raw_index(0), total_bytes(0),
    flags(0), HEADER_SIZE(sizeof(length) + sizeof(type)) {
}

// Destructor - free any allocated memory
Message::~Message() {
    cleanup();
}

// Set message type
void Message::setType(const MessageType& type) {
    this->type = type;
    recalculateLength();
}

// Set the function name
void Message::setName(const char* name) {
    const int len = min(sizeof(this->name), strlen(name) + 1);
    memcpy(this->name, name, len);
}

// Set the server identifier
void Message::setServerIdentifier(const char* identifier) {
    const int len = min(sizeof(this->server_identifier), strlen(identifier) + 1);
    memcpy(this->server_identifier, identifier, len);
}

// Set the server port
void Message::setPort(const int& port) {
    this->port = port;    
}

// Set the reason code
void Message::setReasonCode(const int& reason_code) {
    this->reason_code = reason_code;    
}

// Set the arg types
void Message::setArgTypes(int* arg_types) {
    this->num_args = args::numArgs(arg_types);
    this->arg_types = new int[this->num_args + 1];
    copyArgTypes(this->arg_types, arg_types);

    recalculateLength();
}

// Set the args themselves
void Message::setArgs(void** args) {
    this->args = new void*[num_args];
    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        this->args[i] = new char[buffer_size];
    }

    copyArgs(this->args, args, arg_types);
}

// Get the message type
MessageType Message::getType() const {
    return type;    
}

// Get the function name
const char* Message::getName() const {
    return name;
}

// Get the server identifier
const char* Message::getServerIdentifier() const {
    return server_identifier;
}

// Get the server port                                            
int Message::getPort() const {
    return port;    
}

// Get the reason code
int Message::getReasonCode() const {
    return reason_code;
}

// Get the argument types
int* Message::getArgTypes() const {
    return arg_types;    
}

// Get the arguments
void** Message::getArgs() const {
    return args;    
}

// Get the length of the message (in bytes)
int Message::getLength() const {
    return length;    
}

// Get the number of arguments
int Message::numArgs() const {
    return num_args;    
}

// Remove the number of bytes out of the raw data buffer
// and increment the raw index to the next part of the buffer
void Message::parse(void* dst, const int& buffer_size) {
    const char* buffer = raw_bytes.get() + raw_index;
    memcpy(dst, buffer, buffer_size);
    raw_index += buffer_size;
}

// Read the server identifier
void Message::recvServerIdentifier() {
    parse(server_identifier, sizeof(server_identifier));
}

// Read the server port
void Message::recvPort() {
    parse(&port, sizeof(port));
}

// Read the function name
void Message::recvName() {
    parse(name, sizeof(name));
}

// Read the arg types
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

// Read the args
void Message::recvArgs() {
    args = new void*[num_args];
    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        void* buffer = new char[buffer_size];
        parse(buffer, buffer_size);
        args[i] = buffer;
    }
}

// Read the reason code
void Message::recvReasonCode() {
    parse(&reason_code, sizeof(reason_code));
}

// Read the header
void Message::recvHeader() {
    raw_index = 0;
    parse(&length, sizeof(length));
    parse(&type, sizeof(type));
}

// Read at most max_bytes from the given socket
void Message::recvBytes(const int& socket, const int& max_bytes) {
    const int buffer_size = max_bytes - total_bytes;
    char* buffer = raw_bytes.get() + total_bytes;

    int num_bytes = recv(socket, buffer, buffer_size, 0);
    if (num_bytes <= 0) {
        throw RecvError();
    }
    
    total_bytes += num_bytes;
}

// Non-blocking receive (may need to be called multiple times)
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

// Block version of receive
void Message::recvBlock(const int& socket) {
    while (!eom()) {
        recvNonBlock(socket);
    }
}

// End of message/all bytes received
bool Message::eom() const {
    return flags & END_OF_MESSAGE;
}

// Receive/parse the message body
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

// Send a number of byes from the given buffer
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

// Send the server identifier
void Message::sendServerIdentifier(const int& socket) {
    sendBytes(socket, server_identifier, sizeof(server_identifier));
}

// Send the server port
void Message::sendPort(const int& socket) {
    sendBytes(socket, &port, sizeof(port));
}

// Send the function name
void Message::sendName(const int& socket) {
    sendBytes(socket, name, sizeof(name));
}

// Send the argument types
void Message::sendArgTypes(const int& socket) {
    // Send # of arguments and then send arg types without the null
    sendBytes(socket, &num_args, sizeof(num_args));
    sendBytes(socket, arg_types, num_args * sizeof(int));
}

// Send all arguments
void Message::sendArgs(const int& socket) {
    for (int i = 0; i < num_args; ++i) {
        int buffer_size = argSize(arg_types[i]);
        sendBytes(socket, args[i], buffer_size);
    }
}

// Send the reason code
void Message::sendReasonCode(const int& socket) {
    sendBytes(socket, &reason_code, sizeof(reason_code));
}

// Send the message header
void Message::sendHeader(const int& socket) {
    sendBytes(socket, &length, sizeof(length));
    sendBytes(socket, &type, sizeof(type));
}

// Send the entire message (including header)
void Message::sendMessage(const int& socket) {
    sendHeader(socket);
    switch (type) {
        case REGISTER:
            sendServerIdentifier(socket);
            sendPort(socket);
            sendName(socket);
            sendArgTypes(socket);
            break;
        case REGISTER_SUCCESS:
            sendReasonCode(socket);
            break;
        case REGISTER_FAILURE:
            sendReasonCode(socket);
            break;
        case LOC_REQUEST:
            sendName(socket);
            sendArgTypes(socket);
            break;
        case LOC_SUCCESS:
            sendServerIdentifier(socket);
            sendPort(socket);
            break;
        case LOC_FAILURE:
            sendReasonCode(socket);
            break;
        case EXECUTE:
            sendName(socket);
            sendArgTypes(socket);
            sendArgs(socket);
            break;
        case EXECUTE_SUCCESS:
            sendName(socket);
            sendArgTypes(socket);
            sendArgs(socket);
            break;
        case EXECUTE_FAILURE:
            sendReasonCode(socket);
            break;
        case LOC_CACHE:
            sendName(socket);
            sendArgTypes(socket);
            break;
        case LOC_CACHE_SUCCESS:
            sendArgTypes(socket);
            sendArgs(socket);
            break;
        case TERMINATE:
        case NONE:
        default:
            break;
    }
}

// Recalulate the message length if arg types or message type change
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

// Free any allocated memory
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
