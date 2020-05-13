#include "server.h"

#include <seasocks/PrintfLogger.h>
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

/* ----------- Broadcasting methods ----------- */

void setup_server() {
    er_logger = std::make_shared<seasocks::PrintfLogger>();
    er_server = new seasocks::Server(er_logger);
    er_server->setPerMessageDeflateEnabled(true);
    er_server->addWebSocketHandler("/stream", std::make_shared<ErStreamRendererHandler>(er_server_handler));
    er_server->serve("web", STREAM_PORT);
}

void encode_callback(void *context, void *data, int size) {
    auto image = reinterpret_cast<std::vector<uint8_t>*>(context);
    auto encoded = reinterpret_cast<uint8_t*>(data);
    for (int i = 0; i < size; ++i) {
        image->push_back(encoded[i]);
    }
}

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

/* -------- End of broadcasting methods ------- */

