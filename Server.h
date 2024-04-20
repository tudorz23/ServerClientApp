#ifndef SERVER_H
#define SERVER_H

#include <cstdint>


class Server {
 private:
	uint16_t port;

    int udp_sockfd;	// Socket fd to listen for UDP connections.
    int tcp_sockfd;	// Socket fd to listen for TCP connections.



 public:

	/**
	 * Constructor.
	 * @param port port the server program is being run on
	 */
	Server(uint16_t port);


	/**
	 * Returns a new sockaddr_in struct with the basic IPv4 specifications.
	 */
	struct sockaddr_in fill_sockaddr();


	void prepare_udp_socket();

	void prepare_tcp_socket();

	void prepare();
};


#endif /* SERVER_H */
