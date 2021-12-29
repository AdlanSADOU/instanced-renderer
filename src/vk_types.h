// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <deque>

struct InstanceData
{
    glm::mat4 tranform_matrix;
};

struct BufferObject
{
    VkBuffer       buffer;
    VkDeviceMemory device_memory;
    VmaAllocation  allocation;
};

struct FrameData
{
    VkSemaphore     present_semaphore   = {};
    VkSemaphore     render_semaphore    = {};
    VkFence         render_fence        = {};
    VkCommandPool   command_pool        = {};
    VkCommandBuffer main_command_buffer = {};
    VkDescriptorSet global_descriptor   = {};
};

struct MeshPushConstants
{
    glm::vec4 data          = {};
    glm::mat4 render_matrix = {};
};

struct AllocatedImage
{
    VkImage       _image;
    VmaAllocation allocation;
};

struct VertexInputDescription
{
    std::vector<VkVertexInputBindingDescription>   bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Material
{
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    BufferObject        vertex_buffer;
    BufferObject        index_buffer;

    bool LoadFromObj(const char *filename);
};

struct RenderObject
{
    Mesh     *mesh            = {};
    Material *material        = {};
    glm::mat4 transformMatrix = {};
};
