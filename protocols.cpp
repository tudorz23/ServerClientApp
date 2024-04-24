#include "protocols.h"

#include <cstdlib>
#include "utils.h"


int recv_efficient(int sockfd, tcp_message *msg) {
    int total_bytes_received = 0;
    int bytes_recv;
    int bytes_remaining;
    uint8_t *buff;

    // Firstly, receive the command and the len.
    bytes_remaining = sizeof(msg->command) + sizeof(msg->len);
    buff = (uint8_t *) msg;

    while (bytes_remaining) {
        bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
        if (bytes_recv < 0) {
            // Error.
            return -1;
        }

        if (bytes_recv == 0) {
            // Connection closed.
            return 0;
        }

        total_bytes_received += bytes_recv;
        buff += bytes_recv;
        bytes_remaining -= bytes_recv;
    }

    msg->command = ntohs(msg->command);
    msg->len = ntohs(msg->len);

    // If len is 0, no payload should be received.
    if (msg->len == 0) {
        return total_bytes_received;
    }

    // Now, receive the payload, but firstly allocate memory for it.
    msg->payload = (char *) malloc(msg->len);
    DIE(!msg->payload, "malloc failed\n");

    bytes_remaining = msg->len;
    buff = (uint8_t *) msg->payload;

    while (bytes_remaining) {
        bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
        if (bytes_recv < 0) {
            // Error.
            return -1;
        }

        if (bytes_recv == 0) {
            // Connection closed.
            return 0;
        }

        total_bytes_received += bytes_recv;
        buff += bytes_recv;
        bytes_remaining -= bytes_recv;
    }

    return total_bytes_received;
}


int send_efficient(int sockfd, tcp_message *msg) {
    int total_bytes_sent = 0;
    int bytes_sent;
    int bytes_remaining;
    uint8_t *buff;

    msg->command = htons(msg->command);
    msg->len = htons(msg->len);

    // Firstly, send the command and the len.
    bytes_remaining = sizeof(msg->command) + sizeof(msg->len);
    buff = (uint8_t *) msg;

    while (bytes_remaining) {
        bytes_sent = send(sockfd, buff, bytes_remaining, 0);
        if (bytes_sent < 0) {
            // Error.
            return -1;
        }

        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
        buff += bytes_sent;
    }

    // Set command and len back to host order.
    msg->command = ntohs(msg->command);
    msg->len = ntohs(msg->len);

    // Now, send the payload.
    bytes_remaining = msg->len;
    buff = (uint8_t *) msg->payload;

    while (bytes_remaining) {
        bytes_sent = send(sockfd, buff, bytes_remaining, 0);
        if (bytes_sent < 0) {
            // Error.
            return -1;
        }

        bytes_remaining -= bytes_sent;
        total_bytes_sent += bytes_sent;
        buff += bytes_sent;
    }

    return total_bytes_sent;
}
