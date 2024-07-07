#include "game.hpp"

#include <cstring>
#include <entt/entt.hpp>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        auto *game = new Game(false);
        game->render();
        game->exit_singleplayer();
        delete game;
        return 0;
    } else if (argc == 2) {
        if (std::strcmp(argv[1], "help") == 0) {
            log(LogLevel::INFO, "main", "Usage: {} [SUBCOMMAND]", argv[0]);
            log(LogLevel::INFO, "main", "Possible subcommands:");
            log(LogLevel::INFO, "main", "  help                     Prints this text");
            log(LogLevel::INFO, "main", "  singleplayer             Directly enters a new singleplayer world");
            log(LogLevel::INFO, "main", "  server                   Hosts a server");
            log(LogLevel::INFO, "main", "  connect 127.0.0.1:12345  Connects to a server");
            return 0;
        } else if (std::strcmp(argv[1], "singleplayer") == 0) {
            auto *game = new Game(true);
            game->render();
            game->exit_singleplayer();
            delete game;
            return 0;
        } else if (std::strcmp(argv[1], "server") == 0) {
            log(LogLevel::INFO, "main", "server");
            // auto *world = new World();
            // auto *server = new Server(false, world);
            // server->heartbeat();
            // delete server;
            // delete world;
            return 0;
        }
    } else if (argc == 3) {
        if (std::strcmp(argv[1], "connect") == 0) {
            log(LogLevel::INFO, "main", "connecting to {}", argv[2]);
            return 0;
        }
    }

    log(LogLevel::ERROR, "main", "Invalid arguments, try \"help\" for more information.");
    return 1;
}
