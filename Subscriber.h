#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <string>
#include <cstdint>

class Subscriber {
 private:
    std::string id;
    uint32_t server_ip;
    uint16_t server_port;

    int tcp_sockfd;


 public:

    /**
     * Constructor.
     * @param id String id of the Subscriber.
     */
    Subscriber(std::string id, uint32_t server_ip, uint16_t server_port);

    void prepare();
};


#endif /* SUBSCRIBER_H */
