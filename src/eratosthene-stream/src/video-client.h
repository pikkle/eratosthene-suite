#ifndef ERATOSTHENE_STREAM_VIDEO_CLIENT_H
#define ERATOSTHENE_STREAM_VIDEO_CLIENT_H

#include <unistd.h>
#include <thread>

#include <IXWebSocket/ixwebsocket/IXWebSocket.h>
#include <IXWebSocket/ixwebsocket/IXWebSocketMessage.h>
#include <IXWebSocket/ixwebsocket/IXConnectionState.h>

#include "video-engine.h"
#include "data-client.h"
#include "video-streamer.h"

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
    VideoClient();

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
    void rendering_loop(std::shared_ptr<ix::WebSocket> webSocket,
                        std::shared_ptr<ix::ConnectionState> connectionState);


    /**
     * Handles a received message through websocket. Updates internal data of the data client and video engine to
     * update the rendering according to inputs from the user
     * @param msg The websocket message wrap
     * @param connectionState The websocket connection state
     */
    void handle_message(const ix::WebSocketMessagePtr &msg, std::shared_ptr<ix::ConnectionState> connectionState);
private:
    std::shared_ptr<VideoEngine> vc_video_engine; // The video engine responsible to handle the GPU rendering on the server machine
    std::shared_ptr<DataClient> vc_data_client; // The data client responsible to fetch data from the data server
    std::shared_ptr<VideoStreamer> vc_video_streamer; // The video streamer responsible to send image frames on the web using appropriate codecs
};

#endif
