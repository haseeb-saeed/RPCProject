#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <string>
#include <memory>
#include <utility>

namespace message {

// Message types
enum MessageType {
    NONE,
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
    int length;                 // The length of the message
    MessageType type;           // The type of message
    char name[64];              // The name of the machine/function
    char server_identifier[64]; // IP address or hostname
    int port;                   // The port number
    int reason_code;            // The error code
    int num_args;               // The number of args
    int* arg_types;             // The types of args
    void** args;                // The function arguments
};

// Returns a tuple of the <length, request type>
// A pair with length <= 0 indicates error/connection closed
int getHeader(int socket, MessageInfo& info);

// Places the message body into the struct
// Returns < 0 for errors
int getMessage(int socket, MessageInfo& info);

// Sends the message constructed from info
// Returns < 0 for errors
int sendMessage(int socket, const MessageInfo& info);

}

#endif // __MESSAGE_H__
