/*
 * rpc_client.cc
 *
 * This implements the client-side RPC library.
 */
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "rpc.h"
using namespace std;

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
    
    int status;
    addrinfo host_info, *host_info_list;
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // Getaddrinfo
    status = getaddrinfo(NULL, "0", &host_info, &host_info_list);
    if (status != 0) cerr << "getaddrinfo error" << gai_strerror(status) << endl;

    // Create socket
    int socketfd;
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (socketfd == -1) cerr << "socket error " << endl;

    // Connect
    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) cerr << "connect error" << endl;

    // Send LOC_REQUEST
    //char* padded_name[64] = name;
    //int len = strlen(msg) + 1;
    //ssize_t bytes_sent;
    // Send length
    //bytes_sent = send(socketfd, &len, sizeof(len), 0);
    // Send type
    //bytes_sent = send(socketfd, msg, len, 0);

    return 0;
}

int rpcTerminate() {
        return 0;
}
