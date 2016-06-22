#include <utility>
#include <iostream>

#include <sys/socket.h>

#include "args.h"
#include "message.h"
using namespace args;
using namespace std;

namespace message {

MessageInfo::MessageInfo(): length(0), type(MessageType::NONE), port(0),
    reason_code(0), num_args(0), arg_types(nullptr), args(nullptr) {
}

static int getServerIdentifier(int socket, MessageInfo& info) {
    int status = recv(socket, &info.server_identifier, sizeof(info.server_identifier), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv server identifier error" << endl;
    }
    return status;
}

static int getPort(int socket, MessageInfo& info) {
    int status = recv(socket, &info.port, sizeof(info.port), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv port error" << endl;
    }
    return status;
}

static int getName(int socket, MessageInfo& info) {
    int status = recv(socket, &info.name, sizeof(info.name), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv name error" << endl;
    }
    return status;
}

static int getArgTypes(int socket, MessageInfo& info) {
    int status = recv(socket, &info.num_args, sizeof(info.num_args), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv num args error" << endl;    
    } else {
        info.arg_types = new int[info.num_args + 1];    // +1 for null terminator
        info.args = new void*[info.num_args];
        status = recv(socket, info.arg_types, info.num_args * sizeof(int), MSG_WAITALL);
        if (status <= 0) {
            info.length = status;
            delete [] info.arg_types;
            delete [] info.args;
            
            info.arg_types = nullptr;
            info.args = nullptr;
            cerr << "recv arg types error" << endl;        
        } else {
            info.arg_types[info.num_args] = 0;    
        }
    }

    return status;
}

static int getArgs(int socket, MessageInfo& info) {

    for (int i = 0; i < info.num_args; ++i) {
        int num = arrayLen(info.arg_types[i]);
        if (num == 0) {
            num += 1;   // For a non-array, we only want space for one arg
        }        
        
        void* buffer;
        int buffer_size;
        
        if (isChar(info.arg_types[i])) {
            buffer = (void*) new char[num];
            buffer_size = num * sizeof(char);
        } 
        else if (isShort(info.arg_types[i])) {
            buffer = (void*) new short[num];
            buffer_size = num * sizeof(short);
        } 
        else if (isInt(info.arg_types[i])) {
            buffer = (void*) new int[num];
            buffer_size = num * sizeof(int);
        } 
        else if (isLong(info.arg_types[i])) {
            buffer = (void*) new long[num];
            buffer_size = num * sizeof(long);
        } 
        else if (isFloat(info.arg_types[i])) {
            buffer = (void*) new float[num];
            buffer_size = num * sizeof(float);
        } 
        else if (isDouble(info.arg_types[i])) {
            buffer = (void*) new double[num];
            buffer_size = num * sizeof(double);
        } else {
            // We should never hit this    
        }

        int status = recv(socket, buffer, buffer_size, MSG_WAITALL);
        if (status <= 0) {
            info.length = status;
            for (int j = 0; j <= i; ++j) {
                delete [] info.args[j];
            }

            delete [] info.arg_types;
            delete [] info.args;

            info.arg_types = nullptr;
            info.args = nullptr;
            cerr << "recv arg error" << endl;  
            
            return status;
        }

        info.args[i] = buffer;
    }

    return 0;
}

static int getReasonCode(int socket, MessageInfo& info) {
    int status = recv(socket, &info.reason_code, sizeof(info.reason_code), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv code error" << endl;
    }
    return status;
}

int getHeader(int socket, MessageInfo& info) {
    // Get the length of the message
    int status = recv(socket, &info.length, sizeof(info.length), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv length error" << endl;
    }
    
    // Get the message type
    status = recv(socket, &info.type, sizeof(info.type), MSG_WAITALL);
    if (status <= 0) {
        info.length = status;
        cerr << "recv message type error" << endl;
    }
    return status;
}

int getMessage(int socket, MessageInfo& info) {
    switch (info.type) {
        case REGISTER:
            getServerIdentifier(socket, info);
            getPort(socket, info);
            getName(socket, info);
            getArgTypes(socket, info);
            break;
        case REGISTER_SUCCESS:
            getReasonCode(socket, info);
            break;
        case REGISTER_FAILURE:
            getReasonCode(socket, info);
            break;
        case LOC_REQUEST:
            getName(socket, info);
            getArgTypes(socket, info);
            break;
        case LOC_SUCCESS:
            getServerIdentifier(socket, info);
            getPort(socket, info);
            break;
        case LOC_FAILURE:
            getReasonCode(socket, info);
            break;
        case EXECUTE:
            getName(socket, info);
            getArgTypes(socket, info);
            getArgs(socket, info);
            break;
        case EXECUTE_SUCCESS:
            getName(socket, info);
            getArgTypes(socket, info);
            getArgs(socket, info);
            break;
        case EXECUTE_FAILURE:
            getReasonCode(socket, info);
            break;
        case TERMINATE:
            break;
    }

    return 0;
}

static int sendBytes(int socket, const void* buffer, int buffer_size) {
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

static int sendServerIdentifier(int socket, const MessageInfo& info) {
    return sendBytes(socket, info.server_identifier, sizeof(info.server_identifier));
}

static int sendPort(int socket, const MessageInfo& info) {
    return sendBytes(socket, &info.port, sizeof(info.port));
}

static int sendName(int socket, const MessageInfo& info) {
    return sendBytes(socket, info.name, sizeof(info.name));
}

static int sendArgTypes(int socket, const MessageInfo& info) {
    // Send # of arguments and then send arg types without the null
    int status = sendBytes(socket, &info.num_args, sizeof(info.num_args));
    if (status < 0) {
        return status;
    }
    return sendBytes(socket, info.arg_types, info.num_args * sizeof(int));
}

static int sendArgs(int socket, const MessageInfo& info) {

    for (int i = 0; i < info.num_args; ++i) {
        int num = arrayLen(info.arg_types[i]);
        if (num == 0) {
            num += 1;   // For a non-array, we only want space for one arg
        }        
        
        int buffer_size;
        if (isChar(info.arg_types[i])) {
            buffer_size = num * sizeof(char);
        } 
        else if (isShort(info.arg_types[i])) {
            buffer_size = num * sizeof(short);
        } 
        else if (isInt(info.arg_types[i])) {
            buffer_size = num * sizeof(int);
        } 
        else if (isLong(info.arg_types[i])) {
            buffer_size = num * sizeof(long);
        } 
        else if (isFloat(info.arg_types[i])) {
            buffer_size = num * sizeof(float);
        } 
        else if (isDouble(info.arg_types[i])) {
            buffer_size = num * sizeof(double);
        } else {
            // We should never hit this    
        }

        int status = sendBytes(socket, info.args[i], buffer_size);
        if (status < 0) {
            return status;
        }
    }

    return 0;
}

static int sendReasonCode(int socket, const MessageInfo& info) {
    return sendBytes(socket, &info.reason_code, sizeof(info.reason_code));
}

static int sendHeader(int socket, const MessageInfo& info) {
    int status = sendBytes(socket, &info.length, sizeof(info.length));
    if (status < 0) {
        return status;
    }
    return sendBytes(socket, &info.type, sizeof(info.type));
}

int sendMessage(int socket, const MessageInfo& info) {

    sendHeader(socket, info);
    switch (info.type) {
        case REGISTER:
            sendServerIdentifier(socket, info);
            sendPort(socket, info);
            sendName(socket, info);
            sendArgTypes(socket, info);
            break;
        case REGISTER_SUCCESS:
            sendReasonCode(socket, info);
            break;
        case REGISTER_FAILURE:
            sendReasonCode(socket, info);
            break;
        case LOC_REQUEST:
            sendName(socket, info);
            sendArgTypes(socket, info);
            break;
        case LOC_SUCCESS:
            sendServerIdentifier(socket, info);
            sendPort(socket, info);
            break;
        case LOC_FAILURE:
            sendReasonCode(socket, info);
            break;
        case EXECUTE:
            sendName(socket, info);
            sendArgTypes(socket, info);
            sendArgs(socket, info);
            break;
        case EXECUTE_SUCCESS:
            sendName(socket, info);
            sendArgTypes(socket, info);
            sendArgs(socket, info);
            break;
        case EXECUTE_FAILURE:
            sendReasonCode(socket, info);
            break;
        case TERMINATE:
            break;
    }

    return 0;
}

}
