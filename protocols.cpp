#include "protocols.h"
#include "utils.h"


/**
 * Maybe not really elegant, but efficient for sure. It was a must to
 * receive the structure's attributes one by one, because, if command
 * and len were received continuously (by receiving at (uint8_t *) msg),
 * structure padding might have corrupted the data.
 */
int recv_efficient(int sockfd, tcp_message *msg) {
    int total_bytes_received = 0;
    int bytes_recv;
    int bytes_remaining;
    uint8_t *buff;

    // Firstly, receive the command.
    bytes_remaining = sizeof(msg->command);
    buff = (uint8_t *) &msg->command;

    while (bytes_remaining) {
        bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
        if (bytes_recv <= 0) {
            // Error or connection closed.
            return bytes_recv;
        }

        total_bytes_received += bytes_recv;
        buff += bytes_recv;
        bytes_remaining -= bytes_recv;
    }

    // Then, receive the len.
    bytes_remaining = sizeof(msg->len);
    buff = (uint8_t *) &msg->len;

    while (bytes_remaining) {
        bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
        if (bytes_recv <= 0) {
            // Error or connection closed.
            return bytes_recv;
        }

        total_bytes_received += bytes_recv;
        buff += bytes_recv;
        bytes_remaining -= bytes_recv;
    }

    // Convert len to host order.
    msg->len = ntohs(msg->len);

    // If len is 0, no payload should be received.
    if (msg->len == 0) {
        return total_bytes_received;
    }

    // Now, receive the payload, but firstly, allocate memory for it.
    msg->payload = (char *) malloc(msg->len);
    DIE(!msg->payload, "malloc failed\n");

    bytes_remaining = msg->len;
    buff = (uint8_t *) msg->payload;

    while (bytes_remaining) {
        bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
        if (bytes_recv <= 0) {
            // Error or connection closed.
            return bytes_recv;
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

    // Convert len to network order to prepare for sending.
    msg->len = htons(msg->len);

    // Firstly, send the command.
    bytes_remaining = sizeof(msg->command);
    buff = (uint8_t *) &msg->command;

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

    // Then, send the len.
    bytes_remaining = sizeof(msg->len);
    buff = (uint8_t *) &msg->len;

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

    // Set len back to host order.
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
