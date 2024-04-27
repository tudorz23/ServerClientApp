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
* Hint: this file can be saved with the `.md` extension to be interpreted as a
`Markdown` document (the requirement was for the readme to be in txt format).

---

## 2. File distribution
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

## 3. Running
* The system can be compiled by using the `make` command in the root directory.
* To run the server, the command `./server <PORT>` should be used.
* The command `./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>` should be
used to run the TCP subscriber.
* Both the server and the subscriber can be stopped with the `exit` command.

---

## 4. General details and design choices
* This app is written in C++ for the ability to use the STL (particularly, the
`vector` and `map` containers), so there is no need to manually manage the
memory to handle an indefinite number of clients and subscriptions for each.
* I chose to use classes for the server and for the subscriber, thus being able
to access their class attributes from anywhere inside the class. Without
classes, global variables should have been used, or the variables should have
been initialized in the `main()` functions, and then passed as parameters to
all the functions that needed them. So, the class solution is the best in my
opinion.
* For managing the connected/disconnected clients inside the server class, I
use the `client` structure. When I made the decision, I thought that there will
not be many functions associated with the `client` structure, and there are
not, but now I think that using a `Client` class would have been a little more
elegant and in tone with the rest of the program.
* This was my first-ever project written in real C++, so it was a continuous
learning process, so I will explain not only what decisions I made along the
way, but also why and how I got to them.

---

## 5. The Protocol over TCP
* Probably the most important part of the homework (and the main requirement)
was the creation of an efficient and robust protocol for sending messages above
the TCP.
* For that, I used the `tcp_message` structure, which encapsulates the
`command` type, the `len` of the payload and the `payload` itself (the actual
message sent).
* The `command` field takes a value from 0 to 9 and marks the role of the
message structure (described as `#define` directives in `protcols.h`).
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
occurred (basically it was luck).
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

## 6. Program flow and logic
* The `server` program starts by initializing the basic sockets and `pollfd`
structs that would be used (stdin for messages from user, UDP for messages
from UDP client and TCP for listening for client connection requests). It then
enters an infinite loop, polling events from the open sockets.
* Similarly, the `subscriber` initializes the TCP communication socket, then
sends its ID to the server, waiting for confirmation of connection.
* The Nagle algorithm is deactivated for all the TCP sockets, for a faster
transmission.
* The server checks if the client already exists and if it is already connected
and sends a response to the requester. If accepted, a new `pollfd` entry is
added to the `poll_fds` vector and `poll()` starts checking it too for events.
If the client already existed but was reconnecting, the server would only
update it status as connected and set the new file descriptor, also adding the
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
* The server can receive messages from a UDP client, based on a predefined (by
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
the resources and closes. All the clients to the same thing.

---

## 7. Punctual implementation details
* For managing the `pollfd` entries, a C++ `vector` is used in both server and
subscriber. Entries are added using the `push_back()` method, and resizing is
done behind the scenes.
* The subscriber only uses two `pollfd` structures, but the server can have an
unlimited number of them. That number is stored as a class attribute and always
updated, because the `Server::run` method uses it in a `while(true)` loop, so
if `vector::size()` was used, it might have been inefficient for a large number
of entries.
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
the destructor for the `map` was now called), so another new thing learnt.
* For the messages coming from UDP, I did not see any use for a dedicated
structure, so I received directly in a pre-allocated buffer. I don't think
anything more efficient could have been done here, the size of the UDP message
is unknown until after being received. The messages are then interpreted
according to the pre-defined protocol, and stored using `sprintf()` in a buffer
to send to the TCP clients. Here, the sending is efficient, using the protocol
over TCP previously discussed.

---

## 8. Wildcard handling
* The `client` structure stores the subscribed topics in a `map` in the
following way: the `key` is the actual topic as a `string`, while the value is
a `vector` of `strings`, which stand for the tokens obtained by tokenizing the
topic with the `/` delimiter. Thus, when checking if a user is subscribed to a
given topic, firstly the `map::find` method is used, which checks the topics
without wildcards, because it would not make sense to trigger the wildcard
checking algorithm if the topic could be directly found.
* I decided to use a `map` here instead of an `unordered_map` based on a debate
from `StackOverflow`, from where I learnt that the `map` might be faster for
repeated insertions (such as these subscriptions) than the `unordered_map`, 
because the former does it in `O(log n)` always, while the latter has a worst
case of `O(n)` and an amortized `O(1)`. The unordered one looks to be better
for static collections. I obviously did not find much of a difference by
testing on small inputs.
* Because the tokenization of the stored topic is only done at insertion, when
a wildcard checking is necessary, only the requested topic is tokenized, then,
for each topic the client is subscribed to, the two vectors of tokens are
compared, according to the rules of the wildcards. The algorithm here is simple
enough, no `strcmp()` or anything like that is needed because strings are used,
just a simultaneous iteration of the two vectors is done.
* The tokenization was done using `stringstream` of C++ and the `getline()`
function, because I realized it would not be a good idea to repeatedly convert
strings to char arrays and use `strtok()`, as I did for the stdin input. So
this was another interesting thing learnt.

---

## 9. Final thoughts
* It is worth noting that all the tests pass successfully on my machine, and
that I also tested manually all the situations I could think of, using
`valgrind`, and there are no errors or memory leaks.
* I found this homework to be tough, but also challenging in a good way,
forcing me to use things I hadn't used before and to think very low level in
terms of memory and efficiency.
* I really worked very hard on this assignment and tried to think the program
as thorough as I could. I find the implementation to be really robust. Also,
defensive programming principles were used everywhere.

---

## 10. Bibliography
* The 7th laboratory was used as the base for sockets and multiplexing using
poll: https://pcom.pages.upb.ro/labs/lab7/tcp.html ;
* To learn how to organize the code in classes:
https://www.learncpp.com/cpp-tutorial/classes-and-header-files/ ;
* For various functions documentations in C++:
https://cplusplus.com/reference/ ;
* The debate on `map` vs `unordered_map`:
https://stackoverflow.com/questions/2196995/is-there-any-advantage-of-using-map-over-unordered-map-in-case-of-trivial-keys
