// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#if !defined(VK_RENDERER)
#define VK_RENDERER

#include <SDL.h>
#include <SDL_vulkan.h>

#include <functional>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <vk_mem_alloc.h>
#include <vk_types.h>

#define FRAME_OVERLAP 2

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

struct Camera
{
    glm::mat4    view       = {};
    glm::mat4    projection = {};
    glm::mat4    viewproj   = {};
    BufferObject camera_buffer[FRAME_OVERLAP];
};

struct VulkanRenderer
{
    SDL_Window *window = NULL;
    CameraData  cam_data;
    Camera camera = {};
    //
    // Vulkan initialization phase types
    //
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice         chosen_gpu;
    VkSurfaceKHR             surface;
    //
    // Swapchain
    //
    VkSwapchainKHR           swapchain;
    VkFormat                 swapchain_image_format; // image format expected by the windowing system
    std::vector<VkImage>     swapchain_images;
    std::vector<VkImageView> swapchain_image_views;

    uint32_t graphics_queue_family; // family of that queue
    VkQueue  graphicsQueue; // queue we will submit to

    FrameData frames[FRAME_OVERLAP];

    VkDevice              device;
    VkPipeline            default_pipeline;
    VkPipelineLayout      default_pipeline_layout;
    VkDescriptorSetLayout global_set_layout;

    //
    // Descriptor sets
    //
    VkDescriptorPool descriptor_pool;

    //
    // RenderPass & Framebuffers
    //
    VkRenderPass               render_pass; // created before renderpass
    std::vector<VkFramebuffer> framebuffers; // these are created for a specific renderpass

    VkImageView    depth_image_view;
    AllocatedImage depth_image;
    VkFormat       depth_format;

    ReleaseQueue release_queue;

    // todo(ad): delete this at some point
    VmaAllocator allocator;

    // todo(ad): must rethink these
    std::vector<RenderObject>                 renderables;
    std::unordered_map<std::string, Material> materials;
    std::unordered_map<std::string, Mesh>     meshes;
    ///////////////////////////////////

    bool     is_initialized     = false;
    uint64_t frame_idx_inflight = 0;

    VkExtent2D window_extent { 1700, 900 };
};

#define VK_CHECK(x)                                    \
    do {                                               \
        VkResult err = x;                              \
        if (err) {                                     \
            SDL_Log("Detected Vulkan error: %d", err); \
            abort();                                   \
        }                                              \
    } while (0)

#define SECONDS(value) (1000000000 * value)

struct PipelineBuilder
{
    std::vector<VkPipelineShaderStageCreateInfo> create_info_shader_stages;
    VkPipelineVertexInputStateCreateInfo         create_info_vertex_input;
    VkPipelineInputAssemblyStateCreateInfo       create_info_input_assembly_state;
    VkPipelineRasterizationStateCreateInfo       create_info_rasterizer;
    VkPipelineColorBlendAttachmentState          create_info_attachment_state_color_blend;
    VkPipelineMultisampleStateCreateInfo         create_info_multisample_state;
    VkPipelineDepthStencilStateCreateInfo        create_info_depth_stencil_state;
    VkPipelineLayout                             create_info_pipeline_layout;
    VkViewport                                   viewport;
    VkRect2D                                     scissor;

    VkPipeline BuildPipeline(VkDevice device, VkRenderPass render_pass);
};

FrameData           &get_CurrentFrameData();
static Mesh         *get_Mesh(const std::string &name);
static Material     *get_Material(const std::string &name);
static RenderObject *get_Renderable(std::string name);

static VertexInputDescription GetVertexDescription();

static bool InitShaderModule(const char *filepath, VkShaderModule *outShaderModule);
static void init_Scene();
static void InitMeshes();

// create material and add it to the map
static Material *create_Material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);

static void CreateMesh(Mesh &mesh);
void        BufferCreate(BufferObject *newBuffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
void        BufferFill(void *src, size_t size, VmaAllocation allocation);

static void draw_Renderables(VkCommandBuffer cmd, RenderObject *first, int count);

void vk_Init();
void VulkanUpdateAndRender(double dt);
void vk_EndPass();
void UpdateAndRender();
void vk_Cleanup();

// temporary examples
void DrawExamples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer, double dt);
void InitExamples(VkDevice device, VkPhysicalDevice chosen_gpu);

// Helper functions
bool     allocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory);
uint32_t findProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);

#endif // VK_RENDERER
