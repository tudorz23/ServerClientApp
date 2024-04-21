#include "Subscriber.h"

#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "utils.h"
#include "protocols.h"
#include <cstdlib>

using namespace std;


Subscriber::Subscriber(string id, uint32_t server_ip, uint16_t server_port) {
    this->id = id;
    this->server_ip = server_ip;
    this->server_port = server_port;
}


void Subscriber::prepare() {
    // Create TCP socket to connect to the server.
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sockfd < 0, "[SUBSCRIBER] TCP socket creation failed.\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = server_ip;

    // Disable Nagle algorithm.
    int opt_flag = 1;
    int rc = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt_flag, sizeof(int));
    DIE(rc < 0, "[SUBSCRIBER] Nagle disabling failed.\n");

    // Connect to the server.
    rc = connect(tcp_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(rc < 0, "[SUBSCRIBER] Connection to server failed.\n");
}


bool Subscriber::check_validity() {
    // Send client id to the server.
    id_message *msg = (id_message*) calloc(1, sizeof(id_message));
    DIE(!msg, "malloc failed\n");

    msg->len = htons(id.length() + 1);
    strcpy(msg->payload, id.c_str());
    int rc = send_all(tcp_sockfd, msg, sizeof(id_message));
    DIE(rc < 0, "Error sending id\n");


    memset(msg, 0, sizeof(id_message));
    rc = recv_all(tcp_sockfd, msg, sizeof(id_message));
    DIE(rc < 0, "Error receiving OK status from server\n");

//    if (strcmp(msg->payload, "OK") == 0) {
//        cout << "OK\n";
//    } else {
//        cout << "NOT OK\n";
//    }

    cout << msg->payload << "\n";

    free(msg);

    return true;
}
