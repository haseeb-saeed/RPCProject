#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include "args.h"
#include "codes.h"
#include "message.h"

using namespace args;
using namespace codes;
using namespace message;
using namespace std;

atomic_int port(0);
atomic_bool done(false);

sockaddr_in getAddr(const char* ip_addr) {
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof (addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(ip_addr, &addr.sin_addr);

    return addr;
}

void runServer() {

    int socketfd = socket(PF_INET, SOCK_STREAM, 0);
    auto addr = getAddr("127.0.0.1");    
    bind(socketfd, (sockaddr*)&addr, sizeof(addr));
    if (listen(socketfd, 5) < 0) {
        cout << "dafuq" << endl;
    };

    socklen_t len = sizeof(addr);
    getsockname(socketfd, (sockaddr*)&addr, &len);
    port = ntohs(addr.sin_port);
    cout << "port " << port << endl;

    done = true;

    int client = accept(socketfd, nullptr, nullptr);
    cout << "Accept " << client << endl;

    int temp = 100;
    send(client, &temp, sizeof(temp), 0);
}

void runClient() {

    while (!done) {
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    cout << "port " << port << endl;
    
    int socketfd = socket(PF_INET, SOCK_STREAM, 0);
    auto addr = getAddr("127.0.0.1");
    int status = connect(socketfd, (sockaddr*)&addr, sizeof(addr));

    cout << "Connect " << status << " " << errno << endl;

    int temp;
    recv(socketfd, &temp, sizeof(temp), MSG_WAITALL);
    cout << temp << endl;
}

int main() {

    thread server(runServer);
    thread client(runClient);

    server.join();
    client.join();
}
