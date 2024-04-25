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


    /**
     * Returns a new sockaddr_in struct with the basic IPv4 specifications.
     */
    struct sockaddr_in fill_sockaddr();


    /**
     * Sets up the UDP socket.
     */
    void prepare_udp_socket();


    /**
     * Sets up the TCP socket for listening for new connections.
     */
	void prepare_tcp_socket();


    /**
     * Parses input collected from the STDIN socket.
     * @return true if the input is "exit", false otherwise.
     */
    bool check_stdin_data();


    /**
     * Sends accept/deny message to the client who requested to connect.
     * @param response true to accept connection, false to decline
     * @param client_sockfd Socket to communicate with the requester
     */
    void send_connection_response(bool response, int client_sockfd);


    /**
     * Creates and adds a new pollfd struct to the vector of poll_fds.
     * @param client_sockfd Socket to associate with the new pollfd.
     */
    void add_client_pollfd(int client_sockfd);


    /**
     * Manages new connection request from a TCP client. Accepts if
     * the client had not been previously registered or if he tries
     * to reconnect, declines otherwise.
     */
    void manage_connection_request();


    /**
     * Traverses the client database and returns the <id, client> map
     * entry pair for which the client has the requested fd (or a
     * placeholder if no such entry exists).
     */
    std::pair<std::string, client*> get_client_from_fd(int fd);


    /**
     * Checks if the client corresponding to client_fd can perform the
     * requested subscribe/unsubscribe operation and sends success/failure
     * message. If possible, executes the requested operation.
     */
    void manage_subscribe_unsubscribe(int client_fd, tcp_message *req_msg);


    /**
     * Receives a message from a UDP client.
     * Stores the sender details and the topic in the formatted_msg, then
     * calls the interpret() method, to complete the rest of the message.
     * If the message turns out to be valid, calls send_msg_if_subscribed()
     * to send it to all the clients that are subscribed to the received topic.
     *
     * @param client_fd fd of the UDP client that sends the message
     * @param buff Destination buffer for the received message
     * @param formatted_msg Buffer to store the formatted message that would be
     * sent to the TCP clients
     */
    void manage_udp_message(int client_fd, char *buff, char *formatted_msg);


    /**
     * Interprets the message received from the UDP client, based on a
     * predefined protocol. Appends it to formatted_msg.
     *
     * @param data_type Flag that announces the type of the payload data
     * @param udp_payload The actual data received from the UDP client
     * @param buffer Start of the memory zone of the formatted message
     * where the actual message should be appended (before it, metadata
     * was written).
     * @return true if the message from UDP is in valid format, false otherwise
     */
    bool interpret_udp_payload(int data_type, char *udp_payload, char *formatted_msg);


    /**
     * Iterates over the client database and sends the message to all
     * the clients that are subscribed to the topic, also considering
     * the wildcards.
     */
    void send_msg_if_subscribed(char *topic, char *formatted_msg);


    /**
     * Tokenizes the topic using '/' as delimiter and places the
     * tokens into the tokens vector.
     */
    void tokenize_topic(std::string &topic, std::vector<std::string> &tokens);


    /**
     * Iterates over the client's topics, trying to match the given topic's
     * tokens to one of the subscribed topics' tokens.
     * @param topic Topic to search if the client is subscribed to
     * @param subscr_topics Topics the client is subscribed to.
     * @return true, if the client is subscribed to the topic, false otherwise
     */
    bool check_wildcard_topic(std::string &topic,
                              std::map<std::string, std::vector<std::string>> &subscr_topics);


    /**
     * Wildcard topic matching algorithm.
     * Executes the iteration over two vectors of string tokens,
     * determining if they match (considering the wildcards).
     * @param old_tokens Tokens from a topic the client is subscribed to
     * (might contain wildcards)
     * @param new_tokens Tokens of a queried topic
     * @param star_str The string "*"
     * @param plus_str The string "+"
     * @return true, if the two vectors match, false, otherwise
     */
    bool compare_token_vectors(std::vector<std::string> &old_tokens,
                               std::vector<std::string> &new_tokens,
                               std::string &star_str, std::string &plus_str);


 public:

    /**
     * Constructor.
     * @param port The port the server program is being run on
     */
    Server(uint16_t port);


    /**
     * Destructor. Sends signal to all clients to tell them to close.
     * Closes all file descriptors and deletes the client structures.
     */
    ~Server();


    /**
     * Initializes the server's TCP and UDP sockets and the pollfd
     * structures for them and for stdin.
     */
    void prepare();


    /**
     * The main control function for the server. It calls poll() on the
     * poll_fds vector and manages all the possible events that can occur
     * on any socket_fd (messages from stdin, messages from UDP clients,
     * connection requests from TCP clients, messages from TCP clients).
     */
    void run();
};


#endif /* SERVER_H */
