// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#if !defined(VK_RENDERER)
#define VK_RENDERER

#include <SDL.h>
#include <SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <vk_mem_alloc.h>
#include <vk_types.h>

#define PI 3.14159265358979323846264338327950288

#define ARR_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

#define FRAME_OVERLAP 2

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

// this number reflects the texture array size set in the "shaders/textureArray.frag" shader
// both must be equal
#define MAX_TEXTURE_COUNT 80

struct VulkanRenderer
{
    SDL_Window *window = NULL;
    VkExtent2D  window_extent { 720, 480 };
    // VkExtent2D  window_extent { 1920, 1080 };

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

    VkCommandPool command_pool_graphics = {};
    VkCommandPool command_pool_compute  = {};

    uint32_t graphics_queue_family; // family of that queue
    uint32_t compute_queue_family; // family of that queue
    uint32_t present_queue_family; // family of that queue
    VkQueue  graphics_queue; // queue we will submit to
    VkQueue  compute_queue; // queue we will submit to
    VkQueue  present_queue; // queue we will submit to
    VkBool32 is_present_queue_separate;
    // todo(ad): must also check is_compute_queue_separate

    VkDevice         device;
    VkPipeline       default_pipeline;
    VkPipelineLayout default_pipeline_layout;

    //
    // Descriptor sets
    //
    VkDescriptorPool      descriptor_pool;
    VkDescriptorSetLayout set_layout_global;

    VkDescriptorSetLayout              set_layout_array_of_textures;
    VkDescriptorSet                    set_array_of_textures;
    std::vector<VkDescriptorImageInfo> descriptor_image_infos;
    VkSampler                          sampler;
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

FrameData &get_CurrentFrameData();

static VertexInputDescription GetVertexDescription();

static bool CreateShaderModule(const char *filepath, VkShaderModule *outShaderModule);

VkResult CreateBuffer(BufferObject *newBuffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
VkResult AllocateFillBuffer(void *src, size_t size, VmaAllocation allocation);

void       vk_Init();
void       vk_BeginRenderPass();
void       vk_EndRenderPass();
void       vk_Cleanup();
FrameData &get_CurrentFrameData();

VkPipeline CreateGraphicsPipeline();
// Helper functions
bool AllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory);
bool AllocateImageMemory(VkDevice device, VkPhysicalDevice gpu, VkImage image, VkDeviceMemory *memory);
bool AllocateImageMemory(VmaAllocator allocator, VkImage image, VmaAllocation *allocation, VmaMemoryUsage usage);

uint32_t FindProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);
VkResult CreateDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator, VkDescriptorSetLayout *set_layout, const VkDescriptorSetLayoutBinding *bindings, uint32_t binding_count);
VkResult AllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, uint32_t descriptor_set_count, const VkDescriptorSetLayout *set_layouts, VkDescriptorSet *descriptor_set);
#endif // VK_RENDERER
