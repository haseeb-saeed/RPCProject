/*
 * rpc_client.cc
 *
 * This implements the client-side RPC library.
 */

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
    return 0;
}
