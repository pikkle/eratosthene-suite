#ifndef ERATOSTHENE_STREAM_SERVER_H
#define ERATOSTHENE_STREAM_SERVER_H

#include <seasocks/Server.h>
#include <seasocks/WebSocket.h>

#include "engine.h"

const int STREAM_PORT = 8080;

struct er_connection {
    seasocks::WebSocket *socket;
    Er_vk_engine *engine;
    bool running;

    void close();
};

struct ErStreamRendererHandler : seasocks::WebSocket::Handler {
    std::set<er_connection*> er_open_connections;
    void onConnect(seasocks::WebSocket *socket) override;
    void onData(seasocks::WebSocket *socket, const char *data) override;
    void onDisconnect(seasocks::WebSocket *socket) override;
};

// Server data
seasocks::Server *er_server;
std::shared_ptr<seasocks::Logger> er_logger;
ErStreamRendererHandler er_server_handler;

void setup_server();
void close_server();
void broadcast_frame();
void main_loop(er_connection connection);

#endif //ERATOSTHENE_STREAM_SERVER_H
