#include "Subscriber.h"
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <cstdlib>
#include "utils.h"

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
    DIE(tcp_sockfd < 0, "Subscriber: TCP socket creation failed.\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = server_ip;

    // Disable Nagle algorithm.
    int opt_flag = 1;
    int rc = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt_flag, sizeof(int));
    DIE(rc < 0, "Subscriber: Nagle disabling for TCP socket failed.\n");

    // Connect to the server.
    rc = connect(tcp_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(rc < 0, "Subscriber: Connection to server failed.\n");
}


bool Subscriber::check_connection_validity() {
    // Send client id to the server.
    tcp_message *msg = (tcp_message*) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    msg->command = CONNECT_REQ;
    msg->len = id.length() + 1;

    msg->payload = (char *) malloc(msg->len);
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
        free(msg);
        return true;
    }

    // Connection denied.
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
        DIE(rc < 0, "Subscriber: poll failed.\n");

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


void Subscriber::subscribe_unsubscribe_topic(uint8_t command, char *topic) {
    tcp_message *msg = (tcp_message *) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    msg->command = command;
    msg->len = strlen(topic) + 1;
    msg->payload = (char *) malloc(msg->len);
    DIE(!msg->payload, "malloc failed\n");

    strcpy(msg->payload, topic);

    // Send the subscribe request to the server.
    int rc = send_efficient(tcp_sockfd, msg);
    DIE(rc < 0, "Error sending subscribe/unsubscribe request to the server\n");

    free(msg->payload);
    memset(msg, 0, sizeof(tcp_message));

    // Wait for confirmation from the server.
    rc = recv_efficient(tcp_sockfd, msg);
    DIE(rc < 0, "Error receiving subscribe confirm from the server\n");

    if (command == SUBSCRIBE_REQ) {
        if (msg->command == SUBSCRIBE_SUCC) {
            cout << "Subscribed to topic " << topic << "\n";
        } else {
            cout << "Already subscribed to topic " << topic << "\n";
        }

        free(msg);
        return;
    }

    // Command was UNSUBSCRIBE_REQ
    if (msg->command == UNSUBSCRIBE_SUCC) {
        cout << "Unsubscribed from topic " << topic << "\n";
    } else {
        cout << "Not subscribed to topic " << topic << ", cannot unsubscribe\n";
    }

    free(msg);
}


bool Subscriber::manage_stdin_data() {
    // Use string so the user can input data as long as wanted.
    string stdin_data;

    getline(cin, stdin_data, '\n');

    if (stdin_data == "exit") {
        return true;
    }

    char *helper = strdup(stdin_data.c_str());
    DIE(!helper, "strdup failed\n");

    char *command = strtok(helper, " ");

    if (!command) {
        free(helper);
        cout << "Accepted commands: <exit> <subscribe> <unsubscribe>\n";
        return false;
    }

    if (strcmp(command, "subscribe") == 0) {
        char *topic = strtok(NULL, "\n ");

        if (!topic || strlen(topic) > 50) {
            cout << "Invalid topic. Topic must have at most 50 characters.\n";
            free(helper);
            return false;
        }

        subscribe_unsubscribe_topic(SUBSCRIBE_REQ, topic);
        free(helper);
        return false;
    }

    if (strcmp(command, "unsubscribe") == 0) {
        char *topic = strtok(NULL, "\n ");

        if (!topic || strlen(topic) > 50) {
            cout << "Invalid topic. Topic must have at most 50 characters.\n";
            free(helper);
            return false;
        }

        subscribe_unsubscribe_topic(UNSUBSCRIBE_REQ, topic);
        free(helper);
        return false;
    }

    free(helper);
    cout << "Accepted commands: <exit> <subscribe> <unsubscribe>\n";
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

    // Got a message from the server.
    if (msg->command != MSG_FROM_UDP) {
        fprintf(stderr, "Big error: message is not coming from UDP\n");
        free(msg->payload);
        return false;
    }

    cout << msg->payload << "\n";
    free(msg->payload);

    return false;
}
