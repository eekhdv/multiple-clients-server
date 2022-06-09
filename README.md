# multiple_clients_server
My attempt to create a server implementation using C

## How to use
```
gcc src/server.c -lpthread -o server
./client
```
### Connect to the server as a client using netcat
```
nc 127.0.0.1 8765
```
