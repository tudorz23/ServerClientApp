#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <string>


// Used by the server to keep track of the clients that connect and disconnect
// and of the topics they subscribe to and unsubscribe from.
struct client {
    // This will change when disconnecting and connecting again.
    int curr_fd;
    bool connected;
};


#endif /* PROTOCOLS_H */
