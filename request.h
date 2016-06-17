#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <utility>

namespace request {

// Request types
enum Request {
    REGISTER,
    LOC_REQUEST,
    EXECUTE,
    TERMINATE
};

// Response types
enum Response {
    REGISTER_SUCCESS,
    REGISTER_FAILURE,
    LOC_SUCCESS,
    LOC_FAILURE,
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE
};

// Returns a tuple of the <length, request type>
// A pair with length <= 0 indicates error/connection closed
std::pair<int, Request> getHeader(int socket);

// TODO:
// Add functions to parse each type of response message
}

#endif // __REQUEST_H__
