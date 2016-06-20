/*
 * rpc_server.cc
 *
 * This implements the server-side RPC library.
 */

#include "rpc.h"
using namespace std;

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

int rpcRegister(char* name, int* argTypes, skeleton f) {
    return 0;
}

int rpcExecute() {
    return 0;
}

int rpcTerminate() {
    return 0;
}
