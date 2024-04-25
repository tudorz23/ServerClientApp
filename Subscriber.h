#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <string>
#include <cstdint>
#include <vector>
#include <poll.h>

#include "protocols.h"


class Subscriber {
 private:
    std::string id;
    uint32_t server_ip;
    uint16_t server_port;

    int tcp_sockfd; // Socket to communicate with the server.
    std::vector<pollfd> poll_fds;


    /**
     * Sends subscribe/unsubscribe request to the server, then
     * waits for server's response.
     * @param command Flag for subscribe/unsubscribe
     * @param topic topic to subscribe/unsubscribe to/from
     */
    void subscribe_unsubscribe_topic(uint16_t command, char *topic);


    /**
     * Parses input collected from the STDIN socket.
     * If the command is subscribe/unsubscribe, calls the
     * subscribe_unsubscribe_topic() method to manage it.
     * @return true if the input is "exit",false otherwise
     */
    bool manage_stdin_data();


    /**
     * Receives a message from the server. It can either be a disconnection
     * announcement or a message that a UDP client had sent to the server.
     * @param msg structure to receive the message into
     * @return true if the connection was closed, false otherwise
     */
    bool manage_tcp_data(tcp_message *msg);


 public:

    /**
     * Constructor.
     * @param id String id of the Subscriber (at most 10 characters)
     */
    Subscriber(std::string id, uint32_t server_ip, uint16_t server_port);


    /**
     * Destructor. Closes the tcp_sockfd.
     */
    ~Subscriber();


    /**
     * Sets up the TCP socket and connects to the server.
     */
    void prepare();


    /**
     * Sends the ID to the server and waits for confirmation of acceptance.
     * @return true, if the connection was accepted, false otherwise.
     */
    bool check_connection_validity();


    /**
     * The main control function for the subscriber. It calls poll() on the
     * poll_fds vector and manages the possible events that can occur on
     * any socket_fd (messages from stdin, messages from the server).
     */
    void run();
};


#endif /* SUBSCRIBER_H */
