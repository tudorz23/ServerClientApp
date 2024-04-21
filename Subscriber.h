#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <string>
#include <cstdint>


class Subscriber {
 private:
    std::string id;
    uint32_t server_ip;
    uint16_t server_port;

    int tcp_sockfd; // Socket to communicate with the server.


 public:

    /**
     * Constructor.
     * @param id String id of the Subscriber (at most 10 characters)
     */
    Subscriber(std::string id, uint32_t server_ip, uint16_t server_port);


    void prepare();


    bool check_validity();
};


#endif /* SUBSCRIBER_H */
