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
#include <iostream>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "args.h"
#include "codes.h"
#include "message.h"
#include "rpc.h"

#define SOCK_INVALID -1
using namespace args;
using namespace codes;
using namespace message;
using namespace std;

static int binder_socket = SOCK_INVALID;
static int client_socket = SOCK_INVALID;
static unordered_map<string, skeleton> functions;
static unordered_map<int, Message> requests;
static vector<thread> calls;
static int host_port = 0;
static char host_name[48];

int rpcInit() {

    // Get environment variables
    const char* binder_addr = getenv("BINDER_ADDRESS");
    const char* binder_port = getenv("BINDER_PORT");
    if (binder_addr == nullptr || binder_port == nullptr) {
        return ERROR_MISSING_ENV;    
    }

    // Get information for the server address
    addrinfo hints, *addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(binder_addr, binder_port, &hints, &addr) != 0) {
        return ERROR_ADDRINFO;    
    }

    // Connect to binder
    binder_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (binder_socket == SOCK_INVALID) {
        return ERROR_SOCKET_CREATE;
    }

    int status = connect(binder_socket, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);
    if (status < 0) {
        close(binder_socket);
        binder_socket = SOCK_INVALID;
        return ERROR_SOCKET_CONNECT;
    }

    // Open socket for clients to connect to
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (client_socket == SOCK_INVALID) {
        close(binder_socket);
        binder_socket = SOCK_INVALID;
        return ERROR_SOCKET_CREATE;    
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, "0", &hints, &addr) != 0) {
        close(binder_socket);
        close(client_socket);
        binder_socket = SOCK_INVALID;
        client_socket = SOCK_INVALID;
        return ERROR_ADDRINFO;
    }

    status = bind(client_socket, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);
    if (status < 0) {
        close(binder_socket);
        close(client_socket);
        binder_socket = SOCK_INVALID;
        client_socket = SOCK_INVALID;
        return ERROR_SOCKET_BIND;    
    }
   
    if (listen(client_socket, 5) < 0) {
        close(binder_socket);
        close(client_socket);
        binder_socket = SOCK_INVALID;
        client_socket = SOCK_INVALID;
        return ERROR_SOCKET_LISTEN;
    }
 
    // Get the server name and port
    sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    if (getsockname(client_socket, (sockaddr*)&server_addr, &len) < 0) {
        close(binder_socket);
        close(client_socket);
        binder_socket = SOCK_INVALID;
        client_socket = SOCK_INVALID;
        return ERROR_SOCKET_NAME;
    }

    if (gethostname(host_name, sizeof(host_name)) < 0) {
        close(binder_socket);
        close(client_socket);
        binder_socket = SOCK_INVALID;
        client_socket = SOCK_INVALID;
        return ERROR_HOSTNAME;
    }

    auto host = gethostbyname(host_name);
    if (host == nullptr) {
        close(binder_socket);
        close(client_socket);
        binder_socket = SOCK_INVALID;
        client_socket = SOCK_INVALID;
        return ERROR_HOSTNAME;
    }

    host_port = ntohs(server_addr.sin_port);
    memcpy(host_name, host->h_name, strlen(host->h_name) + 1);

    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {

    if (binder_socket == SOCK_INVALID) {
        return ERROR_NOT_CONNECTED_BINDER;
    }

    // Construct message
    Message msg;
    msg.setType(MessageType::REGISTER);
    msg.setName(name);
    msg.setServerIdentifier(host_name);
    msg.setPort(host_port);
    msg.setArgTypes(argTypes);

    // Send message to binder
    try {
        msg.sendMessage(binder_socket);
        msg.recvHeader(binder_socket);
        msg.recvMessage(binder_socket);
    } catch(Message::SendError) {
        return ERROR_MESSAGE_SEND;
    } catch(Message::RecvError) {
        return ERROR_MESSAGE_RECV;
    }

    // Add function to local datatabse
    string key = getSignature(name, argTypes); 
    functions[key] = f;

    return msg.getReasonCode();
}

static void executeAsync(int client) {
    
    auto& msg = requests[client];
    string key = getSignature(msg.getName(), msg.getArgTypes());

    try {
        if (functions[key] == nullptr) {
            msg.setType(MessageType::EXECUTE_FAILURE);
            msg.setReasonCode(ERROR_MISSING_FUNCTION);
            msg.sendMessage(client);
        }

        int status = (functions[key])(msg.getArgTypes(), msg.getArgs());
        if (status < 0) {
            msg.setType(MessageType::EXECUTE_FAILURE);
            msg.setReasonCode(ERROR_FUNCTION_CALL);
            msg.sendMessage(client);
        }

        msg.setType(MessageType::EXECUTE_SUCCESS);
        msg.sendMessage(client);
    } catch(Message::SendError) {
    }
}

void cleanup(int socketfd, fd_set& master_set) {

    close(socketfd);
    requests.erase(socketfd);
    FD_CLR(socketfd, &master_set);
}

int rpcExecute() {
    
    if (client_socket == SOCK_INVALID) {
        return ERROR_SERVER_NOT_RUNNING;
    }

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(binder_socket, &master_set);
    FD_SET(client_socket, &master_set);
    int max_socket = max(client_socket, binder_socket);
    int ret = 0;

    for (;;) {

        bool terminate = false;
        read_set = master_set;
        if (select(max_socket + 1, &read_set, nullptr, nullptr, nullptr) < 0) {
            ret = ERROR_SOCKET_SELECT;
            break;
        }

        for (int i = 0; i <= max_socket; ++i) {
            if (!FD_ISSET(i, &read_set)) {
                continue;
            }    
            
            if (i == client_socket) {
                // Accept the incoming connection
                int client = accept(client_socket, nullptr, nullptr);
                if (client != SOCK_INVALID) {
                    FD_SET(client, &master_set);
                    max_socket = max(max_socket, client); 
                }
            } else {                
                auto& msg = requests[i];
                try {
                    if (msg.getType() == MessageType::NONE) {
                        // Peek at the header and continue
                        // if we don't have all 8 bytes
                        if (msg.peek(i) < msg.HEADER_SIZE) {
                            continue;
                        }
    
                        msg.recvHeader(i);
                        if (msg.getType() == MessageType::TERMINATE) {
                            // Autheticate termination request
                            if (i != binder_socket) {
                                cleanup(i, master_set);
                                continue;
                            }

                            cout << "YALL CALLED TERMINATE" << endl;
                            terminate = true;
                            break;
                        } 
                    } else {
                        // Peek at the body and continue if
                        // we don't have the full length
                        if (msg.peek(i) < msg.getLength()) {
                            continue;
                        }

                        msg.recvMessage(i);
                        FD_CLR(i, &master_set);
                        thread th(executeAsync, i);
                        calls.push_back(move(th));
                    }
                } catch(...) {
                    cleanup(i, master_set);
                    if (i == binder_socket) {
                        terminate = true;
                        ret = ERROR_LOST_CONNECTION_BINDER;
                        break;    
                    }
                }
            }
        }

        if (terminate) {
            break;    
        }
    }
  
    // Wait for all threads to finish
    for (auto& th : calls) {
        th.join();    
    }
 
    // Close all connections
    for (int i = 0; i <= max_socket; ++i) {
        if (FD_ISSET(i, &master_set)) {
            cleanup(i, master_set);
        }
    }

    return ret;
}
