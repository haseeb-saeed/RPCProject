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
    LOC_CACHE,
    LOC_CACHE_SUCCESS,
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
    char server_identifier[48]; // IP address or hostname
    int port;                   // The port number
    int reason_code;            // The error code
    int num_args;               // The number of args
    int* arg_types;             // The types of args
    void** args;                // The function arguments

public:
    Message();                          // Default constructor
    ~Message();                         // Destructor
    Message(const Message& msg) = delete;
    Message& operator=(const Message& msg) = delete;

    struct PeekError {};
    struct SendError {};
    struct RecvError {};

    void recvHeader(const int& socket);  // Gets a message header from the given socket
    void recvMessage(const int& socket); // Gets a message body from the given socket
    int peek(const int& socket);         // Gets the number of bytes in the socket's buffer
    void sendMessage(const int& socket); // Sends a message to the given socket

    void setType(const MessageType& type);
    void setName(const char* name);
    void setServerIdentifier(const char* identifier);
    void setPort(const int& port);
    void setReasonCode(const int& reason_code);
    void setArgTypes(int* arg_types);
    void setArgs(void** args);

    MessageType getType() const;
    const char* getName() const;
    const char* getServerIdentifier() const;
    int getPort() const;
    int getReasonCode() const;
    int* getArgTypes() const;
    void** getArgs() const;

    int getLength() const;
    int numArgs() const;

    const int HEADER_SIZE;

private:
    void recvName(const int& socket);
    void recvServerIdentifier(const int& socket);
    void recvPort(const int& socket);
    void recvReasonCode(const int& socket);
    void recvArgTypes(const int& socket);
    void recvArgs(const int& socket);

    void sendBytes(const int& socket, const void* buffer, const int& buffer_size);
    void sendHeader(const int& socket);
    void sendName(const int& socket);
    void sendServerIdentifier(const int& socket);
    void sendPort(const int& socket);
    void sendReasonCode(const int& socket);
    void sendArgTypes(const int& socket);
    void sendArgs(const int& socket);

    void recalculateLength();
    void cleanup();
};

}

#endif // __MESSAGE_H__
