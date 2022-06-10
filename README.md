# multiple_clients_server
My attempt to create a server implementation using C

## How to use
```
make && ./app
```
### Connect to the server as a client using netcat
```
nc 127.0.0.1 8765
```
Once connected, the server will wait:
- room number (int)
- username (char set)
### Disconnect from the server
Send '/exit'
