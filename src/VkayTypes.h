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


struct vkayPhysicalDeviceSurfaceInfo_KHR
{
    using SurfaceFormats = std::vector<VkSurfaceFormatKHR>;
    using PresentModes   = std::vector<VkPresentModeKHR>;

    uint32_t                 format_count        = {};
    SurfaceFormats           formats             = {};
    uint32_t                 present_modes_count = {};
    PresentModes             present_modes       = {};
    VkSurfaceCapabilitiesKHR capabilities        = {};

    void query(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, NULL);
        formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, formats.data());

        // todo(ad): .presentMode arbitrarily set right now, we need to check what the OS supports and pick one
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &present_modes_count, NULL);
        present_modes.resize(present_modes_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &present_modes_count, present_modes.data());

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    };
};

struct Color
{
    float r;
    float g;
    float b;
    float a;

    Color()
    {
        a = r = g = b = 0;
    }

    Color(float r, float g, float b, float a)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
    glm::vec4 color;
};

struct Vertex2
{
    glm::vec3 position;
    // glm::vec3 normal;
};

struct BaseMesh
{
    std::vector<Vertex> vertices = {};
};

struct Quad
{
    BaseMesh mesh = {};

    Quad()
    {
        mesh.vertices.resize(6);
        mesh.vertices[0] = { { -1.0f, -1.0f, 1.f }, { 0.0f, 0.0f, -1.f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // bot-left1
        mesh.vertices[1] = { { -1.0f, +1.0f, 1.f }, { 0.0f, 0.0f, -1.f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // top-left
        mesh.vertices[2] = { { +1.0f, +1.0f, 1.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // top-right1
        mesh.vertices[3] = { { -1.0f, -1.0f, 1.f }, { 0.0f, 0.0f, -1.f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // bot-left2
        mesh.vertices[4] = { { +1.0f, +1.0f, 1.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // top-right2
        mesh.vertices[5] = { { +1.0f, -1.0f, 1.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // bot-right
    }
};

struct Triangle
{
    BaseMesh mesh = {};

    Triangle()
    {
        mesh.vertices.resize(3);
        mesh.vertices[0] = { { -1.0f, -1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // bot-left1
        mesh.vertices[1] = { { -1.0f, +1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // top-left
        mesh.vertices[2] = { { +1.0f, +1.0f, 0.f }, { 0.0f, 0.0f, -1.f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }; // top-right1
    }

    uint32_t indicies[3] = {
        0, 1, 2
    };
};

struct InstanceData
{
    alignas(16) glm::vec3 pos   = {};
    alignas(16) glm::vec3 rot   = {};
    alignas(16) glm::vec3 scale = {};
    int32_t texure_idx;
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

struct VkayBuffer
{
    VkBuffer buffer;

    // union?
    VmaAllocation  allocation;
    VkDeviceMemory memory;
};

struct AllocatedImage
{
    VkImage       image;
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
    VkPipelineLayout pipeline_layout;
};

struct Transform
{
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 rotation;
    alignas(16) glm::vec3 scale;
};

// struct RenderObject
// {
//     Mesh     *mesh            = {};
//     Material *material        = {};
//     glm::mat4 transformMatrix = {};
// };

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