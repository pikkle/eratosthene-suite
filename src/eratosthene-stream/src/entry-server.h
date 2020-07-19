#ifndef ERATOSTHENE_STREAM_ENTRY_SERVER_H
#define ERATOSTHENE_STREAM_ENTRY_SERVER_H

#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include "video-client.h"
#include "data-client.h"

namespace EntryServer {

    const char* STREAM_ADDRESS = "127.0.0.1";

    void setup_server(int server_port, unsigned char * data_server_ip, int data_server_port);
    void close_server();
    Vertices load_ply_data(std::string path);

}
#endif //ERATOSTHENE_STREAM_ENTRY_SERVER_H
