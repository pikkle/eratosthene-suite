# Overview
This project aims to provide a new client to render 4D graphic elements to the user.
The main goal is to generate images on a server-based GPU and stream them to a distant client with Vulkan headless rendering abilities.

## Dependencies
This client depends on following libraries:
* VulkanSDK
* GLFW 3.2

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
$ bin/eratosthene-stream
```