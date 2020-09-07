#include "video-client.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

#include <nlohmann/json.hpp>

/**
 * List all possible request types from websocket client
 */
enum RequestType {
    RT_UNDEFINED,
    RT_CANVAS_SIZE, // request to resize the rendered frame
    RT_CLIENT_EVENT, // request to handle a user input (keyboard / mouse)
};

RequestType resolveRequestType(const std::string &e) {
    if (e == "canvas_size") return RT_CANVAS_SIZE;
    if (e == "client_event") return RT_CLIENT_EVENT;
    return RT_UNDEFINED;
}

/**
 * List all possible client events that can be received from websocket
 */
enum ClientEvent {
    EV_UNDEFINED,
    EV_WHEEL_UP,
    EV_WHEEL_DOWN,
    EV_LEFT_MOUSE_MOVEMENT,
    EV_RIGHT_MOUSE_MOVEMENT,
};

/**
 * Transforms the string received in json into the client event value
 */
ClientEvent resolveClientEvent(const std::string &e) {
    if (e == "wheel_down") return EV_WHEEL_DOWN;
    if (e == "wheel_up") return EV_WHEEL_UP;
    if (e == "left_mouse_move") return EV_LEFT_MOUSE_MOVEMENT;
    if (e == "right_mouse_move") return EV_RIGHT_MOUSE_MOVEMENT;
    return EV_UNDEFINED;
}

VideoClient::VideoClient(unsigned char * const data_server_ip, int data_server_port) {
    vc_data_client = new DataClient(data_server_ip, data_server_port);
    cl_model = vc_data_client->get_model();
    cl_view = std::make_shared<er_view_t>((er_view_t) ER_VIEW_D);

    // coordinates ~ above geneva
    cl_view->vw_lon = 6.126579;
    cl_view->vw_lat = 46.2050282;
    cl_view->vw_spn = ER_COMMON_USPAN;

    vc_video_engine = new VideoEngine(vc_data_client->get_model(), cl_view);
}

VideoClient::~VideoClient() {
    delete(vc_video_engine);
    delete(vc_data_client);
//    delete(vc_video_streamer);
}

void VideoClient::handle_message(const ix::WebSocketMessagePtr &msg,
                                 std::shared_ptr<ix::ConnectionState> connectionState) {
    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
        try {
            // parse json
            auto j = nlohmann::json::parse(msg.get()->str.data());

            if (j.contains("request")) {
                    RequestType requestType = resolveRequestType(j["request"]);
                    if (requestType == RT_CANVAS_SIZE && j.contains("width") && j.contains("height")) {
                        vc_video_engine->set_size((uint32_t) j["width"], (uint32_t) j["height"]);
                    }
                    // read client events
                    else if (requestType == RT_CLIENT_EVENT && j.contains("client_event")) {
                        auto event_name = (std::string) j["client_event"];
                        auto mods = j["client_event_mods"];
                        auto data = j["client_event_data"];
                        le_real_t dx, dy;

                        // read data values
                        if (data.contains("dx")) dx = (le_real_t) (int) data["dx"];
                        if (data.contains("dy")) dy = (le_real_t) (int) data["dy"];

                        // resolve event
                        ClientEvent event = resolveClientEvent(event_name);
                        switch (event) {
                            case EV_WHEEL_DOWN:
                                // @TODO: take into account modifiers (ctrl or alt) for speed zoom
                                cl_inertia = er_view_get_inertia(&*cl_view, 0);
                                er_view_set_alt(&*cl_view, +cl_inertia);
                                break;

                            case EV_WHEEL_UP:
                                cl_inertia = er_view_get_inertia(&*cl_view, 0);
                                er_view_set_alt(&*cl_view, -cl_inertia);
                                break;

                            case EV_LEFT_MOUSE_MOVEMENT:
                                cl_inertia = er_view_get_inertia(&*cl_view, 0);
                                /* apply inertia */
                                dx *= ER_COMMON_INP * cl_inertia;
                                dy *= ER_COMMON_INP * cl_inertia;
                                /* update view position */
                                er_view_set_plan(&*cl_view, dx, dy);
                                break;

                            case EV_RIGHT_MOUSE_MOVEMENT:
                                /* apply inertia */
                                dx *= ER_COMMON_INR;
                                dy *= ER_COMMON_INR;
                                /* update view direction */
                                er_view_set_azm(&*cl_view, -dx);
                                er_view_set_gam(&*cl_view, +dy);
                                break;

                            default:
                                std::cerr << "Unrecognized client event \"" << event_name << "\"" << std::endl;
                                break;
                        }
                    }
            } else {
                std::cerr << "Received malformed message from web client :\n\t" << j << std::endl;
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
                              char *outputImage = (char *) malloc(vc_video_engine->er_imagedata_size);

                              // render the image and output it to memory
                              VkSubresourceLayout layout;
                              vc_video_engine->draw_frame(outputImage, layout);

                              // encode image for web
                              // @TODO: use a dedicated streaming server with performant codecs to send the frames
                              std::vector<uint8_t> encodedData;
                              stbi_write_jpg_to_func(encode_callback,
                                                     reinterpret_cast<void *>(&encodedData),
                                                     vc_video_engine->get_width(),
                                                     vc_video_engine->get_height(),
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
            while (vc_data_client->update_model(&*cl_view, delay));
        }
    });
    t.detach();
}
