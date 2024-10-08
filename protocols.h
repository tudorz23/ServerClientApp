#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <map>

#define MAX_UDP_MSG 1600

#define CONNECT_REQ 0
#define CONNECT_ACCEPTED 1
#define CONNECT_DENIED 2
#define SUBSCRIBE_REQ 3
#define UNSUBSCRIBE_REQ 4
#define SUBSCRIBE_SUCC 5
#define SUBSCRIBE_FAIL 6
#define UNSUBSCRIBE_SUCC 7
#define UNSUBSCRIBE_FAIL 8
#define MSG_FROM_UDP 9


/**
 * Used by the server to keep track of the clients that connect and
 * disconnect and of the topics they subscribe to and unsubscribe from.
 */
struct client {
    // These will change when disconnecting and connecting again.
    int curr_fd;
    bool is_connected;

    // Mappings of <topic, tokens> type.
    std::map<std::string, std::vector<std::string>> subscribed_topics;
};


struct tcp_message {
    uint8_t command;
    uint16_t len; // length of the payload
    char *payload;
};


/**
 * Specialized recv function that uses TCP's recv().
 * Receives a tcp_message struct, by first receiving the command, then the
 * len, then allocating memory for the payload for the len it just received.
 * Finally, it receives the payload.
 *
 * Note that the payload must be freed manually by the user of this function.
 * Arranges len in host order after receiving.
 *
 * @param sockfd Socket used to receive the message
 * @return Number of bytes received on success, 0 if the sender closed
 * the connection, -1 on error.
 */
int recv_efficient(int sockfd, tcp_message *msg);


/**
 * Specialized send function that uses TCP's send().
 * Sends a tcp_message struct, by first sending the command, then the
 * len, then finally sending the payload.
 *
 * Arranges len in network order before sending, so the caller doesn't
 * have to do it, but then puts it back in host order, so the
 * overall message structure is left unmodified by this function.
 *
 * @param sockfd Socket used to send the message
 * @return Number of bytes sent on success, -1 on error
 */
int send_efficient(int sockfd, tcp_message *msg);


#endif /* PROTOCOLS_H */
