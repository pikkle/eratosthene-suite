#ifndef ERATOSTHENE_STREAM_MODELS_H
#define ERATOSTHENE_STREAM_MODELS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <array>
#include <optional>
#include <vector>

namespace StreamModels {

struct Vertex {
    /*! position of the vertex */
    glm::vec3 pos;

    /*! color of the vertex */
    glm::vec3 color;

    /*! Used to identify the cell in which the vertex belongs to, in order to apply the appropriate transformation matrix */
    uint32_t cell_id;

    /*!
     * A vertex binding describes at which rate to load data from memory throughout the vertices.
     * It specifies the number of bytes between data entries and whether to move to the next data
     * entry after each vertex or after each instance.
     */
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    };

    /*!
     * An attribute description struct describes how to extract a vertex attribute from a chunk of
     * vertex data originating from a binding description. We have two attributes, position and color,
     * so we need two attribute description structs.
     */
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[2].offset = offsetof(Vertex, cell_id);

        return attributeDescriptions;
    };
};

/*!
 * Transformation matrices to pass to the vertex shader
 */
struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    uint32_t modelCount;
    glm::mat4 *model;

    static size_t size_with_count(size_t count) {
        return sizeof(view) + sizeof(proj) + sizeof(modelCount) + count * sizeof(*model);
    }

    size_t size() const {
        return size_with_count(modelCount);
    }

};

/*!
 * A collection of what composes an image in vulkan
 */
struct Attachment {
    VkImage img;
    VkDeviceMemory mem;
    VkImageView view;
};


struct BufferWrap {
    VkBuffer buf;
    VkDeviceMemory mem;
};

}
#endif //ERATOSTHENE_STREAM_MODELS_H
