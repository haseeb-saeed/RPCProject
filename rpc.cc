/*
 * rpc.c
 *
 * This implements the RPC library.
 */

#include "rpc.h"
using namespace std;

// Called by server
int rpcInit() {

    // TODO:
    // Create and bind a socket for client requests
    // Return error if socket creation or binding fails
    // Create another socket for connecting to the binder
    // Return error if socket creation fails
    // Fetch the binder's address/port from environment variables
    // Return error if environment variables don't exist
    // Connect to the binder
    // Return error if connection fails
    return 0;
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
    return 0;
}

// Probably for the bonus part - forget about this for now
int rpcCacheCall(char* name, int* argTypes, void** args) {
    return 0;
}

// Called by server
int rpcRegister(char* name, int* argTypes, skeleton f) {
    return 0;
}

// Called by server
int rpcExecute() {
    return 0;
}

// Called by client
int rpcTerminate() {
    return 0;
}
