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
static unordered_map<int, MessageInfo> client_info;
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

static void executeAsync(int client) {

    auto& info = client_info[client];
    string key = getSignature(info.name.c_str(), info.arg_types);
    if (functions[key] == nullptr) {
        // TODO: Send back EXECUTE_FAILED with function not found    
    }

    int status = (functions[key])(info.arg_types, info.args);
    if (status < 0) {
        // TODO: Send back EXECUTE_FAILED with function error    
    }

    // TODO: Since we're most likely done with info here, clean up
    // the allocated memory
    client_info.erase(client);
    close(client);
}

int rpcExecute() {

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(client_socket, &master_set);
    int max_socket = client_socket;

    if (listen(client_socket, 5) < 0) {
        // TODO: Error code
        return -1;
    }

    for (;;) {
        read_set = master_set;
        select(max_socket + 1, &read_set, nullptr, nullptr, nullptr);    
        
        for (int i = 0; i <= max_socket; ++i) {
            if (!FD_ISSET(i, &read_set)) {
                continue;
            }    
            
            if (i == client_socket) {
                // Accept the incoming connection
                int client = accept(client_socket, nullptr, nullptr);
                if (client != SOCK_INVALID) {
                    FD_SET(i, &master_set);
                    max_socket = max(max_socket, client); 
                }
            } else {                
                auto& info = client_info[i];
                if (info.type == MessageType::NONE) {
                    // TODO: Read the message header
                    // Peek at the header and continue
                    // if we don't have all 8 bytes
                    getHeader(i, info);

                    if (info.type == MessageType::TERMINATE) {
                        // Wait for all threads to be done
                        for (auto& th : calls) {
                            th.join();    
                        }
                        break;
                    } 
                } else {
                    // TODO: Read the message body
                    // Peek at the body and continue if
                    // we don't have the full length
                    getMessage(i, info);

                    FD_CLR(i, &master_set);
                    thread th(executeAsync, i);
                    calls.push_back(move(th));
                }
            }
        } 
    }
   
    close(binder_socket);
    close(client_socket);
 
    return 0;
}
