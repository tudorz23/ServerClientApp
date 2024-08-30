*Designed by Marius-Tudor Zaharia, 323CA, April 2024*

# Server Client App

---

## Table of contents
1. [What is Server Client App?](#what-is-server-client-app)
2. [File distribution](#file-distribution)
3. [Running](#running)
4. [General details and design choices](#general-details-and-design-choices)
5. [The Protocol over TCP](#the-protocol-over-tcp)
6. [Program flow and logic](#program-flow-and-logic)
7. [Punctual implementation details](#punctual-implementation-details)
8. [Wildcard handling](#wildcard-handling)
9. [Final thoughts](#final-thoughts)
10. [Bibliography](#bibliography)

---

## What is Server Client App?
* Server Client App is a basic implementation of a client-server system based
on the socket API from Linux.
* It focuses on providing an I/O multiplexing mechanism, that allows the
connection of an unlimited number of clients to a singular server. Also, the
emphasis is put on the ability of the system (the server, but also the client)
to handle a large number of messages in a short time span and for the events
that occur for various subscribers to be treated immediately.
* For that, the poll mechanism is used.

---

## File distribution
* The server side of the system is described as a class, placed in `Server.h`,
while the class methods are implemented in `Server.cpp`.
* The main code is written in `server_main.cpp`, thus achieving clarity and
nice modularization.
* The Subscriber side is implemented by the exact same pattern as the Server.
* The protocol over TCP that is used for sending/receiving messages in an
efficient way is described in `protocols.h` and `protocols.cpp`.
* A UDP client implementation (by the PCOM team) can be found in the
`udp_client.py` file.

---

## Running
* The system can be compiled by using the `make` command in the root directory.
* To run the server: `./server <PORT>` (i.e. `./server 12345`).
* To run the TCP subscriber: `./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>`
(i.e. `./subscriber name 127.0.0.1 12345`).
* To subscribe to a topic: `subscribe <topic>`.
* The topics can be found in the JSON files from `pcom_hw2_udp_client/`.
* To run the UDP subscriber: instructions in `pcom_hw2_udp_client/README.md`.
* Both the server and the subscriber can be stopped with the `exit` command.

---

## General details and design choices
* I chose to use classes for the server and for the subscriber, thus being able
to access their class attributes from anywhere inside the class. Without
classes, global variables should have been used, or the variables should have
been initialized in the `main()` functions, and then passed as parameters to
all the functions that needed them.
* For managing the connected/disconnected clients inside the server class, I
used the `client` structure.

---

## The Protocol over TCP
* Probably the most important part of the project was the creation of an
efficient and robust protocol for sending messages over the TCP.
* For that, I used the `tcp_message` structure, which encapsulates the
`command` type, the `len` of the payload and the `payload` itself.
* The `command` field takes a value from 0 to 9 and marks the role of the
message structure (described as `#define` directives in `protocols.h`).
* The `len` attribute is necessary for knowing how long the memory zone where
the payload would be stored should be. For the protocol to be efficient, the
`payload` cannot be statically allocated, because then the same number of bytes
would be sent over the network for all messages, even for very small ones, thus
clotting the network.
* At first, I set the `command` to be an `uint8_t`, but somehow it did not work
as expected, so I set it to `uint16_t` and it was fine. At that time, my `send`
and `receive` functions firstly managed the `command` and `len` fields
simultaneously, by starting from the beginning of the `tcp_message` structure.
* Later, I realized that there was no guarantee that the compiler would not
insert padding into my structure and break that logic. That was probably what
was actually happening when `command` was an `uint8_t`, the `len` field was not
completely filled up, some parts were written to the padding, and when I
switched to `uint16_t`, the memory was already aligned and no more padding
occurred.
* Thus, `recv_efficient()` and `send_efficient()` now manage each struct
attribute separately, by accessing them with `->` operator, so it is guaranteed
that padding does not interfere with the logic anymore.
* It is worth noting that `recv_efficient()` dynamically allocates memory for
the message payload, so exactly `len` bytes are received from the network, just
as exactly `len` bytes are sent in `send_efficient()` (for the payload,
because `command` and `len` are obviously also sent).
* I insisted on the padding problem because, for me, it was the most
interesting part of the homework. I had never previously encountered such a
problem (just heard of it theoretically), so it really made me aware of it for
future projects.

---

## Program flow and logic
* The `server` program starts by initializing the basic sockets and `pollfd`
structs that would be used (stdin for messages from user, UDP for messages
from UDP clients and TCP for listening for client connection requests). It then
enters an infinite loop, polling events from the open sockets.
* Similarly, the `subscriber` initializes the TCP communication socket, then
sends its ID to the server, waiting for confirmation of connection.
* The Nagle algorithm is deactivated for all the TCP sockets, for a faster
transmission.
* The server checks if the client already exists and if it is already connected
and sends a response to the requester. If accepted, a new `pollfd` entry is
added to the `poll_fds` vector and `poll()` starts checking it too for events.
If the client already existed but was reconnecting, the server would only
update its status as connected and set the new file descriptor, also adding the
new `pollfd` entry.
* The subscriber then also enters an infinite loop of polling events, only from
stdin and from the TCP socket.
* The subscriber can receive `subscribe/unsubscribe` commands from the user on
stdin. It then sends a message to the server, asking if the operation can be
done or not (i.e. if it is already subscribed to the respective topic). The
server checks its database and sends an answer to the client.
* The subscriber can also get an `exit` message from stdin. It automatically
sends an empty message to the server to announce its exit and then closes. The
server marks it as not connected anymore and closes the respective file
descriptor, also deleting the `pollfd` entry, but keeps the subscription list,
so the client can reconnect and still have access to its subscriptions.
* The server can receive messages from UDP clients, based on a predefined (by
the PCOM team) protocol. It checks the client database to find those clients
that are subscribed to the given topic and then sends each one of them the
received message.
* When a client subscribes to a topic, that topic can contain `wildcards`,
either a `*` or a `+`. The `*` holds the place for an indefinite number of
levels in a hierarchy of strings separated by `/`, while the `+` only supplies
for one level. When checking if a client is subscribed to a topic, the server
also takes wildcards into consideration.
* If the client receives an `exit` message from stdin, it automatically sends
a message to each connected subscriber to announce the exiting, then frees all
the resources and closes. All the clients must also close.

---

## Punctual implementation details
* For managing the `pollfd` entries, a C++ `vector` is used in both server and
subscriber. Entries are added using the `push_back()` method, and resizing is
done behind the scenes.
* The subscriber only uses two `pollfd` structures, but the server can have an
unlimited number of them.
* A `vector::iterator` is used to traverse the `pollfd` structures of the
connected clients, to allow for removal (using `vector::erase`) if one of them
disconnects.
* For the server side, a `client` structure was created, that stores the file
descriptor currently associated with the client, the connected status and the
topics that it has subscribed to.
* The server stores the clients in an `unordered_map`, with the `key` being the
ID of the client (as a `string`), and the `value` being a pointer to a `client`
structure. Thus, it is very easy to check if a client with a given ID already
exists.
* Interestingly, I firstly allocated the `client` with `malloc()`, but when
I used `free()`, the structure (`map`) used for subscription storage was not
calling its destructor (I got a memory leak). Thus, I allocated the `client`
with `new` and freed it with `delete`, and the memory leak was gone (probably
the destructor for the `map` was now triggered), so another new thing learnt.
* For the messages coming from UDP, I did not see any use for a dedicated
structure, so I received directly in a pre-allocated buffer. I don't think
anything more efficient could have been done here, the size of the UDP message
is unknown until after being received. The messages are then interpreted
according to the pre-defined protocol, and stored using `sprintf()` in a buffer
to send to the TCP clients. Here, the sending is efficient, using the protocol
over TCP previously discussed.

---

## Wildcard handling
* The `client` structure stores the subscribed topics in a `map` in the
following way: the `key` is the actual topic as a `string`, while the value is
a `vector` of `strings`, which stand for the tokens obtained by tokenizing the
topic with the `/` delimiter. Thus, when checking if a user is subscribed to a
given topic, firstly the `map::find` method is used, which checks the topics
(the keys) without wildcards, because it would not make sense to trigger the
wildcard checking algorithm if the topic could be directly found.
* I decided to use a `map` here instead of an `unordered_map` based on a debate
from `StackOverflow`, from where I learnt that the `map` might be faster for
repeated insertions (such as these subscriptions) than the `unordered_map`, 
because the former does it in `O(log n)` always, while the latter has a worst
case of `O(n)` and an amortized `O(1)`. The unordered one looks to be better
for static collections. I obviously did not find much of a difference by
testing on small inputs.
* The tokenization of the stored topic is only done at insertion, so when a
wildcard checking is necessary, only the requested topic is tokenized, then,
for each topic the client is subscribed to, the two vectors of tokens are
compared, according to the rules of the wildcards. The algorithm here is simple
enough, no `strcmp()` or anything like that is needed because strings are used,
just a simultaneous iteration of the two vectors is done.
* The tokenization was done using `stringstream` of C++ and the `getline()`
function, because I realized it would not be a good idea to repeatedly convert
strings to char arrays and use `strtok()`, as I did for the stdin input. So
this was another interesting thing learnt.

---

## Final thoughts
* It is worth noting that all the tests pass successfully on my machine, and
that I also tested manually all the situations I could think of, using
`valgrind`, and there are no errors or memory leaks.
* I found this homework to be tough, but also challenging in a good way,
forcing me to use things I hadn't used before and to think very low level in
terms of memory and efficiency.

---

## Bibliography
* The 7th laboratory was used as the base for sockets and multiplexing using
poll: https://pcom.pages.upb.ro/labs/lab7/tcp.html
* To learn how to organize the code in classes:
https://www.learncpp.com/cpp-tutorial/classes-and-header-files/
* For various function documentations in C++: https://cplusplus.com/reference/
* The debate on `map` vs `unordered_map`:
https://stackoverflow.com/questions/2196995/is-there-any-advantage-of-using-map-over-unordered-map-in-case-of-trivial-keys
