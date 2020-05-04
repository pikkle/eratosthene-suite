#include "server.h"

#include <seasocks/PrintfLogger.h>

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

void ErStreamRendererHandler::onConnect(seasocks::WebSocket *socket) {
    std::cout << ("New connection opened, creating engine instance...") << std::endl;
    auto *engine = new Er_vk_engine();
    auto connection = new er_connection {socket, engine, true};
    er_open_connections.insert(connection);
    // TODO: init vulkan engine
}

void ErStreamRendererHandler::onData(seasocks::WebSocket *socket, const char *data) {
    Handler::onData(socket, data);
    // TODO: handle received data (controls over the camera and time modification)
}

void ErStreamRendererHandler::onDisconnect(seasocks::WebSocket *socket) {

    auto set_it = er_open_connections.begin();
    while (set_it != er_open_connections.end()) {
        if ((*set_it)->socket == socket) {
            auto connection = er_open_connections.extract(set_it).value();
            connection->close();
        }
    }
}

void er_connection::close() {
    running = false;
    delete(engine);
    socket->close();
}

void close_server() {
    for (auto connection: er_server_handler.er_open_connections) {
        connection->close();
        delete(connection);
    }
    er_server->terminate();
}

void main_loop(er_connection connection) {
    while (connection.running) {
        connection.engine->draw_frame();
    }
}

/* -------- End of broadcasting methods ------- */

