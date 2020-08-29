#include "video-client.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <nlohmann/json.hpp>

/**
 * List all possible client events that can be received from websocket
 */
enum ClientEvent {
    EV_UNDEFINED,
    EV_WHEEL_UP,
    EV_WHEEL_DOWN,
};

/**
 * Transforms the string received in json into the client event value
 */
ClientEvent resolveClientEvent(const std::string &e) {
    if (e == "wheel_down") return EV_WHEEL_DOWN;
    if (e == "wheel_up") return EV_WHEEL_UP;
    return EV_UNDEFINED;
}

VideoClient::VideoClient(unsigned char * const data_server_ip, int data_server_port) {
    vc_data_client = new DataClient(data_server_ip, data_server_port);
    cl_model = vc_data_client->get_model();
    cl_view = std::make_shared<er_view_t>((er_view_t) ER_VIEW_D);
    cl_view->vw_spn = ER_COMMON_USPAN;
    vc_video_engine = new VideoEngine(vc_data_client->get_model(), cl_view);
}

VideoClient::~VideoClient() {
    delete(vc_data_client);
    delete(vc_video_engine);
    delete(vc_video_streamer);
}

void VideoClient::handle_message(const ix::WebSocketMessagePtr &msg,
                                 std::shared_ptr<ix::ConnectionState> connectionState) {
    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
        try {
            // parse json
            auto j = nlohmann::json::parse(msg.get()->str.data());

            // read client events
            if (j.contains("client_event")) {
                auto str_event = (std::string) j["client_event"];
                ClientEvent event = resolveClientEvent(str_event);
                switch(event) {
                    case EV_WHEEL_DOWN:
                        // @TODO: take into account modifiers (ctrl or alt) for speed zoom
                        cl_inertia = er_view_get_inertia(&*cl_view, ER_COMMON_KMCTL);
                        er_view_set_alt( &*cl_view, + cl_inertia );
                        break;
                    case EV_WHEEL_UP:
                        cl_inertia = er_view_get_inertia(&*cl_view, ER_COMMON_KMCTL);
                        er_view_set_alt( &*cl_view, - cl_inertia );
                        break;
                    default:
                        std::cerr << "Unrecognized client event \"" << str_event << "\"" << std::endl;
                        break;
                }
            }

            // create transform of the scene to pass to the engine for further frames redraw
            // @TODO update er_client_view depending on the inputs received in the json
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

void VideoClient::loops_render(std::shared_ptr<ix::WebSocket> webSocket,
                               std::shared_ptr<ix::ConnectionState> connectionState) {
    std::thread t([this, webSocket, connectionState]() {
                      er_view_t prev_view = ER_VIEW_C;

                      while (!connectionState->isTerminated()) {
                          // only draw new image if it has been modified since last draw
                          if (!er_view_get_equal(&prev_view, &*cl_view) || cl_model->md_sync == _LE_FALSE) {
                              if (cl_model->md_sync) {
                                  prev_view = *cl_view;
                              }

                              // prepare memory for image
                              char *outputImage = (char *) malloc(VideoEngine::er_imagedata_size);

                              // render the image and output it to memory
                              VkSubresourceLayout layout;
                              vc_video_engine->draw_frame(outputImage, layout);

                              // encode image for web
                              // @TODO: use a dedicated streaming server with performant codecs to send the frames
                              std::vector<uint8_t> encodedData;
                              stbi_write_jpg_to_func(encode_callback, reinterpret_cast<void *>(&encodedData), WIDTH, HEIGHT,
                                                     4, outputImage, 100);
                              auto b64 = base64_encode(encodedData.data(), encodedData.size());
                              auto result = b64.data();

                              // send image data to client
                              webSocket->send(result);

                              // cleanup
                              free(outputImage);
                          }
                      }
                  }
    );
    t.detach();
}

void VideoClient::loops_update(std::shared_ptr<ix::ConnectionState> connectionState, le_size_t delay) {
    std::thread t([this, delay, connectionState]() {
        while (!connectionState->isTerminated()) {
            while (vc_data_client->update_model(&*cl_view, delay)) {
                vc_video_engine->update_internal_data();
            }

//            connectionState->setTerminated();
//            return; // @TODO keep updating
        }
    });
    t.detach();
}
