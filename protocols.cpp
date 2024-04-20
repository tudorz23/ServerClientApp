#include "protocols.h"




int recv_all(int sockfd, void *buffer, size_t len) {
    int bytes_received;
    size_t bytes_remaining = len;
    char *aux_buffer = (char *) buffer;

    while (bytes_remaining) {
        bytes_received = recv(sockfd, aux_buffer, bytes_remaining, 0);
        if (bytes_received < 0) {
            // Error.
            return -1;
        }

        if (bytes_received == 0) {
            // Connection closed.
            return 0;
        }

        bytes_remaining -= bytes_received;
        aux_buffer += bytes_received;
    }

    return len;
}


int send_all(int sockfd, void *buffer, size_t len) {
    int bytes_sent;
    size_t bytes_remaining = len;
    char *aux_buffer = (char *) buffer;

    while (bytes_remaining) {
        bytes_sent = send(sockfd, aux_buffer, bytes_remaining, 0);
        if (bytes_sent < 0) {
            // Error.
            return -1;
        }

        bytes_remaining -= bytes_sent;
        aux_buffer += bytes_sent;
    }

    return len;
}

