#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>


#define MAX_ID_SIZE 11

#define CONNECT_REQ 0
#define CONNECT_ACCEPTED 1
#define CONNECT_DENIED 2
#define EXIT 3
#define SUBSCRIBE 4
#define UNSUBSCRIBE 5
#define MSG_FROM_UDP 6


/**
 * Used by the server to keep track of the clients that connect and
 * disconnect and of the topics they subscribe to and unsubscribe from.
 */
struct client {
    // This will change when disconnecting and connecting again.
    int curr_fd;
    bool is_connected;
};


struct tcp_message {
    uint16_t command;
    uint16_t len; // length of the payload
    char *payload;
};



/**
 * Specialized send function that uses TCP's send().
 * Sends a tcp_message struct, by first sending the command and the len,
 * then sending the payload.
 * @param sockfd Socket used to send the message
 * @return Number of bytes sent on success, -1 on error
 */
int send_efficient(int sockfd, tcp_message *msg);


/**
 * Specialized recv function that uses TCP's recv().
 * Receives a tcp_message struct, by first receiving the command and the len,
 * then allocating memory for the payload for the len it just received.
 * Finally, it receives the payload.
 * @param sockfd Socket used to receive the message
 * @return Number of bytes received, on success, 0 if the sender closed
 * the connection, -1 on error.
 */
int recv_efficient(int sockfd, tcp_message *msg);


#endif /* PROTOCOLS_H */
