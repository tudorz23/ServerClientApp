#include "utils.h"
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>



struct sockaddr_in fill_sockaddr(uint16_t port) {
    struct sockaddr_in new_addr;
    memset(&new_addr, 0, sizeof(struct sockaddr_in));

    new_addr.sin_family = AF_INET;
    new_addr.sin_addr.s_addr = INADDR_ANY;
    new_addr.sin_port = htons(port);

    return new_addr;
}
