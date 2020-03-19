#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <vector>

#include "models.h"
#include "utils.h"
#include "engine.h"
#include "hardware.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint16_t> triangles = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
};

class HelloTriangleApplication {
public:
    void run() {
        init();
        mainLoop();
        cleanup();
    }
private:
    GLFWwindow* window;
    Engine* engine;
    const char* window_title = "Eratosthene Vulkan";

    void init() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, window_title, nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        auto view = glm::lookAt(
                glm::vec3(2.0f, 2.0f, 2.0f), // eye
                glm::vec3(0.0f, 0.0f, 0.0f), // center
                glm::vec3(0.0f, 0.0f, 1.0f) // up
        );

        engine = new Engine(window);
        engine->feedVertices(vertices);
        engine->drawTriangles(triangles);
//        engine->drawPoints();
//        engine->drawLines();
        engine->setSceneView(view);
        engine->prepareRendering();

    }

    static void framebufferResizeCallback(GLFWwindow* _window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(_window));
        app->engine->setFramebufferResized();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            engine->render();
        }
        engine->waitIdle();
    }

    void cleanup() {
        delete engine;
        glfwDestroyWindow(window);
        glfwTerminate();
    }

};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}