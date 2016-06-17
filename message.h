#ifndef __MESSAGE_H__
#define __MESSAGE_H__

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

// Returns a tuple of the <length, request type>
// A pair with length <= 0 indicates error/connection closed
std::pair<int, MessageType> getHeader(int socket);

// TODO:
// Add functions to parse each type of response message
}

#endif // __MESSAGE_H__
