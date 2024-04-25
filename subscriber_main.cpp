#include <iostream>
#include "utils.h"
#include "Subscriber.h"
#include <arpa/inet.h>
#include <cstring>

using namespace std;

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 4) {
        cout << "Subscriber Usage: " << argv[0]
             << " <CLIENT_ID> <SERVER_IP> <SERVER_PORT> \n";
        return -1;
    }

    if (strlen(argv[1]) > 10) {
        cout << "ID must have at most 10 characters\n";
        return -1;
    }

    // Get server_port as number.
    uint16_t server_port;
    int rc = sscanf(argv[3], "%hu", &server_port);
    DIE(rc != 1, "Invalid port number.\n");

    // Get server_id as number in network order.
    uint32_t server_ip = inet_addr(argv[2]);

    Subscriber *subscriber;
    try {
        subscriber = new Subscriber(argv[1], server_ip, server_port);
    } catch (bad_alloc &exception) {
        fprintf(stderr, "Subscriber alloc failed.\n");
        exit(-1);
    }

    subscriber->prepare();

    if (subscriber->check_connection_validity()) {
        subscriber->run();
    }

    delete subscriber;

    return 0;
}
