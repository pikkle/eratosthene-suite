#ifndef ERATOSTHENE_STREAM_VIDEO_CLIENT_H
#define ERATOSTHENE_STREAM_VIDEO_CLIENT_H

#include <unistd.h>
#include <thread>

#include <nlohmann/json.hpp>

#include <IXWebSocket/ixwebsocket/IXWebSocket.h>
#include <IXWebSocket/ixwebsocket/IXWebSocketMessage.h>
#include <IXWebSocket/ixwebsocket/IXConnectionState.h>

#include "video-engine.h"
#include "data-client.h"
#include "video-streamer.h"

const double DEFAULT_LON = 6.126579;
const double DEFAULT_LAT = 46.2050282;
const int DEFAULT_TIA = 1117584000;
const int DEFAULT_TIB = 1117584000;

/**
 * List all possible request types from websocket client
 */
enum RequestType {
    RT_UNDEFINED,
    RT_CANVAS_SIZE,  // request to resize the rendered frame
    RT_CLIENT_EVENT, // request to handle a user input (keyboard / mouse)
    RT_SET_VIEW,     // request to manually set up the view point
};

/**
 * List all possible client events that can be received from websocket
 */
enum ClientEvent {
    EV_UNDEFINED,
    EV_WHEEL_UP,
    EV_WHEEL_DOWN,
    EV_LEFT_MOUSE_MOVEMENT,
    EV_RIGHT_MOUSE_MOVEMENT,
    EV_SPAN_INCREASE,
    EV_SPAN_DECREASE,
};

/**
 * This class is responsible to subordinate all tasks to have a rendering engine available on the web.
 * It gathers required data from the data server, requests its own video engine to render frames and forwards these
 * frames to the video streamer codecs.
 *
 * An instance of this class is thus a priori reserved to a single web client and is destroyed upon connection closing.
 */
class VideoClient {
public:
    /**
     * Constructor of a video client. Initiate an instance of vulkan engine, a data client that connects to the data
     * server and (in the @FUTURE) a video stream server to send image frames through the network
     */
    VideoClient(unsigned char * const data_server_ip, int data_server_port,
                double lat = DEFAULT_LAT, double lon = DEFAULT_LON, int tia = DEFAULT_TIA, int tib = DEFAULT_TIB);

    /**
     * Destructor of video client. Release the GPU from an engine, closes the data socket and (in the @FUTURE) closes
     * the dedicated stream server
     */
    ~VideoClient();

    /**
     * Creates a separate thread that call frame rendering and send them through network
     * @param webSocket The websocket used for communication with the webclient (should be removed in the @FUTURE)
     * @param connectionState
     */
    void loops_render(std::shared_ptr<ix::WebSocket> webSocket,
                      std::shared_ptr<ix::ConnectionState> connectionState);


    /**
     * Creates a separate thread that automatically refresh data to be displayed
     */
    void loops_update(std::shared_ptr<ix::ConnectionState> connectionState,
            le_size_t delay = 0);

    /**
     * Handles a received message through websocket. Updates internal data of the data client and video engine to
     * update the rendering according to inputs from the user
     * @param msg The websocket message wrap
     * @param connectionState The websocket connection state
     */
    void handle_message(const ix::WebSocketMessagePtr &msg, std::shared_ptr<ix::ConnectionState> connectionState);

    void handle_input(ClientEvent clientEvent, nlohmann::json mods, nlohmann::json data);

private:
    VideoEngine *vc_video_engine; // The video engine responsible to handle the GPU rendering on the server machine
    DataClient *vc_data_client; // The data client responsible to fetch data from the data server
    VideoStreamer *vc_video_streamer; // The video streamer responsible to send image frames on the web using appropriate codecs

    std::shared_ptr<er_model_t> cl_model; // Model sub-module structure

    std::shared_ptr<er_view_t>  cl_view; // Active point of view

    le_size_t  cl_x; // Mouse click x-position - according to screen
    le_size_t  cl_y; // Mouse click y-position - according to screen
    le_real_t  cl_inertia; // Motion inertial factor

    le_real_t  cl_scale; // Overall model dynamical scale value
};

#endif
