#include "entry-server.h"

#include <vector>
#include <thread>
#include <regex>

#include <unistd.h>

#include <happly/happly.h>

const char * usage_message = "Usage:\n"
                             "  $ eratosthene-stream [PARAMETERS]\n"
                             "Parameters:\n"
                             "  \x1b[1m-h\x1b[0m                   Displays this message\n"
                             "  \x1b[1m-s\x1b[0m STREAM_PORT       The port on which the stream server should run (mandatory)\n"
                             "  \x1b[1m-d\x1b[0m DATA_SERVER_IP    The data server's ip to which the stream server should fetch the data (mandatory)\n"
                             "  \x1b[1m-d\x1b[0m DATA_SERVER_PORT  The data server's port to which the stream server should fetch the data (mandatory)\n";

int main(int argc, char **argv) {
    int args;
    char *_data_server_ip = nullptr;
    int data_server_port = 0;
    int stream_port = 0;
    while ((args = getopt(argc, argv, "s:d:p:h")) != -1) {
        switch (args) {
            case 's':
                stream_port = atoi(optarg);
                break;
            case 'd':
                _data_server_ip = optarg;
                break;
            case 'p':
                data_server_port = atoi(optarg);
                break;
            case 'h':
                std::cout << usage_message << std::endl;
                return 0;
            case '?':
                if (optopt == 's' || optopt == 'd' || optopt == 'p') {
                    std::cerr << "Option " << optopt << " requires an argument" << std::endl;
                } else {
                    std::cerr << "Unknown option -" << optopt << std::endl;
                }
                std::cerr << usage_message << std::endl;
                return -1;
        }
    }

    if (!stream_port || !_data_server_ip || !data_server_port) {
        std::cerr << "Missing arguments." << std::endl;
        std::cerr << usage_message << std::endl;
        exit(-1);
    }

    const auto* data_server_ip = reinterpret_cast<unsigned char *>(_data_server_ip);
    EntryServer::setup_server(stream_port, data_server_ip, data_server_port);
}

/* ----------- Broadcasting methods ----------- */

void EntryServer::setup_server(int server_port, const unsigned char * data_server_ip, int data_server_port) {
    // @TODO: enable websocket deflate per message
    ix::WebSocketServer er_server_ws(server_port, STREAM_ADDRESS);
    std::cout << "Listening on " << server_port << std::endl;

    // setup data server for all clients
    DataClient::set_data_server(data_server_ip, data_server_port);

    // server main loop to allow connections
    er_server_ws.setOnConnectionCallback(
            [&er_server_ws, data_server_ip, data_server_port](std::shared_ptr<ix::WebSocket> webSocket,
                      std::shared_ptr<ix::ConnectionState> connectionState) {
                // @TODO @FUTURE limit the number of concurrent connections depending on GPU hardware

                // create a private client for this new connection
                auto client = std::make_shared<VideoClient>();

                // client renderer in a new thread
                client->rendering_loop(webSocket, connectionState);

                // handle client messages (commands to transform the view)
                webSocket->setOnMessageCallback([connectionState, client](const ix::WebSocketMessagePtr &msg) {
                    client->handle_message(msg, connectionState);
                });
            }
    );
    auto res = er_server_ws.listen();
    if (!res.first) {
        // Error handling
        std::cerr << "ERROR: " << res.second << std::endl;
        exit(1);
    }

// Run the server in the background. Server can be stoped by calling server.stop()
    er_server_ws.start();

// Block until server.stop() is called.
    er_server_ws.wait();
}

/* -------- End of broadcasting methods ------- */

