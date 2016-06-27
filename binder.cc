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

struct Entry {
    int socketfd;
    string location;
    int port;

    Entry(const int& socketfd, const char* location, const int& port):
        socketfd(socketfd), location(location), port(port) {
    }

    friend bool operator== (const Entry& e1, const Entry &e2);
};

bool operator== (const Entry& e1, const Entry& e2) {
    return e1.location == e2.location && e1.port == e2.port;
}

unordered_map<string, pair<vector<Entry>, int>> database;
unordered_map<int, Message> requests;
unordered_set<int> servers;

void registerFunction(int socketfd) {

    servers.insert(socketfd);
    auto& msg = requests[socketfd];
    const string key = getSignature(msg.getName(), msg.getArgTypes());
    auto& list = database[key].first;

    Entry entry(socketfd, msg.getServerIdentifier(), msg.getPort());
    if (find(list.begin(), list.end(), entry) == list.end()) {
        list.push_back(entry);
        msg.setReasonCode(0);
    } else {
        msg.setReasonCode(WARNING_DUPLICATE_FUNCTION);
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

        const auto& location_info = entry.first[index];
        // TODO: Ping the server - if down, ignore
        
        msg.setType(MessageType::LOC_SUCCESS);
        msg.setServerIdentifier(location_info.location.c_str());
        msg.setPort(location_info.port);
        msg.sendMessage(socketfd);
        return;
    }

    msg.setType(MessageType::LOC_FAILURE);
    msg.setReasonCode(ERROR_MISSING_FUNCTION);
    msg.sendMessage(socketfd);
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
                        // TODO: Stuff    
                    } else if (bytes < msg.HEADER_SIZE) {
                        continue;    
                    } else if (msg.recvHeader(i) < 0) {
                        // TODO: Stuff    
                    }
                } else {
                    // Peek to see if the body has arrived
                    int bytes = msg.peek(i);
                    if (bytes <= 0) {
                        // TODO: Stuff    
                    } else if (bytes < msg.getLength()) {
                        continue;    
                    } else if (msg.recvMessage(i) < 0) {
                        // TODO: Stuff    
                    }
                    
                    bool terminate = false;
                    switch (msg.getType()) {
                        case MessageType::REGISTER:
                            registerFunction(i);
                            break;
                        case MessageType::LOC_REQUEST:
                            getLocation(i);
                            FD_CLR(i, &master_set);
                            close(i);
                            break;
                        case MessageType::TERMINATE:
                        default:
                            terminate = true;
                            break;
                    }

                    // TODO: Is there any other cleanup required?
                    requests.erase(i);

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
        msg.sendMessage(server);
    }

    // Close all connections
    for (int i = 0; i <= maxfd; ++i) {
        if (FD_ISSET(i, &master_set)) {
            close(i);
        }
    }

    // TODO: Deallocate any memory?
    return 0;
}
