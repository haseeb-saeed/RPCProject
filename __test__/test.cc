#include <atomic>
#include <algorithm>
#include <cassert>
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
#include "rpc.h"

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

int* createArgTypes() {
    
    int* arg_types = new int[7];

    arg_types[0] = (1 << ARG_OUTPUT) | (ARG_CHAR << 16);
    arg_types[1] = (1 << ARG_INPUT) | (ARG_SHORT << 16) | 5;
    arg_types[2] = (1 << ARG_INPUT) | (ARG_INT << 16);
    arg_types[3] = (1 << ARG_OUTPUT) | (ARG_LONG << 16);
    arg_types[4] = (1 << ARG_INPUT) | (ARG_FLOAT << 16) | 1;
    arg_types[5] = (1 << ARG_OUTPUT) | (ARG_DOUBLE << 16) | 7;
    arg_types[6] = 0;
    
    return arg_types;
}

void** createArgs() {

    char* arg0 = new char('A');
    short* arg1 = new short[5]{1, 2, 3, 4, 5};
    int* arg2 = new int(7);
    long* arg3 = new long(55);
    float* arg4 = new float[1]{0.05f};
    double* arg5 = new double[7]{1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0};

    void** args = new void*[6];
    args[0] = arg0;
    args[1] = arg1;
    args[2] = arg2;
    args[3] = arg3;
    args[4] = arg4;
    args[5] = arg5;

    return args;
}

void printArgs(int* arg_types, void** args) {

    for (int i = 0; i < numArgs(arg_types); ++i) {
        int num = max(arrayLen(arg_types[i]), 1);
        cout << "num " << num << endl;

        if (isChar(arg_types[i])) {
            cout << "CHAR" << endl;
            char* arg = (char*)(args[i]);
            for (int j = 0; j < num; ++j) {
                cout << arg[j] << " ";
            }
            cout << endl;
        } else if (isShort(arg_types[i])) {
            cout << "SHORT" << endl;
            short* arg = (short*)(args[i]);
            for (int j = 0; j < num; ++j) {
                cout << arg[j] << " ";
            }
            cout << endl;
        } else if (isInt(arg_types[i])) {
            cout << "INT" << endl;
            int* arg = (int*)(args[i]);
            for (int j = 0; j < num; ++j) {
                cout << arg[j] << " ";
            }
            cout << endl;
        } else if (isLong(arg_types[i])) {
            cout << "LONG" << endl;
            long* arg = (long*)(args[i]);
            for (int j = 0; j < num; ++j) {
                cout << arg[j] << " ";
            }
            cout << endl;
        } else if (isFloat(arg_types[i])) {
            cout << "FLOAT" << endl;
            float* arg = (float*)(args[i]);
            for (int j = 0; j < num; ++j) {
                cout << arg[j] << " ";
            }
            cout << endl;
        } else if (isDouble(arg_types[i])) {
            cout << "DOUBLE" << endl;
            double* arg = (double*)(args[i]);
            for (int j = 0; j < num; ++j) {
                cout << arg[j] << " ";
            }
            cout << endl;
        }
    }
}

void cleanupArgs(int* arg_types, void** args) {

    int num_args = numArgs(arg_types);
    for (int i = 0; i < num_args; ++i) {
        cout << arrayLen(arg_types[i]) << endl;;
        if (arrayLen(arg_types[i]) == 0) {
            delete (char*)(args[i]);
        } else {
            delete [] (char*)(args[i]);
        }
    }

    delete [] args;
}

void cleanupArgTypes(int* arg_types) {
    delete [] arg_types;
}

void send(Message& msg, int socketfd) {

    cout << "Sending message of length " << msg.getLength() << endl;
    msg.sendMessage(socketfd);
    cout << "Sending complete" << endl;
}

void receive(Message& msg, int socketfd, MessageType type) {
   
    while (!msg.eom()) {
        cout << "LOL" << endl;
        msg.recvNonBlock(socketfd);
    }
    assert (msg.getType() == type);

    cout << "Receiving message of length " << msg.getLength() << endl;
    cout << "Receiving complete" << endl;
}

void testRegisterServer(int socketfd) {

    // Send request
    auto arg_types = createArgTypes();

    Message msg;
    msg.setType(MessageType::REGISTER);
    msg.setServerIdentifier("Biscuit");
    msg.setPort(73);
    msg.setName("name");
    msg.setArgTypes(arg_types);

    cout << "numArgs " << numArgs(arg_types) << " " << msg.numArgs() << endl;
    for (int i = 0; i < msg.numArgs(); ++i) {
        cout << arg_types[i] << " ";
    }
    cout << endl;

    send(msg, socketfd);
    cleanupArgTypes(arg_types);

    // Get reply
    receive(msg, socketfd, REGISTER_SUCCESS);
    cout << msg.getReasonCode() << endl;
}

void testRegisterClient(int socketfd) {

    // Get request
    Message msg;
    receive(msg, socketfd, MessageType::REGISTER);
    cout << msg.getServerIdentifier() << endl;
    cout << msg.getPort() << endl;
    cout << msg.getName() << endl;

    auto arg_types = msg.getArgTypes();
    for (int i = 0; i < msg.numArgs(); ++i) {
        cout << arg_types[i] << " ";
    }
    cout << endl;

    // Send reply
    msg.setType(MessageType::REGISTER_SUCCESS);
    msg.setReasonCode(5);
    send(msg, socketfd);
}

void testExecuteServer(int socketfd) {

    auto arg_types = createArgTypes();
    auto args = createArgs();

    Message msg;
    msg.setType(MessageType::EXECUTE);
    msg.setName("foo");
    msg.setArgTypes(arg_types);
    msg.setArgs(args);
    send(msg, socketfd);

    cleanupArgs(arg_types, args);
    cleanupArgTypes(arg_types);

    receive(msg, socketfd, MessageType::EXECUTE_FAILURE);
    cout << msg.getReasonCode() << endl;
}

void testExecuteClient(int socketfd) {

    Message msg;
    receive(msg, socketfd, MessageType::EXECUTE);

    cout << msg.getName() << endl;
    printArgs(msg.getArgTypes(), msg.getArgs());

    msg.setType(MessageType::EXECUTE_FAILURE);
    msg.setReasonCode(-1);
    send(msg, socketfd);
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

    //testRegisterServer(client);
    testExecuteServer(client);
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

    //testRegisterClient(socketfd);
    testExecuteClient(socketfd);
}

int main() {

    thread server(runServer);
    thread client(runClient);

    server.join();
    client.join();
}
