# Overview
This project aims to provide a new client to render 4D graphic elements to the user.
The main goal is to generate images on a server-based GPU and stream them to a distant client with Vulkan headless rendering abilities.

## Dependencies
The project compilation is made for LLVM's clang compiler for C++17.

This client depends on following libraries:
* VulkanSDK
* GLFW 3.2
* GLM

You also need the following tools:
* `cmake`
* `glslc` to compile GLSL to SPIR-V  (https://github.com/google/shaderc)


## Build
To build the project, you can use the command 
```
$ make all
```

To clean the project, you can use the command
```
$ make clean
```

To run the project, make sure your current working directory is the project's home directory (`eratosthene-stream`) and run the executable:
```
$ bin/eratosthene-stream -s [STREAMING_PORT] -d [DATA_SERVER_IP] -p [DATA_SERVER_PORT]
```

You can also specify a default camera view position in the world using GPS coordinates, with the following optional arguments:
```
$ bin/eratosthene-stream [...] -x [VIEW_LATITUDE] -y [VIEW_LONGITUDE] -a [VIEW_TIME_A] -b [VIEW_TIME_B]
 
```

## Code structure
The streaming server works with several threads for different tasks.
The entry point of the program is the websocket server that opens new connections with a web client, which corresponds to the two source files :
- `src/entry-server.h`
- `src/entry-server.cpp`

For every new connection, the server instantiate a `VideoClient` object and fires up its two loops in separate threads that renders the images and updates the data set from the data server.
It also sets up the websocket message exchangers to send image frames and user inputs between the server and the client.
The `VideoClient` instance takes care of exchanging messages with the web client. 
Source code can be found in:
- `src/video-client.h`
- `src/video-client.cpp`

The `VideoClient` instance contains one instance of `DataClient` which connects to the data server and updates its model according to the camera viewpoint.
Source code can be found in:
- `src/data-client.h` 
- `src/data-client.cpp` 

It also contains an instance of `VideoEngine` which handles the Vulkan API to render the 4-dimensional data into an image to be displayed later on the web page.
This image is rendered in a headless manner, the image is not directly displayed on the server's screen, but it's binary representation is kept in-memory and (with some basic encoding) sent later through the network.
Source code can be found in:
- `src/video-engine.h` 
- `src/video-engine.cpp` 

Additional source files:
- `src/models.h` : contains several structures used in the video engine
- `src/utils.h`+`src/utils.cpp` : some utility functions used in the video engine or the video client
- `shaders/shader.vert` + `shaders/shader.frag` : simple shaders used in the graphics pipeline (vertex shader and fragment shader)

## Current Issues and Possible Optimizations
Although the server is to some extent working, it might still be unstable and should be considered as a prototype and not to be used in production.

One of the main issue is the main server crash-dependency to the video clients' threads. If one client suddenly crashes, the whole server will also crash to prevent memory corruption. This could be fixed by creating a new process for each client instead of a thread, although one should understand how to give that new process the websocket connection so that the server process and the newly generated process get truly independent.

The streaming server is not as performant as the native OpenGL client, mainly due to the way image frames are currently distributed to the client. They are encoded in jpg format and sent as raw data in base64. This should be replaced by a proper video streaming server (e.g. HLS or MPEG-DASH) which can greatly reduce the network traffic.