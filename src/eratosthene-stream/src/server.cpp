#include "server.h"

#include <seasocks/PrintfLogger.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <vector>

int main() {
    setup_server();
}

/* ----------- Broadcasting methods ----------- */

void setup_server() {
    er_logger = std::make_shared<seasocks::PrintfLogger>();
    er_server = new seasocks::Server(er_logger);
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
    std::cout << ("New connection opened, creating engine instance...") << std::endl;
    auto *engine = new Er_vk_engine();
    auto connection = new er_connection {socket, engine, true};
    er_open_connections.insert(connection);

    main_loop(*connection);
}

void ErStreamRendererHandler::onData(seasocks::WebSocket *socket, const char *data) {
    Handler::onData(socket, data);
    std::cout << "Message received from client: " << std::endl << data << std::endl;
    // TODO: handle received data (controls over the camera and time modification)
}

void ErStreamRendererHandler::onDisconnect(seasocks::WebSocket *socket) {
//    auto set_it = er_open_connections.begin();
//    while (set_it != er_open_connections.end()) {
//        if ((*set_it)->socket == socket) {
//            auto connection = er_open_connections.extract(set_it).value();
//            connection->close();
//        }
//    }
}

void er_connection::close() {
    running = false;
    delete(engine);
    socket->close();
}

void close_server() {
    for (auto connection: er_server_handler.er_open_connections) {
        connection->close();
    }
    er_server->terminate();
}

void main_loop(er_connection connection) {
    while (connection.running) {
        VkSubresourceLayout layout;
        char* imagedata = (char*) malloc(Er_vk_engine::er_imagedata_size);
        connection.engine->draw_frame(imagedata, layout);

        std::vector<uint8_t> encodedData;
        stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void*>(&encodedData), WIDTH, HEIGHT, 4, imagedata, 50);
        auto b64 = base64_encode(encodedData.data(), encodedData.size());
        connection.socket->send(b64.data());
        free(imagedata);
    }
}

/* -------- End of broadcasting methods ------- */

