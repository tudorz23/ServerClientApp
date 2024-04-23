#include "Subscriber.h"

#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "utils.h"
#include <cstdlib>

using namespace std;


Subscriber::Subscriber(string id, uint32_t server_ip, uint16_t server_port) {
    this->id = id;
    this->server_ip = server_ip;
    this->server_port = server_port;
}


Subscriber::~Subscriber() {
    close(tcp_sockfd);
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
    tcp_message *msg = (tcp_message*) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    msg->command = CONNECT_REQ;
    msg->len = htons(id.length() + 1);

    msg->payload = (char *) malloc(id.length() + 1);
    DIE(!msg->payload, "malloc failed\n");
    strcpy(msg->payload, id.c_str());

    int rc = send_efficient(tcp_sockfd, msg);
    DIE(rc < 0, "Error sending id to the server\n");

    free(msg->payload);
    memset(msg, 0, sizeof(tcp_message));

    // Receive response regarding acceptance from server.
    rc = recv_efficient(tcp_sockfd, msg);
    DIE(rc < 0, "Error receiving connect confirmation from the server\n");

    if (msg->command == CONNECT_ACCEPTED) {
        cout << "Connection accepted\n";

        free(msg);
        return true;
    }

    // Connect denied.
    cout << "Connection denied\n";

    free(msg);
    return false;
}


void Subscriber::run() {
    pollfd stdin_pollfd;
    stdin_pollfd.fd = STDIN_FILENO;
    stdin_pollfd.events = POLLIN;

    pollfd tcp_pollfd;
    tcp_pollfd.fd = tcp_sockfd;
    tcp_pollfd.events = POLLIN;

    poll_fds.push_back(stdin_pollfd);
    poll_fds.push_back(tcp_pollfd);

    tcp_message *msg = (tcp_message *) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    while (true) {
        int rc = poll(poll_fds.data(), poll_fds.size(), -1);
        DIE(rc < 0, "[SUBSCRIBER] poll failed.\n");

        if (poll_fds[0].revents & POLLIN) {
            // Received something from stdin.
            if (manage_stdin_data()) {
                free(msg);
                break;
            }
        }

        if (poll_fds[1].revents & POLLIN) {
            // Got message from server.
            if (manage_tcp_data(msg)) {
                free(msg);
                break;
            }
        }
    }
}


bool Subscriber::manage_stdin_data() {
    string stdin_data;

    getline(cin, stdin_data, '\n');

    if (stdin_data == "exit") {
        return true;
    }

    return false;
}


bool Subscriber::manage_tcp_data(tcp_message *msg) {
    memset(msg, 0, sizeof(tcp_message));

    int rc = recv_efficient(tcp_sockfd, msg);
    DIE(rc < 0, "Failed to receive message from the server\n");

    if (rc == 0) {
        // Connection has been closed.
        return true;
    }

    return false;
}
