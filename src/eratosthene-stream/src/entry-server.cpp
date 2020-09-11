#include "entry-server.h"

#include <memory>

#include <unistd.h>

#include <happly/happly.h>

const char * usage_message = "Usage:\n"
                             "  $ eratosthene-stream [PARAMETERS]\n"
                             "Parameters:\n"
                             "  \x1b[1m-h\x1b[0m                   Displays this message\n"
                             "  \x1b[1m-s\x1b[0m STREAM_PORT       The port on which the stream server should run (mandatory)\n"
                             "  \x1b[1m-d\x1b[0m DATA_SERVER_IP    The data server's ip to which the stream server should fetch the data (mandatory)\n"
                             "  \x1b[1m-p\x1b[0m DATA_SERVER_PORT  The data server's port to which the stream server should fetch the data (mandatory)\n"
                             "  \x1b[1m-x\x1b[0m VIEW_LAT          The default position's latitude of the camera (optional)\n"
                             "  \x1b[1m-y\x1b[0m VIEW_LON          The default position's longitude of the camera (optional)\n"
                             "  \x1b[1m-a\x1b[0m VIEW_TIA          The default position's time A of the camera (optional)\n"
                             "  \x1b[1m-b\x1b[0m VIEW_TIB          The default position's time B of the camera (optional)\n";

int main(int argc, char **argv) {
    int args;
    char *_data_server_ip = nullptr;
    int data_server_port = 0;
    int stream_port = 0;
    double view_lat = DEFAULT_LAT;
    double view_lon = DEFAULT_LON;
    int view_tia = DEFAULT_TIA;
    int view_tib = DEFAULT_TIB;
    while ((args = getopt(argc, argv, "s:d:p:h:x:y:a:b")) != -1) {
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
            case 'x':
                view_lat = atof(optarg);
                break;
            case 'y':
                view_lon = atof(optarg);
                break;
            case 'a':
                view_tia = atoi(optarg);
                break;
            case 'b':
                view_tib = atoi(optarg);
                break;
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

    auto* data_server_ip = reinterpret_cast<unsigned char *>(_data_server_ip);
    EntryServer::setup_server(stream_port, data_server_ip, data_server_port, view_lat, view_lon, view_tia, view_tib);
}

/* ----------- Broadcasting methods ----------- */

void EntryServer::setup_server(int server_port, unsigned char * data_server_ip, int data_server_port,
                               double view_lat, double view_lon, int view_tia, int view_tib) {
    ix::WebSocketServer er_server_ws(server_port, STREAM_ADDRESS);
    std::cout << "Listening on " << server_port << std::endl;

    // server main loop to allow connections
    er_server_ws.setOnConnectionCallback(
            [&er_server_ws, data_server_ip, data_server_port, view_lat, view_lon, view_tia, view_tib]
            (std::shared_ptr<ix::WebSocket> webSocket, std::shared_ptr<ix::ConnectionState> connectionState) {
                // create a private client for this new connection
                auto client = std::make_shared<VideoClient>(data_server_ip, data_server_port, view_lat, view_lon, view_tia, view_tib);

                // client renderer in a new thread
                client->loops_render(webSocket, connectionState);

                // client data fetch in a new thread
                client->loops_update(connectionState);

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

