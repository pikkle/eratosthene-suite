#include "video-client.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <base64/base64.h>

RequestType resolveRequestType(const std::string &e) {
    if (e == "canvas_size") return RT_CANVAS_SIZE;
    if (e == "client_event") return RT_CLIENT_EVENT;
    return RT_UNDEFINED;
}

/**
 * Transforms the string received in json into the client event value
 */
ClientEvent resolveClientEvent(const std::string &e) {
    if (e == "wheel_down") return EV_WHEEL_DOWN;
    if (e == "wheel_up") return EV_WHEEL_UP;
    if (e == "left_mouse_move") return EV_LEFT_MOUSE_MOVEMENT;
    if (e == "right_mouse_move") return EV_RIGHT_MOUSE_MOVEMENT;
    if (e == "button_s") return EV_SPAN_INCREASE;
    if (e == "button_a") return EV_SPAN_DECREASE;
    return EV_UNDEFINED;
}

VideoClient::VideoClient(unsigned char * const data_server_ip, int data_server_port,
                         double view_lat, double view_lon, int view_tia, int view_tib) {
    vc_data_client = new DataClient(data_server_ip, data_server_port);
    cl_model = vc_data_client->get_model();
    cl_view = std::make_shared<er_view_t>((er_view_t) ER_VIEW_D);

    // coordinates ~ above geneva
    cl_view->vw_lat = view_lat;
    cl_view->vw_lon = view_lon;
    cl_view->vw_tia = view_tia;
    cl_view->vw_tib = view_tib;
    cl_view->vw_spn = ER_COMMON_LSPAN;

    vc_video_engine = new VideoEngine(vc_data_client->get_model(), cl_view);
}

VideoClient::~VideoClient() {
    delete(vc_video_engine);
    delete(vc_data_client);
//    delete(vc_video_streamer);
}

void VideoClient::handle_input(ClientEvent event, nlohmann::json mods, nlohmann::json data) {
    le_real_t dx, dy;

    // read data values
    if (data.contains("dx")) dx = (le_real_t) (int) data["dx"];
    if (data.contains("dy")) dy = (le_real_t) (int) data["dy"];

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
            er_view_set_plan(&*cl_view, -dx, -dy);
            break;

        case EV_RIGHT_MOUSE_MOVEMENT:
            /* apply inertia */
            dx *= ER_COMMON_INR;
            dy *= ER_COMMON_INR;
            /* update view direction */
            er_view_set_azm(&*cl_view, -dx);
//            er_view_set_gam(&*cl_view, +dy);
            break;

        case EV_SPAN_INCREASE:
            er_view_set_span( &*cl_view, +1);
            break;

        case EV_SPAN_DECREASE:
            er_view_set_span( &*cl_view, -1);
            break;

        default:break;
    }
}

void VideoClient::handle_message(const ix::WebSocketMessagePtr &msg,
                                 std::shared_ptr<ix::ConnectionState> connectionState) {
    if (!connectionState->isTerminated() && msg->type == ix::WebSocketMessageType::Message) {
        try {
            // parse json
            auto j = nlohmann::json::parse(msg.get()->str.data());

            if (j.contains("request")) {
                    RequestType requestType = resolveRequestType(j["request"]);

                    if (requestType == RT_UNDEFINED) {
                        // Unresolvable request type error
                        std::cerr << "Unrecognized request type : " << j["request"] << std::endl;

                    } else {
                        auto data = j["data"];

                        // Resize screen request
                        if (requestType == RT_CANVAS_SIZE && data.contains("width") && data.contains("height")) {
                            vc_video_engine->set_size((uint32_t) data["width"], (uint32_t) data["height"]);
                        }

                        // User input request
                        else if (requestType == RT_CLIENT_EVENT && j.contains("client_event")) {
                            auto event_name = (std::string) j["client_event"];
                            auto event_type = resolveClientEvent(event_name);
                            if (event_type == EV_UNDEFINED) {
                                // Unresolvable client event type error
                                std::cerr << "Unrecognized client event \"" << event_name << "\"" << std::endl;

                            } else {
                                // Call function to interpret user's inputs
                                handle_input(event_type, j["client_event_mods"], data);
                            }
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
                      while (!connectionState->isTerminated()) {
                          // prepare memory for image
                          char *outputImage = (char *) malloc(vc_video_engine->er_imagedata_size);

                          // render the image and output it to memory
                          VkSubresourceLayout layout;
                          vc_video_engine->draw_frame(outputImage, layout);

                          // encode image for web
                          std::vector<uint8_t> encodedData;
                          stbi_write_jpg_to_func(encode_callback,
                                                 reinterpret_cast<void *>(&encodedData),
                                                 vc_video_engine->get_width(),
                                                 vc_video_engine->get_height(),
                                                 4, outputImage, 30);
                          auto b64 = base64_encode(encodedData.data(), encodedData.size());
                          auto result = b64.data();

                          nlohmann::json j;
                          j["frame"] = result; // the rendered image
                          j["view"] = { // additional data about the rendered image
                                  {"lon", cl_view->vw_lon},
                                  {"lat", cl_view->vw_lat},
                                  {"alt", cl_view->vw_alt},

                                  {"gam", cl_view->vw_gam},
                                  {"azm", cl_view->vw_azm},

                                  {"tia", cl_view->vw_tia},
                                  {"tib", cl_view->vw_tib},

                                  {"spn", cl_view->vw_spn}
                          };

                          // send image data to client
                          webSocket->send(j.dump());

                          // cleanup
                          free(outputImage);
                      }
                  }
    );
    t.detach();
}

void VideoClient::loops_update(std::shared_ptr<ix::ConnectionState> connectionState, le_size_t delay) {
    std::thread t([this, delay, connectionState]() {
        while (true) {
            while (!connectionState->isTerminated() && vc_data_client->update_model(&*cl_view, delay));
        }
    });
    t.detach();
}
