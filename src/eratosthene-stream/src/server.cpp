#include "server.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <vector>
#include <thread>

#include <unistd.h>

#include <nlohmann/json.hpp>

int main() {
    setup_server();
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

void setup_server() {
    // TODO: enable websocket deflate per message
    ix::WebSocketServer er_server_ws(STREAM_PORT, STREAM_ADDRESS);
    er_server_ws.setOnConnectionCallback(
            [&er_server_ws](std::shared_ptr<ix::WebSocket> webSocket,
                      std::shared_ptr<ix::ConnectionState> connectionState) {

                std::cerr << "New connection" << std::endl;
                auto engine = std::make_shared<Er_vk_engine>();
                std::thread t(main_loop, webSocket, connectionState, engine);
                t.detach();
                webSocket->setOnMessageCallback([connectionState, engine](const ix::WebSocketMessagePtr &msg) {
                    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
                        std::cout << "Received message : " << msg->str << std::endl;
                        // TODO: parse json
                        // TODO: create transform of the scene to pass to the engine for further frames redraw
                        // TODO: call engine->newTransform() or something
                    }
                });
            }
    );
    auto res = er_server_ws.listen();
    if (!res.first) {
        // Error handling
        std::cerr << res.second << std::endl;
        exit(1);
    }

// Run the server in the background. Server can be stoped by calling server.stop()
    er_server_ws.start();

// Block until server.stop() is called.
    er_server_ws.wait();
}


/*
void ErStreamRendererHandler::onConnect(seasocks::WebSocket *socket) {
    er_logger->info("New connection opened, creating engine instance...");
    auto *engine = new Er_vk_engine();
    er_connection connection  {engine, true};
    er_open_connections[socket] = &connection;

    std::thread t(main_loop, socket, &connection);
    t.detach();
}
float angle = 0;
void ErStreamRendererHandler::onData(seasocks::WebSocket *socket, const char *data) {
    Handler::onData(socket, data);
    auto j = nlohmann::json::parse(data);
    for ( auto const& [s, connection] : er_open_connections ) {
        if (s == socket) {
            connection->angle += j["camera_rotate"].get<float>();
            break;
        }
    }
}

void ErStreamRendererHandler::onDisconnect(seasocks::WebSocket *socket) {
    for ( auto const& [s, connection] : er_open_connections ) {
        if (s == socket) {
            connection->close();
            er_server_handler.er_open_connections.erase(s);
            er_logger->info("Closed client connection (id : %x)", connection);
            break;
        }
    }
}

void er_connection::close() {
    er_logger->debug("Closing engine");
    running = false;
}

void close_server() {
    for (auto & er_open_connection : er_server_handler.er_open_connections) {
        er_open_connection.second->close();
        er_open_connection.first->close();
    }
    er_server->terminate();
}

void main_loop(seasocks::WebSocket *socket, er_connection *connection) {
    while (connection->running) {
        VkSubresourceLayout layout;
        char* imagedata = (char*) malloc(Er_vk_engine::er_imagedata_size);
        printf("angle : %f\n", connection->angle);
        connection->engine->draw_frame(connection->angle, imagedata, layout);

        std::vector<uint8_t> encodedData;
        stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void*>(&encodedData), WIDTH, HEIGHT, 4, imagedata, 70);
        auto b64 = base64_encode(encodedData.data(), encodedData.size());
        auto result = b64.data();
        er_server->execute([socket, &connection, result]{
            if (connection->running)
                socket->send(result);
        });
//        free(imagedata);
    }
    er_logger->debug("Terminated rendering loop");

}
 */

/* -------- End of broadcasting methods ------- */

