#include "Server.h"
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "utils.h"
#include "protocols.h"
#include <cstdlib>
#include <algorithm>
#include <sstream>

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

    // Buffers to avoid repeatedly allocating memory.
    char udp_msg[MAX_UDP_MSG];
    char formatted_msg[MAX_UDP_MSG];

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
            manage_udp_message(poll_fds[1].fd, udp_msg, formatted_msg);
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

    getline(cin, stdin_data);

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
    client_pollfd.revents = 0;

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

    map<string, vector<string>>::iterator it;
    it = req_client->subscribed_topics.find(topic);

    if (req_msg->command == SUBSCRIBE_REQ) {
        if (it != req_client->subscribed_topics.end()) {
            // Client is already subscribed to the topic.
            ans_msg->command = SUBSCRIBE_FAIL;
            rc = send_efficient(client_fd, ans_msg);
            DIE(rc < 0, "Failed to send error msg for subscribe\n");

            free(ans_msg);
            return;
        }

        // Subscribe to the new topic.
        vector<string> tokens;
        tokenize_topic(topic, tokens);
        req_client->subscribed_topics.insert({topic, tokens});

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


void Server::manage_udp_message(int client_fd, char *buff, char *formatted_msg) {
    struct sockaddr_in udp_client_addr;
    socklen_t udp_len = sizeof(udp_client_addr);

    memset(buff, 0, MAX_UDP_MSG);
    memset(formatted_msg, 0, MAX_UDP_MSG);

    int rc = recvfrom(client_fd, buff, MAX_UDP_MSG, 0,
                      (struct sockaddr*) &udp_client_addr, &udp_len);
    DIE(rc < 0, "Error receiving from UDP client\n");

    // Get the topic from the received buff.
    char topic[51];
    memcpy(topic, buff, 50);
    topic[50] = '\0';

    // Data type from the received buff.
    uint8_t data_type = buff[50];

    bool valid = interpret_udp_payload((int) data_type, buff + 51, topic,
                               udp_client_addr, formatted_msg);
    if (valid) {
        send_msg_if_subscribed(topic, formatted_msg);
//        cout << "Sent msg: " << formatted_msg << "\n\n";
    }
}


bool Server::interpret_udp_payload(int data_type, char *udp_payload, char *topic,
                                    struct sockaddr_in &udp_client_addr, char *buffer) {
    char *client_ip = inet_ntoa(udp_client_addr.sin_addr);
    uint16_t client_port = htons(udp_client_addr.sin_port);

    sprintf(buffer, "%s:%hu - ", client_ip, client_port);
    sprintf(buffer + strlen(buffer), "%s - ", topic);

    switch (data_type) {
        case 0: {
            uint8_t sign = udp_payload[0];
            uint32_t number = 0;
            memcpy(&number, udp_payload + 1, sizeof(uint32_t));

            number = ntohl(number);

            if (sign == 1) {
                number *= -1;
            }

            sprintf(buffer + strlen(buffer), "INT - %d", number);
            return true;
        }
        case 1: {
            uint16_t number = 0;
            memcpy(&number, udp_payload, sizeof(uint16_t));

            double real_nr = (double) ntohs(number);
            real_nr /= 100;

            sprintf(buffer + strlen(buffer), "SHORT_REAL - %.2lf", real_nr);
            return true;
        }
        case 2: {
            uint8_t sign = udp_payload[0];
            uint32_t number = 0;
            memcpy(&number, udp_payload + 1, sizeof(uint32_t));

            double real_nr = (double) ntohl(number);

            uint8_t power = udp_payload[5];

            for (int i = 0; i < (int) power; i++) {
                real_nr /= 10.0;
            }

            if (sign == 1) {
                real_nr *= -1.0;
            }

            sprintf(buffer + strlen(buffer), "FLOAT - %.*f", power, real_nr);
            return true;
        }
        case 3: {
            sprintf(buffer + strlen(buffer), "STRING - ");
            strcpy(buffer + strlen(buffer), udp_payload);
            return true;
        }
        default: {
            fprintf(stderr, "This format is not supported\n");
            return false;
        }
    }
}


void Server::send_msg_if_subscribed(char *topic, char *formatted_msg) {
    string string_topic(topic);

    tcp_message *msg = (tcp_message *) calloc(1, sizeof(tcp_message));
    DIE(!msg, "calloc failed\n");

    msg->command = MSG_FROM_UDP;
    msg->len = strlen(formatted_msg) + 1;
    msg->payload = strdup(formatted_msg);
    DIE(!msg->payload, "strdup failed\n");

    for (auto &entry : clients) {
        client *curr_client = entry.second;
        if (!curr_client->is_connected) {
            continue;
        }

        auto it = curr_client->subscribed_topics.find( string_topic);
        if (it == curr_client->subscribed_topics.end()) {
            // Did not find the topic directly, try with wildcards.
            if (!check_wildcard_topic(string_topic, curr_client->subscribed_topics)) {
                continue;
            }
        }

        // Send the message to this client.
        send_efficient(curr_client->curr_fd, msg);
    }

    free(msg->payload);
    free(msg);
}


void Server::tokenize_topic(string &topic, vector<string> &tokens) {
    char delim = '/';
    stringstream topic_stream(topic);
    string token;

    while (getline(topic_stream, token, delim)) {
        tokens.push_back(token);
    }
}



bool Server::check_wildcard_topic(string &topic, map<string, vector<string>> &subscr_topics) {
    vector<string> new_tokens;
    tokenize_topic(topic, new_tokens);

    string star_str("*");
    string plus_str("+");

    for (auto &entry : subscr_topics) {
        if (compare_token_vectors(entry.second, new_tokens, star_str, plus_str)) {
            return true;
        }
    }
    return false;
}



bool Server::compare_token_vectors(vector<string> &old_tokens, vector<string> &new_tokens,
                                   string &star_str, string &plus_str) {
    int new_size = new_tokens.size();
    int old_size = old_tokens.size();

    int it_old = -1; // iterator through old_tokens
    int it_new = -1; // iterator through new_tokens

    while (true) {
        it_old++;
        it_new++;

        if (it_old == old_size || it_new == new_size) {
            break;
        }

        if (old_tokens[it_old] == star_str) {
            // Found a "*" wildcard.
            it_old++;
            if (it_old == old_size) {
                // The star is at the end, so return true.
                return true;
            }

            string after_star = old_tokens[it_old]; // next string after the *

            // Advance with the new_tokens until after_star is found.
            while (true) {
                if (it_new == new_size) {
                    return false;
                }

                if (new_tokens[it_new] == after_star) {
                    break;
                }

                it_new++;
            }
            continue;
        }

        if (old_tokens[it_old] == plus_str) {
            // Found a "+" wildcard. Jump one comparison.
            continue;
        }

        // Normal case.
        if (old_tokens[it_old] != new_tokens[it_new]) {
            return false;
        }
    }

    // For a match, both iterators should have reached the end.
    if (it_old != old_size || it_new != new_size) {
        return false;
    }
    return true;
}


void Server::printTokens(const string &topic, vector<string> &tokens) {
    cout << "Topic is: " << topic << "\n";
    cout << "Tokens are: ";
    for (auto &tok : tokens) {
        cout << tok << " ";
    }
    cout <<"\n\n";
}
