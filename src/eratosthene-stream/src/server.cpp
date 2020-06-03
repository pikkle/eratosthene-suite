#include "server.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <vector>
#include <thread>

#include <unistd.h>

#include <nlohmann/json.hpp>

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

int main(int argc, char **argv) {
    if (argc == 1) {
        setup_server(debug_vertices, debug_triangles, debug_lines, debug_points);
    } else if (argc == 2) {

    } else {
        printf("Program usage:\n\t > eratosthene-stream [\"path/to/plyfile\"]\n");
        printf("If no ply file is given as an argument, the application will run with debug data to display on the application.\n");
        exit(-1);
    }
}

/* -------------- Helper methods -------------- */

void load_ply_data(const char* path, Vertices &v) {

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

void setup_server(Vertices v, Indices t, Indices l, Indices p) {
    // @TODO: enable websocket deflate per message
    ix::WebSocketServer er_server_ws(STREAM_PORT, STREAM_ADDRESS);

    // server main loop to allow connections
    er_server_ws.setOnConnectionCallback(
            [&er_server_ws, &v, &t, &l, &p](std::shared_ptr<ix::WebSocket> webSocket,
                      std::shared_ptr<ix::ConnectionState> connectionState) {
                // @TODO @FUTURE limit the number of concurrent connections depending on GPU hardware

                // create a private engine for this new connection
                auto engine = std::make_shared<Er_vk_engine>(v, t, l, p);

                // client renderer in a new thread
                std::thread t(main_loop, webSocket, connectionState, engine);
                t.detach();

                // handle client messages (commands to transform the view)
                webSocket->setOnMessageCallback([connectionState, engine](const ix::WebSocketMessagePtr &msg) {
                    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
                        // parse json
                        auto j = nlohmann::json::parse(msg.get()->str.data());
                        // @TODO check that json is transform-consistent

                        // create transform of the scene to pass to the engine for further frames redraw
                        Er_transform new_transform = engine->get_transform();
                        new_transform.rotate_x = new_transform.rotate_x + (float) j["rotate_x"];
                        new_transform.rotate_y = new_transform.rotate_y + (float) j["rotate_y"];
                        new_transform.rotate_z = new_transform.rotate_z + (float) j["rotate_z"];
                        engine->set_transform(new_transform);
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
    Er_transform last_transform = {.rotate_z =  0.0f};
    engine->set_transform(last_transform);
    bool drew_once = false;

    while (!connectionState->isTerminated()) {
        // only draw new image if it has been modified since last draw
        if (engine->get_transform() != last_transform || !drew_once) {
            drew_once = true;
            last_transform = engine->get_transform();
            // prepare memory for image
            VkSubresourceLayout layout;
            char* imagedata = (char*) malloc(Er_vk_engine::er_imagedata_size);

            // render the image and output it to memory
            engine->draw_frame(imagedata, layout);

            // encode image for web
            std::vector<uint8_t> encodedData;
            stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void*>(&encodedData), WIDTH, HEIGHT, 4, imagedata, 100);
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

