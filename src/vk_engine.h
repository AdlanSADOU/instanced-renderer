// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

#include <vk_mem_alloc.h>
#include <vk_mesh.h>

#include <vk_types.h>

struct GPUCameraData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewproj;
};

struct FrameData {
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence     _renderFence;

    VkCommandPool   _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    AllocatedBuffer cameraBuffer;
    VkDescriptorSet globalDescriptor;
};

struct Material {
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject {
    Mesh *    mesh;
    Material *material;
    glm::mat4 transformMatrix;
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

class PipelineBuilder;

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()> &&function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call functors
        }

        deletors.clear();
    }
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine
{
  public:
    bool _isInitialized{false};
    int  _frameNumber{0};

    VkExtent2D _windowExtent{1700, 900};
    bool       _up = false, _down = false, _left = false, _right = false;
    bool       _W = false, _S = false, _A = false, _D = false;
    float      _x = 0, _y = 0, _z = 0;

    struct SDL_Window *_window{nullptr};

    //
    // Vulkan Context
    //
    VkInstance               _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice         _chosenGPU;
    VkDevice                 _device;
    VkSurfaceKHR             _surface;

    //
    // Swapchain
    //
    VkSwapchainKHR           _swapchain;
    VkFormat                 _swapchainImageFormat; // image format expected by the windowing system
    std::vector<VkImage>     _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    uint32_t _graphicsQueueFamily; //family of that queue
    VkQueue  _graphicsQueue;       //queue we will submit to

    FrameData _frames[FRAME_OVERLAP];

    //
    // Descriptor sets
    //
    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorPool      _descriptorPool;

    //
    // RenderPass & Framebuffers
    //
    VkRenderPass               _renderPass;   // created before renderpass
    std::vector<VkFramebuffer> _framebuffers; // these are created for a specific renderpass

    VkImageView    _depthImageView;
    AllocatedImage _depthImage;
    VkFormat       _depthFormat;

    DeletionQueue _mainDeletionQueue;

    VmaAllocator _allocator;

    //default array of renderable objects
    std::vector<RenderObject> _renderables;

    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh>     _meshes;

    void init();
    void cleanup();
    void draw();
    void run();

  private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
    void init_framebuffers();
    void init_sync_structures();
    bool load_shader_module(const char *filepath, VkShaderModule *outShaderModule);
    void init_descriptors();
    void init_pipelines();

    void load_meshes();
    void upload_mesh(Mesh &mesh);

    //create material and add it to the map
    Material *    create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);
    Material *    get_material(const std::string &name);
    Mesh *        get_mesh(const std::string &name);
    RenderObject *get_renderable(std::string name);

    void       init_scene();
    void       draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);
    FrameData &get_current_frame();

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};

class PipelineBuilder
{
  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo         _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo       _inputAssembly;

    VkViewport _viewport;
    VkRect2D   _scissor;

    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState    _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo   _multisampling;
    VkPipelineLayout                       _pipelineLayout;

    VkPipelineDepthStencilStateCreateInfo _depthStencil;

    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};
