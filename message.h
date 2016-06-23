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

// Message
class Message {
    int length;                 // The length of the message
    MessageType type;           // The type of message
    char name[64];              // The name of the machine/function
    char server_identifier[64]; // IP address or hostname
    int port;                   // The port number
    int reason_code;            // The error code
    int num_args;               // The number of args
    int* arg_types;             // The types of args
    void** args;                // The function arguments

public:
    Message();                          // Default constructor
    int recvHeader(const int& socket);  // Gets a message header from the given socket
    int recvMessage(const int& socket); // Gets a message body from the given socket
    int peekHeader(const int& socket);  // Checks if all header bytes have arrived
    int peekMessage(const int& socket); // Checks if all message body bytes have arrived
    int sendMessage(const int& socket); // Sends a message to the given socket

    void setType(const MessageType& type);
    void setName(const char* name);
    void setServerIdentifier(const char* identifier);
    void setPort(const int& port);
    void setReasonCode(const int& reason_code);
    void setArgTypes(int* arg_types);
    void setArgs(void** args);

    MessageType getType() const;
    const char* getName() const;
    const char* getServerIndentifier() const;
    int getPort() const;
    int getReasonCode() const;
    int* getArgTypes() const;
    void** getArgs() const;

    int getLength() const;
    int numArgs() const;

    const int HEADER_SIZE;

private:
    int recvName(const int& socket);
    int recvServerIdentifier(const int& socket);
    int recvPort(const int& socket);
    int recvReasonCode(const int& socket);
    int recvArgTypes(const int& socket);
    int recvArgs(const int& socket);

    int sendBytes(const int& socket, const void* buffer, const int& buffer_size);
    int sendHeader(const int& socket);
    int sendName(const int& socket);
    int sendServerIdentifier(const int& socket);
    int sendPort(const int& socket);
    int sendReasonCode(const int& socket);
    int sendArgTypes(const int& socket);
    int sendArgs(const int& socket);

    void recalculateLength();
};

}

#endif // __MESSAGE_H__
