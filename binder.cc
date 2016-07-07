#include "args.h"
#include "codes.h"
#include "message.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
#include "rpc.h"
#include <sys/socket.h>
#include <netdb.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <stdlib.h>
#include <utility>
#include <vector>

using namespace args;
using namespace codes;
using namespace std;
using namespace message;

struct Entry {
    string name;
    int port;
    unordered_set<string> functions;
    
    Entry(const pair<string, int>& location):
        name(location.first), port(location.second) {
    }

    friend bool operator== (const Entry&, const Entry&);
};

bool operator== (const Entry& e1, const Entry& e2) {
    return e1.name == e2.name && e1.port == e2.port;
}

vector<Entry> database;
int database_index = 0;
unordered_map<int, pair<string, int>> servers;
unordered_map<int, Message> requests;

void registerFunction(int socketfd) {
    auto& msg = requests[socketfd];
    const string signature = getSignature(msg.getName(), msg.getArgTypes());
    auto location = make_pair(msg.getServerIdentifier(), msg.getPort());
    Entry entry(location);
    servers[socketfd] = location; 
    
    auto it = find(database.begin(), database.end(), entry);
    if (it != database.end()) {
        // Check existing slot for server
        // If signature doesn't exist, add it
        auto& functions = it->functions;
        if (functions.find(signature) == functions.end()) {
            functions.insert(signature);
            msg.setReasonCode(0);
        } else {
            msg.setReasonCode(WARNING_DUPLICATE_FUNCTION);
        }
    } else {
        // Create a new slot for the server and add signature
        entry.functions.insert(signature);
        database.push_back(entry);
        msg.setReasonCode(0);
    }

    try {
        msg.setType(MessageType::REGISTER_SUCCESS);
        msg.sendMessage(socketfd);
    } catch(...) {
    }
}

void getLocation(int socketfd) {
    auto& msg = requests[socketfd];
    const string signature = getSignature(msg.getName(), msg.getArgTypes());
    const int size = database.size();
    int i;

    // Round-robin scheduling
    // Each server is checked at most once
    for (i = 0; i < size; ++i) {
        const auto& entry = database[database_index++];
        database_index %= size;

        // If we have a match, send the info the the client
        if (entry.functions.find(signature) != entry.functions.end()) {
            msg.setType(MessageType::LOC_SUCCESS);
            msg.setServerIdentifier(entry.name.c_str());
            msg.setPort(entry.port);
            break;
        }
    }

    // No servers were found
    if (i == size) {
        msg.setType(MessageType::LOC_FAILURE);
        msg.setReasonCode(ERROR_MISSING_FUNCTION);
    }

    try {
        msg.sendMessage(socketfd);
    } catch(...) {
    }
}

void getAllLocations(int socketfd) {
    auto& msg = requests[socketfd];
    const string signature = getSignature(msg.getName(), msg.getArgTypes());
    vector<pair<string, int>> locations;

    // Get the location of every registered server
    for (auto& entry : database) {
        if (entry.functions.find(signature) != entry.functions.end()) {
            locations.push_back(make_pair(entry.name, entry.port));
        }
    }

    int num_args = locations.size() * 2;
    if (num_args == 0) {
        msg.setType(MessageType::LOC_FAILURE);
        msg.setReasonCode(ERROR_MISSING_FUNCTION);
    } else {
        // Send all location backs to the client
        unique_ptr<int[]> arg_types(new int[num_args + 1]);
        unique_ptr<void*[]> args(new void*[num_args]);

        // Locations are sent as args in pairs
        // First arg is the identifier, second arg is the port
        for (int i = 0; i < num_args; i += 2) {
            const auto& str = locations[i].first;
            arg_types[i] = (ARG_CHAR << 16) | (str.length() + 1);
            arg_types[i + 1] = (ARG_INT << 16);
            args[i] = (void*)str.c_str();
            args[i + 1] = &(locations[i].second);
        }
        arg_types[num_args] = 0;

        msg.setType(MessageType::LOC_CACHE_SUCCESS);
        msg.setArgTypes(arg_types.get());
        msg.setArgs(args.get());
    }

    try {
        msg.sendMessage(socketfd);
    } catch(...) {
    }
}

void cleanup(int socketfd, fd_set& master_set) {
    close(socketfd);    
    requests.erase(socketfd);
    FD_CLR(socketfd, &master_set);

    // Remove the entry for the database if it exists
    if (servers.find(socketfd) != servers.end()) {
        const auto& location = servers[socketfd];
        const Entry entry(location);
        auto it = find(database.begin(), database.end(), entry);

        if (it != database.end()) {
            database.erase(it);
            if (database.size() > 0) {
                database_index %= database.size();
            } else {
                database_index = 0;    
            }
        }

        servers.erase(socketfd);
    }
}

int main() {
    addrinfo host_info, *host_info_list;
    memset(&host_info, 0, sizeof host_info);
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, "0", &host_info, &host_info_list); 
    if (status < 0) {
        cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
        return EXIT_FAILURE;
   }

    // Create socket
    int socketfd;
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (socketfd == -1) {
        cerr << "socket error" << endl;
        return EXIT_FAILURE;
    }

    // Bind
    status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    freeaddrinfo(host_info_list);
    if (status == -1) {
        cerr << "bind error" << endl;
        close(socketfd);
        return EXIT_FAILURE;
    }

    // Listen
    if (listen(socketfd, 5) < 0) {
        cerr << "listen error" << endl;
        close(socketfd);
        return EXIT_FAILURE;
    }

    // Get hostname and port
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(socketfd, (sockaddr*)&addr, &len) < 0) {
        cerr << "getsockname error" << endl;
        close(socketfd);
        return EXIT_FAILURE;
    }

    char host_name[64];
    if (gethostname(host_name, sizeof(host_name)) < 0) {
        cerr << "gethostname error" << endl;
        close(socketfd);
        return EXIT_FAILURE;
    }

    auto host = gethostbyname(host_name);
    if (host == nullptr) {
        cerr << "gethostbyname error" << endl;
        close(socketfd);
        return EXIT_FAILURE;
    }

    int port = ntohs(addr.sin_port);
    cout << "BINDER_ADDRESS " << host->h_name << endl;
    cout << "BINDER_PORT " << port << endl;

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(socketfd, &master_set);
    int maxfd = socketfd;

    for(;;) {
        bool terminate = false;
        read_set = master_set;
        select(maxfd + 1, &read_set, nullptr, nullptr, nullptr);

        // Go through the sockets with data on them
        for (int i = 0; i <= maxfd; ++i) {
            if (!FD_ISSET(i, &read_set)) {
                continue;    
            }
            
            if (i == socketfd) {
                // Accept an incoming connection
                int client = accept(socketfd, nullptr, nullptr);
                if (client != -1) {
                    FD_SET(client, &master_set);
                    maxfd = max(maxfd, client);
                }
            } else {
                // Receive the request
                auto& msg = requests[i];
                try {
                    msg.recvNonBlock(i);
                    if (!msg.eom()) {
                        continue;
                    }

                    // Handle the request
                    // If the request is not a server request,
                    // close the connection after servicing
                    switch (msg.getType()) {
                        case MessageType::REGISTER:
                            registerFunction(i);
                            requests.erase(i);
                            break;
                        case MessageType::LOC_REQUEST:
                            getLocation(i);
                            cleanup(i, master_set);
                            break;
                        case MessageType::LOC_CACHE:
                            getAllLocations(i);
                            cleanup(i, master_set);
                            break;
                        case MessageType::TERMINATE:
                            terminate = true;
                            break;
                        default:
                            break;
                    }

                    if (terminate) {
                        break;
                    }

                } catch(...) {
                    // Usually this happens because somebody closed their
                    // connection so recv threw
                    cleanup(i, master_set);
                }
            }
        }

        if (terminate) {
            break;    
        }
    }

    Message msg;
    msg.setType(MessageType::TERMINATE);

    // Tell all servers to terminate
    for (const auto& server : servers) {
        msg.sendMessage(server.first);
    }

    // Close all connections
    for (int i = 0; i <= maxfd; ++i) {
        if (FD_ISSET(i, &master_set)) {
            cleanup(i, master_set);
        }
    }

    return 0;
}
