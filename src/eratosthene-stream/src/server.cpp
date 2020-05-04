#include "server.h"

#include <seasocks/PrintfLogger.h>

int main() {
    setup_server();
}

const char* sampleImage = "R0lGODlhPQBEAPeoAJosM//AwO/AwHVYZ/z595kzAP/s7P+goOXMv8+fhw/v739/f+8PD98fH/8mJl+fn/9ZWb8/PzWl"
                          "wv///6wWGbImAPgTEMImIN9gUFCEm/gDALULDN8PAD6atYdCTX9gUNKlj8wZAKUsAOzZz+UMAOsJAP/Z2ccMDA8PD/95"
                          "eX5NWvsJCOVNQPtfX/8zM8+QePLl38MGBr8JCP+zs9myn/8GBqwpAP/GxgwJCPny78lzYLgjAJ8vAP9fX/+MjMUcAN8z"
                          "M/9wcM8ZGcATEL+QePdZWf/29uc/P9cmJu9MTDImIN+/r7+/vz8/P8VNQGNugV8AAF9fX8swMNgTAFlDOICAgPNSUnNW"
                          "SMQ5MBAQEJE3QPIGAM9AQMqGcG9vb6MhJsEdGM8vLx8fH98AANIWAMuQeL8fABkTEPPQ0OM5OSYdGFl5jo+Pj/+pqcsT"
                          "E78wMFNGQLYmID4dGPvd3UBAQJmTkP+8vH9QUK+vr8ZWSHpzcJMmILdwcLOGcHRQUHxwcK9PT9DQ0O/v70w5MLypoG8w"
                          "KOuwsP/g4P/Q0IcwKEswKMl8aJ9fX2xjdOtGRs/Pz+Dg4GImIP8gIH0sKEAwKKmTiKZ8aB/f39Wsl+LFt8dgUE9PT5x5"
                          "aHBwcP+AgP+WltdgYMyZfyywz78AAAAAAAD///8AAP9mZv///wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                          "AAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAEAAKgALAAAAAA9AEQAAAj/AFEJHEiwoMGDCBMqXMiwocAbBww4nEhxoYkU"
                          "pzJGrMixogkfGUNqlNixJEIDB0SqHGmyJSojM1bKZOmyop0gM3Oe2liTISKMOoPy7GnwY9CjIYcSRYm0aVKSLmE6nfq0"
                          "5QycVLPuhDrxBlCtYJUqNAq2bNWEBj6ZXRuyxZyDRtqwnXvkhACDV+euTeJm1Ki7A73qNWtFiF+/gA95Gly2CJLDhwEH"
                          "MOUAAuOpLYDEgBxZ4GRTlC1fDnpkM+fOqD6DDj1aZpITp0dtGCDhr+fVuCu3zlg49ijaokTZTo27uG7Gjn2P+hI8+PDP"
                          "ERoUB318bWbfAJ5sUNFcuGRTYUqV/3ogfXp1rWlMc6awJjiAAd2fm4ogXjz56aypOoIde4OE5u/F9x199dlXnnGiHZWE"
                          "YbGpsAEA3QXYnHwEFliKAgswgJ8LPeiUXGwedCAKABACCN+EA1pYIIYaFlcDhytd51sGAJbo3onOpajiihlO92KHGaUX"
                          "GwWjUBChjSPiWJuOO/LYIm4v1tXfE6J4gCSJEZ7YgRYUNrkji9P55sF/ogxw5ZkSqIDaZBV6aSGYq/lGZplndkckZ98x"
                          "oICbTcIJGQAZcNmdmUc210hs35nCyJ58fgmIKX5RQGOZowxaZwYA+JaoKQwswGijBV4C6SiTUmpphMspJx9unX4Kaimj"
                          "Dv9aaXOEBteBqmuuxgEHoLX6Kqx+yXqqBANsgCtit4FWQAEkrNbpq7HSOmtwag5w57GrmlJBASEU18ADjUYb3ADTinIt"
                          "tsgSB1oJFfA63bduimuqKB1keqwUhoCSK374wbujvOSu4QG6UvxBRydcpKsav++Ca6G8A6Pr1x2kVMyHwsVxUALDq/kr"
                          "nrhPSOzXG1lUTIoffqGR7Goi2MAxbv6O2kEG56I7CSlRsEFKFVyovDJoIRTg7sugNRDGqCJzJgcKE0ywc0ELm6KBCCJo"
                          "8DIPFeCWNGcyqNFE06ToAfV0HBRgxsvLThHn1oddQMrXj5DyAQgjEHSAJMWZwS3HPxT/QMbabI/iBCliMLEJKX2EEkom"
                          "BAUCxRi42VDADxyTYDVogV+wSChqmKxEKCDAYFDFj4OmwbY7bDGdBhtrnTQYOigeChUmc1K3QTnAUfEgGFgAWt88hKA6"
                          "aCRIXhxnQ1yg3BCayK44EWdkUQcBByEQChFXfCB776aQsG0BIlQgQgE8qO26X1h8cEUep8ngRBnOy74E9QgRgEAC8SvO"
                          "fQkh7FDBDmS43PmGoIiKUUEGkMEC/PJHgxw0xH74yx/3XnaYRJgMB8obxQW6kL9QYEJ0FIFgByfIL7/IQAlvQwEpnAC7"
                          "DtLNJCKUoO/w45c44GwCXiAFB/OXAATQryUxdN4LfFiwgjCNYg+kYMIEFkCKDs6PKAIJouyGWMS1FSKJOMRB/BoIxYJI"
                          "UXFUxNwoIkEKPAgCBZSQHQ1A2EWDfDEUVLyADj5AChSIQW6gu10bE/JG2VnCZGfo4R4d0sdQoBAHhPjhIB94v/wRoRKQ"
                          "WGRHgrhGSQJxCS+0pCZbEhAAOw==";

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

    size_t size = Er_vk_engine::er_imagedata_size + 1;
    char* imagedata = (char*) malloc(size);
    engine->draw_frame(imagedata);
    imagedata[size-1] = '\0';
    int count = 0;
    for (int i = 0; i < Er_vk_engine::er_imagedata_size; ++i) {
        if ((int) imagedata[i] != 0)
        count++;
    }
    std::cout << count << " pixel values" << std::endl;
    socket->send(sampleImage);
    free(imagedata);
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
//        connection.engine->draw_frame();
    }
}

/* -------- End of broadcasting methods ------- */

