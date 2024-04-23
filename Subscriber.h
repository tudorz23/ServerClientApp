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


 public:

    /**
     * Constructor.
     * @param id String id of the Subscriber (at most 10 characters)
     */
    Subscriber(std::string id, uint32_t server_ip, uint16_t server_port);


    /**
     * Destructor.
     */
    ~Subscriber();


    /**
     * Initializes the TCP socket and connects to the server.
     */
    void prepare();


    /**
     * Sends the ID to the server and waits for confirmation of acceptance.
     * @return true, if the connection was accepted, false otherwise.
     */
    bool check_validity();


    void run();


    void subscribe_unsubscribe_topic(uint16_t command, char *topic);


    bool manage_stdin_data();


    bool manage_tcp_data(tcp_message *msg);
};


#endif /* SUBSCRIBER_H */
