#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <poll.h>
#include <unistd.h>

#include "protocols.h"


class Server {
 private:
	uint16_t port;

    int udp_sockfd;	// Socket fd to listen for UDP connections.
    int tcp_sockfd;	// Socket fd to listen for TCP connections.

    // Mappings of <id, client> type.
    std::unordered_map<std::string, client*> clients;

    std::vector<pollfd> poll_fds;

 public:

	/**
	 * Constructor.
	 * @param port The port the server program is being run on
	 */
	Server(uint16_t port);


    /**
     * Destructor. Sends signal to all clients to tell them to close.
     * Closes all file descriptors.
     */
    ~Server();


	/**
	 * Returns a new sockaddr_in struct with the basic IPv4 specifications.
	 */
	struct sockaddr_in fill_sockaddr();


    /**
     * Initializes the UDP socket.
     */
	void prepare_udp_socket();


    /**
     * Initializes the TCP socket.
     */
	void prepare_tcp_socket();


    /**
     * Initializes the server.
     */
	void prepare();


    void run();


    bool check_stdin_data();


    void manage_connection_request();
};


#endif /* SERVER_H */
