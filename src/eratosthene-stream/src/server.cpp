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

Vertices debug_vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},

        {{-0.6f, -0.6f, -0.6f}, {0.0f, 0.0f, 1.0f}},
        {{0.6f, 0.6f, 0.6f}, {0.0f, 1.0f, 0.0f}},

        {{0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}},
};

Indices debug_triangles = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
};

Indices debug_lines = {
        8, 9,
};

Indices debug_points = {
        10,
};

Indices empty = {};

const char * usage_message = "Usage:\n"
                             "  $ eratosthene-stream [PARAMETERS]\n"
                             "Parameters:\n"
                             "  \x1b[1m-h\x1b[0m                 Displays this message\n"
                             "  \x1b[1m-s\x1b[0m STREAM_PORT     The port on which the stream server should run (mandatory)\n"
                             "  \x1b[1m-d\x1b[0m DATA_SERVER_IP  The data server to which the stream server should fetch the data (mandatory)\n";

int main(int argc, char **argv) {
    int args;
    char *data_server_ip = nullptr;
    int data_server_port = 0;
    int stream_port = 0;
    while ((args = getopt(argc, argv, "s:d:p:h")) != -1) {
        switch (args) {
            case 's':
                stream_port = atoi(optarg);
                break;
            case 'd':
                data_server_ip = optarg;
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

    if (!stream_port || !data_server_ip || !data_server_port) {
        std::cerr << "Missing arguments." << std::endl;
        std::cerr << usage_message << std::endl;
        exit(-1);
    }

    setup_server(stream_port, data_server_ip, data_server_port);
}
/* -------------- Helper methods -------------- */

void split(const std::string& s, char c, std::vector<std::string>& v) {
    int i = 0;
    int j = s.find(c);

    while (j >= 0) {
        v.push_back(s.substr(i, j-i));
        i = ++j;
        j = s.find(c, j);

        if (j < 0) {
            v.push_back(s.substr(i, s.length()));
        }
    }
}

Vertices load_ply_data(std::string path) {
    std::cout << "Loading ply scene..." << std::endl;
    std::vector<Vertex> vertices;

    happly::PLYData plyIn(path);
    auto vPos = plyIn.getVertexPositions();
    auto vCol = plyIn.getVertexColors();
    assert(vPos.size() == vCol.size());
    auto posFactor = 2.f;
    for (int i = 0; i < vPos.size(); ++i) {
        auto pos = vPos.at(i) ;
        auto col = vCol.at(i);
        vertices.push_back(Vertex{
            {-(float) pos[0] / posFactor, (float) pos[1] / posFactor, 2.f-(float) pos[2] / posFactor},
            {((float) col[0] / 255.f),  ((float) col[1] / 255.f), ((float) col[2] / 255.f)}
        });
    }
    std::cout << "Finished importing " << vPos.size() << " vertices from the .ply file." << std::endl;
    return vertices;
}

void encode_callback(void *context, void *data, int size) {
    auto image = reinterpret_cast<std::vector<uint8_t>*>(context);
    auto encoded = reinterpret_cast<uint8_t*>(data);
    for (int i = 0; i < size; ++i) {
        image->push_back(encoded[i]);
    }
}

/* ---------- End of helper methods ----------- */

/* ----------- Broadcasting methods ----------- */

void setup_server(int server_port, const char * data_server_ip, int data_server_port) {
    // @TODO: enable websocket deflate per message
    ix::WebSocketServer er_server_ws(server_port, STREAM_ADDRESS);
    std::cout << "Listening on " << server_port << std::endl;
    // server main loop to allow connections
    er_server_ws.setOnConnectionCallback(
            [&er_server_ws, data_server_ip, data_server_port](std::shared_ptr<ix::WebSocket> webSocket,
                      std::shared_ptr<ix::ConnectionState> connectionState) {
                // @TODO @FUTURE limit the number of concurrent connections depending on GPU hardware

                // create a private engine for this new connection
                auto engine = std::make_shared<Er_vk_engine>(data_server_ip, data_server_port);

                // client renderer in a new thread
                std::thread t(main_loop, webSocket, connectionState, engine);
                t.detach();

                // handle client messages (commands to transform the view)
                webSocket->setOnMessageCallback([connectionState, engine](const ix::WebSocketMessagePtr &msg) {
                    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
                        try {
                        // parse json
                            auto j = nlohmann::json::parse(msg.get()->str.data());
                            // @TODO check that json is transform-consistent

                            // create transform of the scene to pass to the engine for further frames redraw
//                            Er_transform new_transform = engine->get_transform();
//
//                            new_transform.rotate_x += (float) j["rotate_x"];
//                            new_transform.rotate_y += (float) j["rotate_y"];
//                            new_transform.rotate_z += (float) j["rotate_z"];
//                            new_transform.translate_camera_x += (float) j["translate_camera_x"];
//                            new_transform.translate_camera_y += (float) j["translate_camera_y"];
//                            new_transform.translate_camera_z += (float) j["translate_camera_z"];
//                            new_transform.zoom += (float) j["zoom"];
//                            engine->set_transform(new_transform);
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

void main_loop(std::shared_ptr<ix::WebSocket> webSocket,
               std::shared_ptr<ix::ConnectionState> connectionState,
               std::shared_ptr<Er_vk_engine> engine) {
//    Er_transform last_transform = {.rotate_z =  0.0f};
//    engine->set_transform(last_transform);
    bool drew_once = false;

    while (!connectionState->isTerminated()) {
        // only draw new image if it has been modified since last draw
        if (/*engine->get_transform() != last_transform || */!drew_once) {
            drew_once = true;
//            last_transform = engine->get_transform();
            // prepare memory for image
            VkSubresourceLayout layout;
            char* imagedata = (char*) malloc(Er_vk_engine::er_imagedata_size);

            // render the image and output it to memory
            engine->draw_frame(imagedata, layout);

            // encode image for web
            std::vector<uint8_t> encodedData;
            stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void*>(&encodedData), WIDTH, HEIGHT, 4, imagedata,  30);
//            stbi_write_bmp_to_func(encode_callback, reinterpret_cast<void*>(&encodedData), WIDTH, HEIGHT, 4, imagedata);
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

