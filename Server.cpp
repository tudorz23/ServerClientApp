#include "Server.h"
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "utils.h"
#include "protocols.h"
#include <cstdlib>
#include <algorithm>

#define LISTEN_BACKLOG 50

using namespace std;


Server::Server(uint16_t port) {
    this->port = port;
    this->num_pollfds = 0;
}


Server::~Server() {
    // Close all the fds (except STDIN_FILENO).
    for (int i = 1; i < num_pollfds; i++) {
        close(poll_fds[i].fd);
    }

    // Free all the client structures.
    for (auto &entry : clients) {
        delete entry.second;
    }
}


void Server::prepare_udp_socket() {
    // Create UDP socket.
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sockfd < 0, "[SERVER]: UDP socket creation failed.\n");

    // Mark socket as reusable for multiple short time runnings.
    int udp_enable = 1;
    int rc = setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &udp_enable, sizeof(int));
    DIE(rc < 0, "[SERVER]: setsockopt() for UDP failed.\n");

    // Fill sockaddr_in structure details.
    struct sockaddr_in udp_addr = fill_sockaddr(port);

    // Bind the UDP socket to the address.
    rc = bind(udp_sockfd, (const struct sockaddr *)&udp_addr, sizeof(udp_addr));
    DIE(rc < 0, "[SERVER] UDP socket bind failed.\n");
}


void Server::prepare_tcp_socket() {
    // Create TCP socket.
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sockfd < 0, "[SERVER] TCP socket creation failed.\n");

    // Mark socket as reusable for multiple short time runnings.
    int opt_flag = 1;
    int rc = setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_flag, sizeof(int));
    DIE(rc < 0, "[SERVER]: setsockopt() for TCP failed.\n");

    // Fill sockaddr_in structure details.
    struct sockaddr_in tcp_addr = fill_sockaddr(port);

    // Bind the TCP socket to the address.
    rc = bind(tcp_sockfd, (const struct sockaddr *)&tcp_addr, sizeof(tcp_addr));
    DIE(rc < 0, "[SERVER] TCP socket bind failed.\n");

    // Disable Nagle algorithm.
    rc = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt_flag, sizeof(int));
    DIE(rc < 0, "[SERVER] Nagle disabling failed.\n");

    // Set the socket to listen.
    rc = listen(tcp_sockfd, LISTEN_BACKLOG);
    DIE(rc < 0, "[SERVER] Listening failed.\n");
}


void Server::prepare() {
    prepare_udp_socket();
    prepare_tcp_socket();

    // Initialize the poll_fds vector with pollfds corresponding to
    // stdin and the UDP and TCP sockets.
    pollfd stdin_pollfd;
    stdin_pollfd.fd = STDIN_FILENO;
    stdin_pollfd.events = POLLIN;
    poll_fds.push_back(stdin_pollfd);

    pollfd udp_pollfd;
    udp_pollfd.fd = udp_sockfd;
    udp_pollfd.events = POLLIN;
    poll_fds.push_back(udp_pollfd);

    pollfd tcp_pollfd;
    tcp_pollfd.fd = tcp_sockfd;
    tcp_pollfd.events = POLLIN;
    poll_fds.push_back(tcp_pollfd);

    num_pollfds = 3;
}


void Server::run() {
    tcp_message *msg = (tcp_message*) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    char udp_msg[MAX_UDP_MSG];

    while (true) {
        // Use vector::data to access the start of the memory zone
        // internally used by the vector.
        int rc = poll(poll_fds.data(), poll_fds.size(), -1);
        DIE(rc < 0, "[SERVER] poll failed.\n");

        if (poll_fds[0].revents & POLLIN) {
            // Received something from stdin.
            if (check_stdin_data()) {
                break;
            }
        }

        if (poll_fds[1].revents & POLLIN) {
            manage_udp_message(poll_fds[1].fd, udp_msg);
        }

        if (poll_fds[2].revents & POLLIN) {
            // Received connection request on tcp_socket.
            manage_connection_request();
        }

        if (num_pollfds < 4) {
            continue;
        }

        // Iterate the client sockets.
        auto it = poll_fds.begin() + 3;
        while (it != poll_fds.end()) {
            pollfd client_pollfd = *it;

            // Flag to know if a pollfd has been deleted,
            // so the iterator is not incremented.
            bool deleted = false;

            if (client_pollfd.revents & POLLIN) {
                // Got message from client.
                memset(msg, 0, sizeof(tcp_message));
                rc = recv_efficient(client_pollfd.fd, msg);
                DIE(rc < 0, "Failed to receive message from the client\n");

                if (rc == 0) {
                    // Connection has been closed.
                    pair<string, client*> entry = get_client_from_fd(client_pollfd.fd);

                    client* exiting_client = entry.second;
                    // Mark client as disconnected.
                    exiting_client->is_connected = false;
                    exiting_client->curr_fd = -1; // the fd is not useful anymore

                    close(client_pollfd.fd);
                    it = poll_fds.erase(it);
                    num_pollfds--;
                    deleted = true;

                    cout << "Client " << entry.first << " disconnected.\n";
                } else {
                    manage_subscribe_unsubscribe(client_pollfd.fd, msg);
                    free(msg->payload);
                }
            }

            if (!deleted) {
                it++;
            }
        }
    }

    free(msg);
}


bool Server::check_stdin_data() {
    // Use string so the user can input data as long as wanted.
    string stdin_data;

    getline(cin, stdin_data, '\n');

    if (stdin_data == "exit") {
        return true;
    }

    cout << "Only exit command is supported\n";
    return false;
}


void Server::send_connection_response(bool ok_status, int client_sockfd) {
    tcp_message *msg = (tcp_message *) malloc(sizeof(tcp_message));
    DIE(!msg, "malloc failed\n");

    if (ok_status) {
        msg->command = CONNECT_ACCEPTED;
    } else {
        msg->command = CONNECT_DENIED;
    }

    msg->len = 0;

    int rc = send_efficient(client_sockfd, msg);
    DIE(rc < 0, "Error sending connection response\n");

    free(msg);
}


void Server::add_client_pollfd(int client_sockfd) {
    pollfd client_pollfd;
    client_pollfd.fd = client_sockfd;
    client_pollfd.events = POLLIN;

    poll_fds.push_back(client_pollfd);
    num_pollfds++;
}


void Server::manage_connection_request() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sockfd = accept(tcp_sockfd, (struct sockaddr*)&client_addr, &client_len);
    DIE(client_sockfd < 0, "[SERVER] accept failed.\n");

    // Disable Nagle algorithm.
    int opt_flag = 1;
    int rc = setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt_flag, sizeof(int));
    DIE(rc < 0, "[SERVER] Nagle disabling for client failed.\n");

    tcp_message *msg = (tcp_message*) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    rc = recv_efficient(client_sockfd, msg);
    DIE(rc < 0, "Failed to receive client id\n");

    string client_id = string(msg->payload);

    free(msg->payload);
    free(msg);

    // Check if the id is already present in the id-client map.
    unordered_map<string, client*>::iterator it = clients.find(client_id);
    if (it == clients.end()) {
        // It is a new client, so add it to the map.
        client* new_client;
        try {
            new_client = new client();
        } catch (bad_alloc &exception) {
            fprintf(stderr, "Client allocation failed\n");
            exit(-1);
        }

        new_client->curr_fd = client_sockfd;
        new_client->is_connected = true;

        clients.insert({client_id, new_client});

        // Add the pollfd to the vector of poll_fds.
        add_client_pollfd(client_sockfd);

        // Send confirmation.
        send_connection_response(true, client_sockfd);

        cout << "New client " << client_id << " connected from "
            << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ".\n";

        return;
    }

    // Client id is already registered. Check if the client is already connected.
    client* database_client = it->second;
    if (database_client->is_connected) {
        // Send decline message to the client and tell it to exit.
        send_connection_response(false, client_sockfd);

        cout << "Client " << client_id << " already connected.\n";

        return;
    }

    // Client is not connected, so update the client struct and mark as connected.
    database_client->is_connected = true;
    database_client->curr_fd = client_sockfd;

    // Add the pollfd to the vector of poll_fds.
    add_client_pollfd(client_sockfd);

    // Send confirmation.
    send_connection_response(true, client_sockfd);

    cout << "New client " << client_id << " connected from "
         << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "\n";
}


pair<string, client*> Server::get_client_from_fd(int fd) {
    for (auto const& entry : clients) {
        if (entry.second->curr_fd == fd) {
            return entry;
        }
    }

    // Never reached.
    pair<string, client*> placeholder("placeholder", NULL);
    return placeholder;
}


void Server::manage_subscribe_unsubscribe(int client_fd, tcp_message *req_msg) {
    string topic(req_msg->payload);
    client *req_client = get_client_from_fd(client_fd).second;
    int rc;

    tcp_message *ans_msg = (tcp_message *) calloc(1, sizeof(tcp_message));
    DIE(!ans_msg, "calloc failed\n");
    ans_msg->len = 0; // will not send any payload.

    vector<string>::iterator it = find(req_client->subscribed_topics.begin(),
                                      req_client->subscribed_topics.end(), topic);

    if (req_msg->command == SUBSCRIBE_REQ) {
        if (it != req_client->subscribed_topics.end()) {
            // Client is already subscribed to the topic.
            ans_msg->command = SUBSCRIBE_FAIL;
            rc = send_efficient(client_fd, ans_msg);
            DIE(rc < 0, "Failed to send error msg for subscribe\n");

            free(ans_msg);
            return;
        }

        req_client->subscribed_topics.push_back(topic);
        ans_msg->command = SUBSCRIBE_SUCC;
        rc = send_efficient(client_fd, ans_msg);
        DIE(rc < 0, "Failed to send success msg for subscribe\n");

        free(ans_msg);
        return;
    }

    // UNSUBSCRIBE_REQ
    if (it == req_client->subscribed_topics.end()) {
        ans_msg->command = UNSUBSCRIBE_FAIL;
        rc = send_efficient(client_fd, ans_msg);
        DIE(rc < 0, "Failed to send error msg for unsubscribe\n");

        free(ans_msg);
        return;
    }

    req_client->subscribed_topics.erase(it);
    ans_msg->command = UNSUBSCRIBE_SUCC;
    rc = send_efficient(client_fd, ans_msg);
    DIE(rc < 0, "Failed to send success msg for unsubscribe\n");

    free(ans_msg);
}


void Server::manage_udp_message(int client_fd, char *buff) {
    struct sockaddr_in udp_client_addr;
    socklen_t udp_len = sizeof(udp_client_addr);

    memset(buff, 0, MAX_UDP_MSG);

    int rc = recvfrom(client_fd, buff, MAX_UDP_MSG, 0,
                      (struct sockaddr*) &udp_client_addr, &udp_len);
    DIE(rc < 0, "Error receiving from UDP client\n");

    char *client_ip = inet_ntoa(udp_client_addr.sin_addr);
    uint16_t client_port = htons(udp_client_addr.sin_port);

    string topic(buff);

    uint8_t int_data = buff[50];

    cout << "Topic is: " << topic << "\n";

    switch ((int) int_data) {
        case 0:
            uint8_t sign = buff[51];

            uint32_t number = 0;
            memcpy(&number, buff + 52, sizeof(uint32_t));

            number = ntohl(number);

            cout << "Data type is INT and data is: ";
            if (sign == 1) {
                cout << "-";
            }
            cout << number << "\n";

//            tcp_message* msg = (tcp_message *) calloc(1, sizeof(tcp_message));
//            DIE(!msg, "calloc failed\n");
//
//            msg->command = MSG_FROM_UDP;

            char buffer[80];
            sprintf(buffer, "%s:%hu - ", client_ip, client_port);
            sprintf(buffer + strlen(buffer), "%s - INT - ", topic.c_str());

            if (sign == 1) {
                sprintf(buffer + strlen(buffer), "-");
            }

            sprintf(buffer + strlen(buffer), "%d", number);

            cout << buffer << "\n";

            break;
    }
}
