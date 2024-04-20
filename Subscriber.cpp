#include "Subscriber.h"


using namespace std;

Subscriber::Subscriber(string id, uint32_t server_ip, uint16_t server_port) {
    this->id = id;
    this->server_ip = server_ip;
    this->server_port = server_port;
}
