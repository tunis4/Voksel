#include "client/client.hpp"

int main(int argc, char *argv[]) {
    Client *client = new Client();
    client->loop();
    delete client;
    return 0;
}
