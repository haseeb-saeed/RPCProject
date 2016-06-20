/*
 * rpc_server.cc
 *
 * This implements the server-side RPC library.
 */

#include <cstring>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "args.h"
#include "message.h"
#include "rpc.h"

#define SOCK_INVALID -1
using namespace args;
using namespace message;
using namespace std;

static int binder_socket = SOCK_INVALID;
static int client_socket = SOCK_INVALID;
static unordered_map<string, skeleton> functions;
static vector<thread> calls;

int rpcInit() {

    // Get environment variables
    const char* binder_addr = getenv("BINDER_ADDRESS");
    const char* binder_port = getenv("BINDER_PORT");
    if (binder_addr == nullptr || binder_port == nullptr) {
        // TODO: Error code
        return -1;    
    }

    // Get information for the server address
    addrinfo hints, *addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(binder_addr, binder_port, &hints, &addr) != 0) {
        // TODO: Error code
        return -1;    
    }

    // Connect to binder
    binder_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (binder_socket == SOCK_INVALID) {
        // TODO: Error code
        return -1;    
    }

    int status = connect(binder_socket, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);
    if (status < 0) {
        // TODO: Error code
        close(binder_socket);
        return -1;
    }

    // Open socket for clients to connect to
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (client_socket == SOCK_INVALID) {
        // TODO: Error code
        close(binder_socket);
        return -1;    
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, "0", &hints, &addr) != 0) {
        // TODO: Error code
        close(binder_socket);
        close(client_socket);
        return -1; 
    }

    status = bind(client_socket, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);
    if (status < 0) {
        // TODO: Error code
        close(binder_socket);
        close(client_socket);
        return -1;    
    }
    
    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {

    // TODO: Send REGISTER request to binder and return any errors
    // Add function to local datatabse
    string key = getSignature(name, argTypes); 
    functions[key] = f;

    return 0;
}

int rpcExecute() {

    // TODO: Handle all kinds of messages
    for (;;) {
        
        MessageInfo info;
        if (getMessage(client_socket, info) < 0) {
            // TODO: Well this is awkward...
            return -1;
        }
     
        switch (info.type) {
            case MessageType::EXECUTE:
                // TODO: Stuff
                break;

            case MessageType::TERMINATE:
                // Wait for all threads to be done
                for (auto& th : calls) {
                    th.join();    
                }

                close(binder_socket);
                close(client_socket);
                break;;

            default:
                // Shit, we shouldn't be here
                return -1;
        }
    }
    
    return 0;
}

int rpcTerminate() {
    return 0;
}
