#include "args.h"
#include "binder.h"
#include "message.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <unordered_map>
#include <stdlib.h>
#include <utility>
#include <vector>

using namespace args;
using namespace std;
using namespace message;

struct Entry {
    int socketfd;
    string location;
    int port;
};

unordered_map<string, pair<vector<Entry>, int>> database;
unordered_map<int, Message> requests;

void registerFunction(int socketfd) {
    // TODO: Stuff
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

    // TODO: Reason code
    msg.setType(MessageType::LOC_FAILURE);
    msg.setReasonCode(-1);
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
                    int bytes = msg.peekHeader(i);
                    if (bytes <= 0) {
                        // TODO: Stuff    
                    } else if (bytes < msg.HEADER_SIZE) {
                        continue;    
                    } else if (msg.recvHeader(i) < 0) {
                        // TODO: Stuff    
                    }
                } else {
                    // Peek to see if the body has arrived
                    int bytes = msg.peekMessage(i);
                    if (bytes <= 0) {
                        // TODO: Stuff    
                    } else if (bytes < msg.getLength()) {
                        continue;    
                    } else if (msg.recvMessage(i) < 0) {
                        // TODO: Stuff    
                    }
                    
                    switch (msg.getType()) {
                        case MessageType::REGISTER:
                            registerFunction(i);
                            break;
                        case MessageType::LOC_REQUEST:
                            getLocation(i);
                            break;
                        default:
                            // TODO: Should we handle wrong types?
                            break;
                    }

                    // TODO: Is there any other cleanup required?
                    requests.erase(i);
                    FD_CLR(i, &master_set);
                }
            }
        }
    }

    return 0;
}
