## Overview

This repository contains an implementation of a multithreaded TCP server and a TCP client for Linux written in C++11.

This particular version uses the server to calculate the arithmetic mean of numbers received from clients.


## Workflow

1. The server starts listening to a specific port.
2. Several clients connect and start sending nonzero `int32_t` values to the server.
3. When the server receives 0 from a client, it sends back the average of this client's values and closes the connection.
4. When the last of the clients disconnects, the server outputs the average of all clients' values to `result.txt` and stops.


## Building

`make` and `g++` are required.

Build the server and the client:
```
make
```

Run a simple test:
```
make test
```

Delete the executables and the created text files:
```
make clean
```


## Running

Start the server:
```
./server <port>
```

Start a client and send a sequence of numbers:
```
./client <host> <port> [<number>...]
```
