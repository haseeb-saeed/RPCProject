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
        auto& functions = it->functions;
        if (functions.find(signature) == functions.end()) {
            cout << "Adding signature to existing slot" << endl;
            functions.insert(signature);
            msg.setReasonCode(0);
        } else {
            cout << "Signature already exits" << endl;
            msg.setReasonCode(WARNING_DUPLICATE_FUNCTION);
        }
    } else {
        cout << "Adding signature to new slot" << endl;
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

    for (i = 0; i < size; ++i) {
        const auto& entry = database[database_index++];
        database_index %= size;

        if (entry.functions.find(signature) != entry.functions.end()) {
            msg.setType(MessageType::LOC_SUCCESS);
            msg.setServerIdentifier(entry.name.c_str());
            msg.setPort(entry.port);
            break;
        }
    }

    if (i == size) {
        msg.setType(MessageType::LOC_FAILURE);
        msg.setReasonCode(ERROR_MISSING_FUNCTION);
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

    // Remove all entries associated with the server
    if (servers.find(socketfd) != servers.end()) {
        const auto& location = servers[socketfd];
        const Entry entry(location);
        auto it = find(database.begin(), database.end(), entry);

        if (it != database.end()) {
            database.erase(it);
            cout << "I see a putty cat" << endl;
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
    cout << "BINDER ADDRESS " << host->h_name << endl;
    cout << "BINDER PORT " << port << endl;

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(socketfd, &master_set);
    int maxfd = socketfd;

    for(;;) {

        bool terminate = false;
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
                try {
                    if (msg.getType() == MessageType::NONE) {
                        // Peek to see if the heaer has arrived
                        if (msg.peek(i) < msg.HEADER_SIZE) {
                            continue;    
                        }
                        msg.recvHeader(i);
                    } else {
                        // Peek to see if the body has arrived
                        if (msg.peek(i) < msg.getLength()) {
                            continue;    
                        }

                        msg.recvMessage(i);
                        
                        switch (msg.getType()) {
                            case MessageType::REGISTER:
                                cout << "REGISTER" << endl;
                                registerFunction(i);
                                requests.erase(i);
                                break;
                            case MessageType::LOC_REQUEST:
                                cout << "LOC_REQUEST" << endl;
                                getLocation(i);
                                cleanup(i, master_set);
                                break;
                            case MessageType::TERMINATE:
                            default:
                                cout << "TERMINATE" << endl;
                                terminate = true;
                                break;
                        }

                        if (terminate) {
                            break;
                        }
                    }
                } catch(...) {
                    cout << "Caught some random-ass exception" << endl;
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
