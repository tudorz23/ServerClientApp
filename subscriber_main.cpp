#include <iostream>
#include "utils.h"
#include "Subscriber.h"
#include <arpa/inet.h>

using namespace std;

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 4) {
        cout << "[SUBSCRIBER] Usage: " << argv[0];
        cout << " <CLIENT_ID> <SERVER_IP> <SERVER_PORT> \n";
        return 1;
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

    subscriber->check_validity();

    return 0;
}
