#ifndef __CODES_H__
#define __CODES_H__

namespace codes {
    
    enum {
        WARNING_DUPLICATE_FUNCTION = 1,
        
        ERROR_MISSING_ENV = -1,
        ERROR_ADDRINFO = -2, 
        ERROR_SOCKET_CREATE = -3,
        ERROR_SOCKET_CONNECT = -4,
        ERROR_SOCKET_BIND = -5,
        ERROR_SOCKET_LISTEN = -6,
        ERROR_SOCKET_SELECT = -7,
        ERROR_SOCKET_ACCEPT = -8,
        ERROR_SOCKET_NAME = -9,
        ERROR_HOSTNAME = -10,
        ERROR_MESSAGE_SEND = -11,
        ERROR_MESSAGE_RECV = -12,
        ERROR_MISSING_FUNCTION = -13,
    };
}

#endif // __CODES_H__
