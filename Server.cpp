#include "Server.h"
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "utils.h"

#define LISTEN_BACKLOG 50

using namespace std;


Server::Server(uint16_t port) {
	this->port = port;
}


Server::~Server() {
    // TODO: Close all clients.
    cout << "Destructor called\n";
}


struct sockaddr_in Server::fill_sockaddr() {
	struct sockaddr_in new_addr;
	memset(&new_addr, 0, sizeof(struct sockaddr_in));

	new_addr.sin_family = AF_INET;
	new_addr.sin_addr.s_addr = INADDR_ANY;
	new_addr.sin_port = htons(port);

	return new_addr;
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
	struct sockaddr_in udp_addr = fill_sockaddr();

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
	struct sockaddr_in tcp_addr = fill_sockaddr();

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
}


void Server::run() {
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
    }


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
