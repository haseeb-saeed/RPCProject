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
using namespace std;
using namespace message;

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

    for(;;) {
        // TODO: Stuff
    }

    return 0;
}
