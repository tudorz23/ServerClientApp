#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <cstdlib>


/*
 * Error checking macro.
 */
#define DIE(assertion, call_description)                                       \
	do {                                                                       \
		if (assertion) {                                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
			perror(call_description);                                          \
			exit(EXIT_FAILURE);                                                \
		}                                                                      \
	} while (0)


/**
 * Returns a new sockaddr_in struct with the basic IPv4 specifications.
 * @param port The port to set to sin_port.
 */
struct sockaddr_in fill_sockaddr(uint16_t port);


#endif /* UTILS_H */
