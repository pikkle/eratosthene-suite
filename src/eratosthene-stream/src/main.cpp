
// @TODO: remove import of GLFW and include vulkan directly instead
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
#include <thread>

#include "models.h"
#include "utils.h"
#include "engine.h"
#include "hardware.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const float FPS = 60;

const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},

        {{-0.6f, -0.6f, -0.6f}, {0.0f, 0.0f, 1.0f}},
        {{0.6f, 0.6f, 0.6f}, {0.0f, 1.0f, 0.0f}},

        {{0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint16_t> triangles = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
};

const std::vector<uint16_t> lines = {
        8, 9,
};

const std::vector<uint16_t> points = {
        10,
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

        auto view = glm::lookAt(
                glm::vec3(2.0f, 2.0f, 2.0f), // eye
                glm::vec3(0.0f, 0.0f, 0.0f), // center
                glm::vec3(0.0f, 0.0f, 1.0f) // up
        );

        engine = new Engine(WIDTH, HEIGHT, FPS);
        engine->feedVertices(vertices);
        if (triangles.size() % 3 != 0) {
            throw std::runtime_error("Invalid format of triangle indices list");
        }
        engine->drawTriangles(triangles);

        if (lines.size() % 2 != 0) {
            throw std::runtime_error("Invalid format of triangle indices list");
        }
        engine->drawLines(lines);
        engine->drawPoints(points);

        engine->setSceneView(view);
        engine->prepareRendering();

    }

    static void framebufferResizeCallback(GLFWwindow* _window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(_window));
        app->engine->setFramebufferResized();
    }

    void mainLoop() {
        while (true) {
//            glfwPollEvents();
            engine->render();
        }
        engine->waitIdle();
    }

    void cleanup() {
        delete engine;
//        glfwDestroyWindow(window);
//        glfwTerminate();
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