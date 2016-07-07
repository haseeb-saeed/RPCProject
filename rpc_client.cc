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
#include <unordered_map>
#include <vector>

#include "args.h"
#include "rpc.h"
#include "codes.h"
#include "message.h"
using namespace std;
using namespace message;
using namespace codes;
using namespace args;

typedef pair<string, int> Location;
unordered_map<string, vector<Location>> cache;

int connectToBinder() {
    // Get environment variables
    const char* binder_addr = getenv("BINDER_ADDRESS");
    const char* binder_port = getenv("BINDER_PORT");
    if (binder_addr == nullptr || binder_port == nullptr) {
        return ERROR_MISSING_ENV;    
    }

    // Initialization and setup
    int status;
    addrinfo host_info, *host_info_list;
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // Getaddrinfo for binder, return error if fails
    status = getaddrinfo(binder_addr, binder_port, &host_info, &host_info_list);
    if (status != 0) {
        return ERROR_ADDRINFO;
    }

    // Create binder socket, return error if fails
    int binder_socket;
    binder_socket = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (binder_socket == -1) {
        return ERROR_SOCKET_CREATE;
    }

    // Connect to binder socket, return error if fails
    status = connect(binder_socket, host_info_list->ai_addr, host_info_list->ai_addrlen);
    freeaddrinfo(host_info_list);
    if (status == -1) {
        close(binder_socket);
        return ERROR_SOCKET_CONNECT;
    }

    // Return binder socket
    return binder_socket;
}

int connectToServer(const char* host_name, const char* port) {

    // Initialization and setup
    int status;
    addrinfo host_info, *host_info_list;
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // Getaddrinfo for server, return error if fails
    status = getaddrinfo(host_name, port, &host_info, &host_info_list);
    if (status != 0) {
        return ERROR_ADDRINFO;
    }

    // Create server socket, return error if fails
    int server_socket;
    server_socket = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (server_socket == -1) {
        return ERROR_SOCKET_CREATE;
    }

    // Connect to server socket, return error if fails
    status = connect(server_socket, host_info_list->ai_addr, host_info_list->ai_addrlen);
    freeaddrinfo(host_info_list);
    if (status == -1) {
        close(server_socket);
        return ERROR_SOCKET_CONNECT;
    }

    // Return server socket
    return server_socket;
}

int callServer(const char* identifier, const char* port, const char* name,
    int* argTypes, void** args) {

    // Connect to serever
    int server_socket = connectToServer(identifier, port);
    if (server_socket < 0) {
        return server_socket;
    }

    // Create EXECUTE message
    Message executeMsg;
    executeMsg.setType(MessageType::EXECUTE);
    executeMsg.setName(name);
    executeMsg.setArgTypes(argTypes);
    executeMsg.setArgs(args);

    // Attempt to send EXECUTE message to server, and recv reply
    // If either fails, return appropriate error
    try {
        executeMsg.sendMessage(server_socket);
        executeMsg.recvBlock(server_socket);
    } catch (Message::SendError) {
        close(server_socket);
        return ERROR_MESSAGE_SEND;
    } catch (Message::RecvError) {
        close(server_socket);
        return ERROR_MESSAGE_RECV;
    }

    // If server replies with EXECUTE_SUCCESS, copy arguments to args and argTypes
    if (executeMsg.getType() == MessageType::EXECUTE_SUCCESS) {
        copyArgTypes(argTypes, executeMsg.getArgTypes());
        copyArgs(args, executeMsg.getArgs(), executeMsg.getArgTypes());
        executeMsg.setReasonCode(0);
    }

    // Close socket
    close(server_socket);
    return executeMsg.getReasonCode();
}

int rpcCall(char* name, int* argTypes, void** args) {

    // Connect to binder
    int binder_socket = connectToBinder();
    if (binder_socket < 0) {
        return binder_socket;
    }

    // Create LOC_REQUEST message
    Message msg;
    msg.setType(MessageType::LOC_REQUEST);
    msg.setName(name);
    msg.setArgTypes(argTypes);

    // Send LOC_REQUEST message to the binder
    // Recv reply from the binder
    // If either fails, close socket and return appropriate error
    try {
        msg.sendMessage(binder_socket);
        msg.recvBlock(binder_socket);
    } catch (Message::SendError) {
        close(binder_socket);
        return ERROR_MESSAGE_SEND;
    } catch (Message::RecvError) {
        close(binder_socket);
        return ERROR_MESSAGE_RECV;
    }

    // If binder returns LOC_FAILURE, close socket and return error code
    if (msg.getType() == MessageType::LOC_FAILURE) {
        close(binder_socket);
        return msg.getReasonCode();
    }

    // Now that we have the server info from the binder reply,
    // call the server using this info
    close(binder_socket);
    return callServer(msg.getServerIdentifier(),
        to_string(msg.getPort()).c_str(), name, argTypes, args);
}

int rpcCacheCall(char* name, int* argTypes, void** args) {

    const string key = getSignature(name, argTypes);
    auto& list = cache[key];

    // Already cached, so call server with pairs of args from list
    for (const auto& location : list) {
        if (callServer(location.first.c_str(), to_string(location.second).c_str(),
            name, argTypes, args) == 0) {
            return 0;
        }
    }

    // Connect to binder
    int binder_socket = connectToBinder();
    if (binder_socket < 0) {
        return binder_socket;
    }

    // Create LOC_CACHE message
    Message msg;
    msg.setType(MessageType::LOC_CACHE);
    msg.setName(name);
    msg.setArgTypes(argTypes);

    // Send LOC_CACHE message to the binder
    // Recv reply from the binder
    // If either fails, close socket and return appropriate error
    try {
        msg.sendMessage(binder_socket);
        msg.recvBlock(binder_socket);
    } catch(Message::SendError) {
        close(binder_socket);
        return ERROR_MESSAGE_SEND;
    } catch(Message::RecvError) {
        close(binder_socket);
        return ERROR_MESSAGE_RECV;
    }

    // Close socket, and if binder replied with LOC_FAILURE, return error
    close(binder_socket);
    if (msg.getType() == MessageType::LOC_FAILURE) {
        return msg.getReasonCode();
    }

    list.clear();
    // Parsing args from binder reply
    auto msg_args = msg.getArgs();
    for (int i = 0; i < msg.numArgs(); i += 2) {
        list.push_back(make_pair((char*)msg_args[i], *(int*)(msg_args[i + 1])));
    }
    
    // call server using the pairs of args from list
    for (const auto& location : list) {
        if (callServer(location.first.c_str(), to_string(location.second).c_str(),
            name, argTypes, args) == 0) {
            return 0;
        }
    }

    return ERROR_MISSING_FUNCTION;
}

int rpcTerminate() {
    // Create TERMINATE message
    Message msg;
    msg.setType(MessageType::TERMINATE);

    // Connect to binder
    int binder_socket = connectToBinder();
    if (binder_socket < 0) {
        return binder_socket;
    }

    // Send TERMINATE message to binder
    // If sending fails, close socket and return error
    try {
        msg.sendMessage(binder_socket); 
    } catch(Message::SendError) {
        close(binder_socket);
        return ERROR_MESSAGE_SEND;
    }

    // Close socket
    close(binder_socket);
    return 0;
}
