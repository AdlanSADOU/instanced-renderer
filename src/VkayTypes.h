// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <SDL2/SDL_vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <functional>
#include <deque>

class vector;

#define PI 3.14159265358979323846264338327950288

#define ARR_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 tex_uv;
};

struct Quad
{
    Vertex vertices[6] = {
        { { -1.0f, -1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // bot-left 1
        { { -1.0f, +1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }, // top-left
        { { +1.0f, +1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // top-right 1
        { { -1.0f, -1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // bot-left 2
        { { +1.0f, +1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // top-right 2
        { { +1.0f, -1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }, // bot-right
    };
};

struct BufferObject
{
    VkBuffer buffer;

    // union?
    VmaAllocation  allocation;
    VkDeviceMemory memory;
};

struct InstanceData
{
    alignas(16) glm::vec3 pos   = {};
    alignas(16) glm::vec3 rot   = {};
    alignas(16) glm::vec3 scale = {};
    int32_t texure_id;
};


struct ModelData
{
    glm::mat4 transform;
};


struct FrameData
{
    VkSemaphore     present_semaphore = {};
    VkSemaphore     render_semaphore  = {};
    VkFence         render_fence      = {};
    VkFence         compute_fence     = {};
    VkCommandBuffer cmd_buffer_gfx    = {};
    VkCommandBuffer cmd_buffer_cmp    = {};
    VkDescriptorSet set_model         = {};
    uint32_t        idx_swapchain_image;
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

////////////////////////////
// todo(ad): cleanup

enum ECommandPoolType
{
    Graphics,
    Transfer,
    Present
};

struct ReleaseQueue
{
    std::deque<std::function<void()>> deletors = {};

    void push_function(std::function<void()> &&function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); // call functors
        }

        deletors.clear();
    }
};

static std::unordered_map<VkResult, std::string> fence_status = {
    { VK_SUCCESS, "VK_SUCCESS" },
    { VK_NOT_READY, "VK_NOT_READY" },
    { VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST" },
};