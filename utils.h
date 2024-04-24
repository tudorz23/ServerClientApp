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


#endif /* UTILS_H */
