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
    int num_pollfds;

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


    /**
     * Parses input collected from the STDIN socket.
     * @return true if the input is "exit", false otherwise.
     */
    bool check_stdin_data();


    /**
     * Sends response to the client who requested to connect.
     * @param ok_status true to accept connection, false to decline
     * @param client_sockfd Socket to communicate with the requester
     */
    void send_connection_response(bool ok_status, int client_sockfd);


    /**
     * Adds a new pollfd struct to the vector of poll_fds.
     * @param client_sockfd Socket to associate with the new pollfd.
     */
    void add_client_pollfd(int client_sockfd);


    std::pair<std::string, client*> get_client_from_fd(int fd);


    /**
     * Manages new connection requests. Accept if the client had not been
     * previously registered of it it tries to reconnect, decline otherwise.
     */
    void manage_connection_request();


    void manage_subscribe_unsubscribe(int client_fd, tcp_message *msg_req);


    void manage_udp_message(int client_fd, char *buff, char *formatted_msg);


    bool interpret_udp_payload(int data_type, char *udp_payload, char *topic,
                                struct sockaddr_in &udp_client_addr, char *buffer);

    void send_msg_if_subscribed(char *topic, char *formatted_msg);
};


#endif /* SERVER_H */
