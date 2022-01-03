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


#define ARR_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

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
    struct Data
    {
        glm::mat4 view       = {};
        glm::mat4 projection = {};
        glm::mat4 viewproj   = {};
    } data {};

    BufferObject UBO[FRAME_OVERLAP];
};

struct VulkanRenderer
{
    SDL_Window  *window = NULL;
    Camera       camera = {};
    FrameData    frames[FRAME_OVERLAP];
    BufferObject triangle_SSBO[FRAME_OVERLAP];
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
    uint32_t present_queue_family; // family of that queue
    VkQueue  graphics_queue_idx; // queue we will submit to
    VkQueue  present_queue_idx; // queue we will submit to
    VkBool32 is_present_queue_separate;


    VkDevice         device;
    VkPipeline       default_pipeline;
    VkPipelineLayout default_pipeline_layout;

    //
    // Descriptor sets
    //
    VkDescriptorPool      descriptor_pool;
    VkDescriptorSetLayout global_set_layout;
    VkDescriptorSetLayout model_set_layout;

    //
    // RenderPass & Framebuffers
    //
    VkRenderPass               render_pass;
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

    VkExtent2D window_extent { 1280, 720 };
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
    VkPipelineVertexInputStateCreateInfo         create_info_vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo       create_info_input_assembly_state;
    VkPipelineMultisampleStateCreateInfo         create_info_multisample_state;
    VkPipelineDepthStencilStateCreateInfo        create_info_depth_stencil_state;
    VkPipelineColorBlendAttachmentState          attachment_state_color_blend;
    VkPipelineRasterizationStateCreateInfo       rasterization_state;
    VkPipelineLayout                             pipeline_layout;
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
void        CreateBuffer(BufferObject *newBuffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
void        AllocateFillBuffer(void *src, size_t size, VmaAllocation allocation);

static void draw_Renderables(VkCommandBuffer cmd, RenderObject *first, int count);

void vk_Init();
void VulkanUpdateAndRender(double dt);
void vk_EndPass();
void UpdateAndRender();
void vk_Cleanup();

// temporary examples
void DrawExamples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *buffer, double dt);
void InitExamples();

// Helper functions
bool     AllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory);
uint32_t FindProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);
VkResult CreateDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator, VkDescriptorSetLayout *set_layout, const VkDescriptorSetLayoutBinding *bindings, uint32_t binding_count);
VkResult AllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, uint32_t descriptor_set_count, const VkDescriptorSetLayout *set_layouts, VkDescriptorSet *descriptor_set);
#endif // VK_RENDERER
