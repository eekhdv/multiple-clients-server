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
Once connected, the server will ask you for:
- room number (int)
- (if needed) limit of users in the room
- username (char set)
> You can create multiple connections using netcat
### Disconnect from the server
Send '/exit'
