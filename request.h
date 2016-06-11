#ifndef __REQUEST_H__
#define __REQUEST_H__

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

// TODO:
// Add functions to get length and type of response message
// Add functions to parse each type of response message

#endif // __REQUEST_H__
