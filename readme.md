*Designed by Marius-Tudor Zaharia, 323CA, April 2024*

# Server Client App

---

## 1. What is Server Client App?
* Server Client App is a basic implementation of a client-server system based
on the socket API from Linux.
* It focuses on providing an I/O multiplexing mechanism, that allows the
connection of an unlimited number of clients to a singular server. Also, the
emphasis is put on the ability of the system (the server, but also the client)
to handle a large number of messages in a short time span and for the events
that occur for various subscribers to be treated immediately.
* For that, the poll mechanism is used.

---

## 2. File distribution
* The server side of the system is described as a class, placed in `Server.h`,
while the class methods are implemented in `Server.cpp`.
* The main code is written in `server_main.cpp`, thus achieving clarity and nice
modularization.
* The Subscriber side is implemented by the exact same pattern as the Server.
* The protocol over TCP that is used for sending/receiving messages in an
efficient way is described in `protocols.h` and `protocols.cpp`.

---

## 3. Running
* The system can be compiled by using the `make` command in the root directory.
* To run the server, the command `./server <IP> <PORT>` should be used.
* The command `./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>` should be
used to run the subscriber.
* Both the server and the subscriber can be stopped with the `exit` command.

---

## 4. Design choices

