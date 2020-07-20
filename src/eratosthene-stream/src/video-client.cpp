#include "video-client.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <nlohmann/json.hpp>

VideoClient::VideoClient(unsigned char * const data_server_ip, int data_server_port) :
        vc_data_client(data_server_ip, data_server_port) {
    cl_model = vc_data_client.get_model();
}

VideoClient::~VideoClient() {
}

void VideoClient::handle_message(const ix::WebSocketMessagePtr &msg,
                                 std::shared_ptr<ix::ConnectionState> connectionState) {
    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
        try {
            // parse json
            auto j = nlohmann::json::parse(msg.get()->str.data());
            // @TODO check that json is consistent on what is expected

            // create transform of the scene to pass to the engine for further frames redraw
            // @TODO update er_client_view depending on the inputs received in the json
            // Ex:
            // float rotation = (float) j["rotate_x"];
        } catch (std::exception &e) {
            std::cerr << "Got a malformed json object :" << std::endl << msg.get()->str << std::endl;
        }
    }
}

void encode_callback(void *context, void *data, int size) {
    auto image = reinterpret_cast<std::vector<uint8_t>*>(context);
    auto encoded = reinterpret_cast<uint8_t*>(data);
    for (int i = 0; i < size; ++i) {
        image->push_back(encoded[i]);
    }
}

void VideoClient::rendering_loop(std::shared_ptr<ix::WebSocket> webSocket,
                                 std::shared_ptr<ix::ConnectionState> connectionState) {
    std::thread t([this, webSocket, connectionState]() {
                      bool drew_once = false;

                      while (!connectionState->isTerminated()) {
                          // only draw new image if it has been modified since last draw
                          if (!drew_once) { // @TODO: check if the client_view has been updated
                              drew_once = true;

                              // prepare memory for image
                              char *outputImage = (char *) malloc(VideoEngine::er_imagedata_size);

                              // render the image and output it to memory
                              VkSubresourceLayout layout;
                              vc_video_engine.draw_frame(outputImage, layout);

                              // encode image for web
                              // @TODO: use a dedicated streaming server with performant codecs to send the frames
                              std::vector<uint8_t> encodedData;
                              stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void *>(&encodedData), WIDTH, HEIGHT,
                                                     4, outputImage, 30);
                              auto b64 = base64_encode(encodedData.data(), encodedData.size());
                              auto result = b64.data();

                              // send image data to client
                              webSocket->send(result);

                              // cleanup
                              free(outputImage);
                          } else {
                              usleep(1000);
                          }
                      }
                  }
    );
    t.detach();
}

