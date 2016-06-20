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
    int status;
    addrinfo host_info, *host_info_list;

    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    host_info.ai_flags = AI_PASSIVE;
    status = getaddrinfo(NULL, "0", &host_info, &host_info_list);
    if (status != 0) cerr << "getaddrinfo errori: " << gai_strerror(status) << endl;

    // Create socket
    int socketfd;
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (socketfd == -1) cerr << "socket error" << endl;

    // Bind
    int yes = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = ::bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) cerr << "bind error" << endl;
    freeaddrinfo(host_info_list);

    // Listen
    status = listen(socketfd, 10);
    if (status == -1) cerr << "listen error" << endl;

    // Accept
    int new_sd;
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    new_sd = accept(socketfd, (struct sockaddr *)&client_addr, &addr_size);
    if (new_sd == -1) cerr << "listen error" << endl;

    MessageInfo msgInfo;
    int msg = getMessage(new_sd, msgInfo);

    return 0;
}
