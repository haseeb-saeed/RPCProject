#include "args.h"
#include "codes.h"
#include "message.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
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

typedef pair<string, int> Location;
unordered_map<string, pair<vector<Location>, int>> database;
unordered_map<string, unordered_set<string>> registered_functions;
unordered_map<int, Location> servers;
unordered_map<int, Message> requests;

string getHash(const Location& location) {
    
    return location.first + to_string(location.second);
}

void registerFunction(int socketfd) {

    auto& msg = requests[socketfd];
    Location location(msg.getServerIdentifier(), msg.getPort());
    servers[socketfd] = location;

    const string key = getSignature(msg.getName(), msg.getArgTypes());
    auto& functions = registered_functions[getHash(location)];
    if (functions.find(key) != functions.end()) {
        msg.setReasonCode(WARNING_DUPLICATE_FUNCTION);
    } else {
        auto& list = database[key].first;
        list.push_back(location);
        functions.insert(key);
        msg.setReasonCode(0);
    }

    msg.setType(MessageType::REGISTER_SUCCESS);
    msg.sendMessage(socketfd);
}

void getLocation(int socketfd) {

    auto& msg = requests[socketfd];
    const string key = getSignature(msg.getName(), msg.getArgTypes());
    auto& entry = database[key];
    const int size = entry.first.size();
    
    for (int i = 0; i < size; ++i) {
        int index = entry.second;
        entry.second = (entry.second + 1) % size;

        const auto& location = entry.first[index];
        msg.setType(MessageType::LOC_SUCCESS);
        msg.setServerIdentifier(location.first.c_str());
        msg.setPort(location.second);
        msg.sendMessage(socketfd);
        return;
    }

    msg.setType(MessageType::LOC_FAILURE);
    msg.setReasonCode(ERROR_MISSING_FUNCTION);
    msg.sendMessage(socketfd);
}

void cleanup(int socketfd, fd_set& master_set) {

    close(socketfd);    
    requests.erase(socketfd);
    FD_CLR(socketfd, &master_set);

    // Remove all entries associated with the server
    if (servers.find(socketfd) != servers.end()) {
        const auto& location = servers[socketfd];
        const string hash = getHash(location);
        for (const auto& signature : registered_functions[hash]) {
            auto& list = database[signature].first;
            auto& index = database[signature].second;
            auto it = find(list.begin(), list.end(), location);
            if (it != list.end()) {
                list.erase(it);
                index = index % list.size();
            }
        }
        registered_functions.erase(hash);
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
    cout << "BINDER ADDRESS " << host->h_name << endl;
    cout << "BINDER PORT " << port << endl;

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(socketfd, &master_set);
    int maxfd = socketfd;

    for(;;) {
        read_set = master_set;
        select(maxfd + 1, &read_set, nullptr, nullptr, nullptr);

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
                auto& msg = requests[i];
                if (msg.getType() == MessageType::NONE) {
                    // Peek to see if the heaer has arrived
                    int bytes = msg.peek(i);
                    if (bytes <= 0) {
                        cleanup(i, master_set);
                    } else if (bytes < msg.HEADER_SIZE) {
                        continue;    
                    } else if (msg.recvHeader(i) < 0) {
                        cleanup(i, master_set);
                    }
                } else {
                    // Peek to see if the body has arrived
                    int bytes = msg.peek(i);
                    if (bytes <= 0) {
                        cleanup(i, master_set);
                    } else if (bytes < msg.getLength()) {
                        continue;    
                    } else if (msg.recvMessage(i) < 0) {
                        cleanup(i, master_set);
                    }
                    
                    bool terminate = false;
                    switch (msg.getType()) {
                        case MessageType::REGISTER:
                            registerFunction(i);
                            requests.erase(i);
                            break;
                        case MessageType::LOC_REQUEST:
                            getLocation(i);
                            cleanup(i, master_set);
                            break;
                        case MessageType::TERMINATE:
                        default:
                            terminate = true;
                            break;
                    }

                    if (terminate) {
                        break;
                    }
                }
            }
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
