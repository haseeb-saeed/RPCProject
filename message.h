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
    int length;                         // The length of the message
    MessageType type;                   // The type of message
    char name[64];                      // The name of the machine/function
    char server_identifier[48];         // IP address or hostname
    int port;                           // The port number
    int reason_code;                    // The error code
    int num_args;                       // The number of args
    int* arg_types;                     // The types of args
    void** args;                        // The function arguments
    std::unique_ptr<char[]> raw_bytes;  // Raw message data
    int raw_index;                      // Index into raw message data
    int total_bytes;                    // Total number of bytes received
    char flags;                         // Message flags
    const int HEADER_SIZE;              // The size of the message header

    enum {
        END_OF_HEADER = 0x1,
        END_OF_MESSAGE = 0x10,
    };

public:
    Message();
    ~Message();
    Message(const Message& msg) = delete;
    Message& operator=(const Message& msg) = delete;

    // Exceptions to throw
    struct PeekError {};
    struct SendError {};
    struct RecvError {};

    // Send/receive a message
    void sendMessage(const int& socket);
    void recvBlock(const int& socket);
    void recvNonBlock(const int& socket);

    // Setters
    void setType(const MessageType& type);
    void setName(const char* name);
    void setServerIdentifier(const char* identifier);
    void setPort(const int& port);
    void setReasonCode(const int& reason_code);
    void setArgTypes(int* arg_types);
    void setArgs(void** args);

    // Getters
    MessageType getType() const;
    const char* getName() const;
    const char* getServerIdentifier() const;
    int getPort() const;
    int getReasonCode() const;
    int* getArgTypes() const;
    void** getArgs() const;

    int getLength() const;
    int numArgs() const;
    bool eom() const;


private:
    // Receiving helper functions
    void recvBytes(const int& socket, const int& max_bytes);
    void recvHeader();
    void recvMessage();
    void recvName();
    void recvServerIdentifier();
    void recvPort();
    void recvReasonCode();
    void recvArgTypes();
    void recvArgs();

    // Sending helper functions
    void sendBytes(const int& socket, const void* buffer, const int& buffer_size);
    void sendHeader(const int& socket);
    void sendName(const int& socket);
    void sendServerIdentifier(const int& socket);
    void sendPort(const int& socket);
    void sendReasonCode(const int& socket);
    void sendArgTypes(const int& socket);
    void sendArgs(const int& socket);

    // Miscellaneous helper functions
    void recalculateLength();
    void cleanup();
    void parse(void* dst, const int& buffer_size);
};

}

#endif // __MESSAGE_H__
