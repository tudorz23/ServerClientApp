#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>


#define MAX_ID_SIZE 11


/**
 * Used by the server to keep track of the clients that connect and
 * disconnect and of the topics they subscribe to and unsubscribe from.
 */
struct client {
    // This will change when disconnecting and connecting again.
    int curr_fd;
    bool connected;
};


/**
 * Used by the client to send its ID when trying to connect and
 * by the server to send confirm status back to the client.
 */
struct id_message {
    size_t len;
    char payload[MAX_ID_SIZE];
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
