#ifndef ERATOSTHENE_STREAM_SERVER_H
#define ERATOSTHENE_STREAM_SERVER_H

#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include "engine.h"

namespace StreamServer {

    const char* STREAM_ADDRESS = "127.0.0.1";

    void setup_server(int server_port, const unsigned char * data_server_ip, int data_server_port);
    void close_server();
    Vertices load_ply_data(std::string path);

    void main_loop(std::shared_ptr<ix::WebSocket> webSocket,
            std::shared_ptr<ix::ConnectionState> connectionState,
            std::shared_ptr<StreamEngine> engine);

}
#endif //ERATOSTHENE_STREAM_SERVER_H
