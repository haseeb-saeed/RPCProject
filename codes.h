#ifndef __CODES_H__
#define __CODES_H__

namespace codes {
    
    enum {
        WARNING_DUPLICATE_FUNCTION = 1,             // The server is attempting to register a function that has already been registered
        
        ERROR_MISSING_ENV = -1,                     // The BINDER_ADDRESS or BINDER_PORT env variables have not been manually set
        ERROR_ADDRINFO = -2,                        // If getaddrinfo fails, ie the getaddrinfo method returns a negative error code
        ERROR_SOCKET_CREATE = -3,                   // If creating a socket fails, ie the socket method returns a negative error code
        ERROR_SOCKET_CONNECT = -4,                  // If connecting to a socket fails, ie the connect method returns a negative error code
        ERROR_SOCKET_BIND = -5,                     // If binding to a socket fails, ie the bind method returns a negative error code
        ERROR_SOCKET_LISTEN = -6,                   // If listening to a socket fails, ie the listen method returns a negative error code
        ERROR_SOCKET_SELECT = -7,                   // If select fails, ie the select method returns a negative error code
        ERROR_SOCKET_ACCEPT = -8,                   // If accept fails, ie the accept method returns a negative error code
        ERROR_SOCKET_NAME = -9,                     // If getting the socket name fails, ie the getsockname method returns a negative error code
        ERROR_HOSTNAME = -10,                       // If getting the host name fails, ie the gethostname method returns a negative error code
        ERROR_MESSAGE_SEND = -11,                   // If sending a message fails, ie the send method returns a negative error code
        ERROR_MESSAGE_RECV = -12,                   // If recieving a message fails, ie the recv method returns a negative error code
        ERROR_MISSING_FUNCTION = -13,               // If the desired function does not exist on the server or binder
        ERROR_FUNCTION_CALL = -14,                  // If the function called returns a negative error code
        ERROR_NOT_CONNECTED_BINDER = -15,           // The server is not connected to the binder, ie the binder socket has not been created on the server
        ERROR_SERVER_NOT_RUNNING = -16,             // The server is not running, ie the socket for clients to connect to has not been created
        ERROR_LOST_CONNECTION_BINDER = -17,         // The binder disconnected from the server
    };
}

#endif // __CODES_H__
