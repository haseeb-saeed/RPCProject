/*
 * rpc_client.cc
 *
 * This implements the client-side RPC library.
 */
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "rpc.h"
#include "codes.h"
#include "message.h"
using namespace std;
using namespace message;
using namespace codes;

int connectToBinder() {
    // Get environment variables
    const char* binder_addr = getenv("BINDER_ADDRESS");
    const char* binder_port = getenv("BINDER_PORT");
    if (binder_addr == nullptr || binder_port == nullptr) {
        return ERROR_MISSING_ENV;    
    }

    int status;
    addrinfo host_info, *host_info_list;
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // Getaddrinfo for binder
    status = getaddrinfo(binder_addr, binder_port, &host_info, &host_info_list);
    if (status != 0) {
        return ERROR_ADDRINFO;
    }

    // Create binder socket
    int binder_socket;
    binder_socket = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (binder_socket == -1) {
        return ERROR_SOCKET_CREATE;
    }

    // Connect to binder socket
    status = connect(binder_socket, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        close(binder_socket);
        return ERROR_SOCKET_CONNECT;
    }

    freeaddrinfo(host_info_list);

    return binder_socket;
}

// Called by client
int rpcCall(char* name, int* argTypes, void** args) {
   
    // Maybe it would be better to only connect to the
    // binder once instead of every time we do an RPC?
 
    // TODO:
    // Create socket and connect to binder
    // If socket creation/connection fails, return error
    // Send LOC_REQUEST to binder
    // If reply is LOC_FAILURE, return the failure code
    // Close server connection
    // Extract server info from reply
    // Create new socket to connect to server
    // If creation/connection fails, return error
    // Send EXECUTE to the server
    // If reply is SERVER_FAILURE, return the failure code
    // Copy reply arguments to args
    
    int binder_socket = connectToBinder();
    if (binder_socket < 0) {
        return binder_socket;
    }

    Message msg;
    msg.setType(MessageType::LOC_REQUEST);
    msg.setName(name);
    msg.setArgTypes(argTypes);

    if (msg.sendMessage(binder_socket) < 0) {
        close(binder_socket);
        return ERROR_MESSAGE_SEND;
    }

    if (msg.recvHeader(binder_socket) < 0) {
        close(binder_socket);
        return ERROR_MESSAGE_RECV;
    }

    if (msg.recvMessage(binder_socket) <= 0) {
        close(binder_socket);
        return ERROR_MESSAGE_RECV;
    }

    if (msg.getType() == MessageType::LOC_FAILURE) {
        close(binder_socket);
        return msg.getReasonCode();
    }

    int status;
    addrinfo host_info, *host_info_list;
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // Getaddrinfo for server
    status = getaddrinfo(msg.getServerIdentifier(), to_string(msg.getPort()).c_str(), &host_info, &host_info_list);
    if (status != 0) {
        close(binder_socket);
        return ERROR_ADDRINFO;
    }

    // Create server socket
    int server_socket;
    server_socket = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (server_socket == -1) {
        close(binder_socket);
        return ERROR_SOCKET_CREATE;
    }

    // Connect to server socket
    status = connect(server_socket, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        close(binder_socket);
        close(server_socket);
        return ERROR_SOCKET_CONNECT;
    }

    freeaddrinfo(host_info_list);

    Message executeMsg;
    executeMsg.setType(MessageType::EXECUTE);
    executeMsg.setName(name);
    executeMsg.setArgTypes(argTypes);
    executeMsg.setArgs(args);

    if (executeMsg.sendMessage(server_socket) < 0) {
        close(binder_socket);
        close(server_socket);
        return ERROR_MESSAGE_SEND;
    }

    if (executeMsg.recvHeader(server_socket) <= 0) {
        close(binder_socket);
        close(server_socket);
        return ERROR_MESSAGE_RECV;
    }

    if (executeMsg.recvMessage(server_socket) <= 0) {
        close(binder_socket);
        close(server_socket);
        return ERROR_MESSAGE_RECV;
    }

    if (executeMsg.getType() == MessageType::EXECUTE_FAILURE) {
        close(binder_socket);
        close(server_socket);
        return executeMsg.getReasonCode();
    }

    memcpy(&argTypes, executeMsg.getArgTypes(), executeMsg.numArgs()*sizeof(*argTypes));
    // TODO: Fixme! Change to deep copy
    memcpy(&args, executeMsg.getArgs(), executeMsg.numArgs()*sizeof(*args));
    close(binder_socket);
    close(server_socket);
    return 0;
}

int rpcTerminate() {
    Message msg;
    msg.setType(MessageType::TERMINATE);

    int binder_socket = connectToBinder();
    if (binder_socket < 0) {
        return binder_socket;
    }

    if (msg.sendMessage(binder_socket) < 0) {
        close(binder_socket);
        return ERROR_MESSAGE_SEND;
    }

    close(binder_socket);
    return 0;
}
