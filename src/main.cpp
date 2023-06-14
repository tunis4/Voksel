#include "client/client.hpp"

int main(int argc, char *argv[]) {
    auto client = new client::Client();
    client->loop();
    delete client;
    return 0;
}
