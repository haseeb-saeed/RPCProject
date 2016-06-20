#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <string>
#include <memory>
#include <utility>

namespace message {

// Message types
enum MessageType {
    REGISTER,
    REGISTER_SUCCESS,
    REGISTER_FAILURE,
    LOC_REQUEST,
    LOC_SUCCESS,
    LOC_FAILURE,
    EXECUTE,
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE,
    TERMINATE
};

// Message info
struct MessageInfo {
    int length;             // The length of the message
    MessageType type;       // The type of message
    std::string name;       // The name of the machine/function
    std::string server_identifier;          // IP address or hostname
    int port;               // The port number
    int reason_code;        // The error code
    int num_args;           // The number of args
    std::unique_ptr<int[]> arg_types;       // The types of args
    std::unique_ptr<void*[]> args;          // The function arguments
};

// Returns a tuple of the <length, request type>
// A pair with length <= 0 indicates error/connection closed
// std::pair<int, MessageType> getHeader(int socket);

// Places the message body into the struct
// Returns < 0 for errors
int getMessage(int socket, MessageInfo& info);

}

#endif // __MESSAGE_H__
