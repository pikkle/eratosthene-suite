#include "server.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <vector>
#include <thread>
#include <regex>

#include <unistd.h>

#include <nlohmann/json.hpp>
#include <happly/happly.h>

const char * usage_message = "Usage:\n"
                             "  $ eratosthene-stream [PARAMETERS]\n"
                             "Parameters:\n"
                             "  \x1b[1m-h\x1b[0m                 Displays this message\n"
                             "  \x1b[1m-s\x1b[0m STREAM_PORT     The port on which the stream server should run (mandatory)\n"
                             "  \x1b[1m-d\x1b[0m DATA_SERVER_IP  The data server to which the stream server should fetch the data (mandatory)\n";

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
    StreamServer::setup_server(stream_port, data_server_ip, data_server_port);
}
/* -------------- Helper methods -------------- */

void encode_callback(void *context, void *data, int size) {
    auto image = reinterpret_cast<std::vector<uint8_t>*>(context);
    auto encoded = reinterpret_cast<uint8_t*>(data);
    for (int i = 0; i < size; ++i) {
        image->push_back(encoded[i]);
    }
}

/* ---------- End of helper methods ----------- */

/* ----------- Broadcasting methods ----------- */

void StreamServer::setup_server(int server_port, const unsigned char * data_server_ip, int data_server_port) {
    // @TODO: enable websocket deflate per message
    ix::WebSocketServer er_server_ws(server_port, STREAM_ADDRESS);
    std::cout << "Listening on " << server_port << std::endl;
    // server main loop to allow connections
    er_server_ws.setOnConnectionCallback(
            [&er_server_ws, data_server_ip, data_server_port](std::shared_ptr<ix::WebSocket> webSocket,
                      std::shared_ptr<ix::ConnectionState> connectionState) {
                // @TODO @FUTURE limit the number of concurrent connections depending on GPU hardware

                // create a private engine for this new connection
                auto engine = std::make_shared<StreamEngine>(data_server_ip, data_server_port);

                // client renderer in a new thread
                std::thread t(main_loop, webSocket, connectionState, engine);
                t.detach();

                // handle client messages (commands to transform the view)
                webSocket->setOnMessageCallback([connectionState, engine](const ix::WebSocketMessagePtr &msg) {
                    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
                        try {
                        // parse json
                            auto j = nlohmann::json::parse(msg.get()->str.data());
                            // @TODO check that json is consistent on what is expected

                            // create transform of the scene to pass to the engine for further frames redraw
                            // @TODO update client_view depending on the inputs received in the json
                            // Ex:
                            // float rotation = (float) j["rotate_x"];
                        } catch (std::exception &e) {
                            std::cerr << "Got a malformed json object :" << std::endl << msg.get()->str << std::endl;
                        }
                    }
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

void StreamServer::main_loop(std::shared_ptr<ix::WebSocket> webSocket,
               std::shared_ptr<ix::ConnectionState> connectionState,
               std::shared_ptr<StreamEngine> engine) {
    bool drew_once = false;

    while (!connectionState->isTerminated()) {
        // only draw new image if it has been modified since last draw
        if (!drew_once) { // @TODO: check if the client_view has been updated
            drew_once = true;

            // prepare memory for image
            VkSubresourceLayout layout;
            char* imagedata = (char*) malloc(StreamEngine::er_imagedata_size);

            // render the image and output it to memory
            engine->draw_frame(imagedata, layout);

            // encode image for web
            // @TODO: use a dedicated streaming server with performant codecs to send the frames
            std::vector<uint8_t> encodedData;
            stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void*>(&encodedData), WIDTH, HEIGHT, 4, imagedata,  30);
            auto b64 = base64_encode(encodedData.data(), encodedData.size());
            auto result = b64.data();

            // send image data to client
            webSocket->send(result);

            // cleanup
            free(imagedata);
        } else {
            usleep(1000);
        }
    }
}

/* -------- End of broadcasting methods ------- */

