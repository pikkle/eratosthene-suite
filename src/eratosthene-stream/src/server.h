#ifndef ERATOSTHENE_STREAM_SERVER_H
#define ERATOSTHENE_STREAM_SERVER_H

#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include "engine.h"

const char* STREAM_ADDRESS = "127.0.0.1";
const int STREAM_PORT = 8080;

void setup_server();
void close_server();

void main_loop(std::shared_ptr<ix::WebSocket> webSocket,
        std::shared_ptr<ix::ConnectionState> connectionState,
        std::shared_ptr<Er_vk_engine> engine);

#endif //ERATOSTHENE_STREAM_SERVER_H
