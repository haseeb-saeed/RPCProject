Group Members:
Haseeb Saeed (h7saeed)
Sarah Heffer (scheffer)

Dependencies:
-lpthread

How to compile and run the system:
1.  make   command to compile the code to get librpc.a and binder executable in the linux.student.cs environment
2.  g++ -L. client.o -lrpc -o client   for compiling the client
3.  g++ -L. server_functions.o server_function_skels.o server.o -lrpc -lpthread -o server   for compiling the server
4.  ./binder
5.  Manually set the BINDER_ADDRESS and BINDER_PORT environment variables on the client and server machines. (Use setenv if using C shell)
6.  ./server  and ./client  to run the server(s) and client(s).

Note: Step 3 differs slightly from step 3 in the assignment specification, due to including the -lpthread dependency.

Note: We are making the assumption that the *.o object files exist for the client and server, if this is not the case, then include the following steps before running make command:
g++ -c server_functions.c -o server_functions.o
g++ -c server_function_skels.c -o server_function_skels.o
g++ -c server.c -o server.o
g++ -c client.c -o client.o

