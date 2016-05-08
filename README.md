# libuv-socket-example
Simple client server using libuv

Build instructions:

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

Client:
gcc -o client client.c -Wall -luv

Server:
gcc -o server server.c -Wall -luv
