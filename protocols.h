#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>


#define MAX_MSG_SIZE 1510

// Used by the server to keep track of the clients that connect and disconnect
// and of the topics they subscribe to and unsubscribe from.
struct client {
    // This will change when disconnecting and connecting again.
    int curr_fd;
    bool connected;
};


struct tcp_message {
    size_t len;
    char payload[MAX_MSG_SIZE];
};


/**
 * Receives exactly len bytes from socket sockfd and
 * stores them into buffer.
 * @return Number of bytes received, -1 on error,
 * 0 if sender closed connection.
 */
int recv_all(int sockfd, void *buffer, size_t len);


/**
 * Sends exactly len bytes from buffer to socket sockfd.
 * @return Number of bytes sent or -1 on error.
 */
int send_all(int sockfd, void *buffer, size_t len);


#endif /* PROTOCOLS_H */
