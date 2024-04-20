#include <iostream>
#include "utils.h"
#include "Server.h"

using namespace std;


int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 2) {
        printf("[SERVER] Usage: ./%s <port>\n", argv[0]);
        return 1;
    }

    // Get port as number.
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Invalid port number.\n");

	Server *server;
	try {
		server = new Server(port);
	} catch (bad_alloc &exception) {
		fprintf(stderr, "Server alloc failed.\n");
		exit(-1);
	}

	server->prepare();


    return 0;
}
