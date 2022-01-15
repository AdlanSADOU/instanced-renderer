// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#if !defined(VK_RENDERER)
#define VK_RENDERER

#if defined(WIN32)
#define __func__ __FUNCTION__
#endif

#include <SDL.h>
#include <SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <vk_types.h>

#define PI 3.14159265358979323846264338327950288

#define ARR_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

#define FRAME_BUFFER_COUNT 2

#define SECONDS(value) (1000000000 * value)

FrameData &get_CurrentFrameData();

VertexInputDescription GetVertexDescription();

static bool CreateShaderModule(const char *filepath, VkShaderModule *outShaderModule);

VkResult CreateBuffer(BufferObject *dst_buffer, VmaAllocator allocator, ReleaseQueue *queue, size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
bool     CreateUniformBuffer(VkDevice device, VkDeviceSize size, VkBuffer *out_buffer);
VkResult MapMemcpyMemory(void *src, size_t size, VmaAllocator allocator, VmaAllocation allocation);

void       vk_Init();
void       vk_BeginCommandBuffer();
void       vk_EndCommandBuffer();
void       vk_BeginRenderPass();
void       vk_EndRenderPass();
void       vk_Cleanup();
FrameData &get_CurrentFrameData();

VkPipeline CreateGraphicsPipeline();
VkPipeline CreateComputePipeline();

// Helper functions
bool AllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory);
bool AllocateImageMemory(VkDevice device, VkPhysicalDevice gpu, VkImage image, VkDeviceMemory *memory);
bool AllocateImageMemory(VmaAllocator allocator, VkImage image, VmaAllocation *allocation, VmaMemoryUsage usage);

uint32_t FindProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);
VkResult CreateDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator, VkDescriptorSetLayout *set_layout, const VkDescriptorSetLayoutBinding *bindings, uint32_t binding_count);
VkResult AllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, uint32_t descriptor_set_count, const VkDescriptorSetLayout *set_layouts, VkDescriptorSet *descriptor_set);

struct VulkanRenderer
{
    SDL_Window *window = NULL;
    VkExtent2D  window_extent { 720, 480 };
    // VkExtent2D  window_extent { 1920, 1080 };

    FrameData    frames[FRAME_BUFFER_COUNT];
    BufferObject triangle_SSBO[FRAME_BUFFER_COUNT];
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

    ////////////////////
    // Graphics
    VkDescriptorPool descriptor_pool;

    VkDescriptorSetLayout set_layout_global; // todo(ad): camera set layout
    VkDescriptorSetLayout set_layout_array_of_textures;
    VkDescriptorSet       set_array_of_textures;

    std::vector<VkDescriptorImageInfo> descriptor_image_infos;
    VkSampler                          sampler;

    ////////////////////
    // Compute
    VkPipeline                   compute_pipeline;
    VkPipelineLayout             compute_pipeline_layout;
    VkDescriptorPool             descriptor_pool_compute;
    VkDescriptorSetLayout        set_layout_instanced_data;
    std::vector<VkDescriptorSet> compute_descriptor_sets;

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
    uint32_t frame_idx_inflight = 0;
};

struct Camera
{
    struct Data
    {
        glm::mat4 view       = {};
        glm::mat4 projection = {};
        glm::mat4 viewproj   = {};
    } data {};

    void           *data_ptr[FRAME_BUFFER_COUNT];
    BufferObject    UBO[FRAME_BUFFER_COUNT];
    VkDescriptorSet set_global[FRAME_BUFFER_COUNT] = {};


    void MapDataPtr(VmaAllocator allocator, VkDevice device, VkPhysicalDevice gpu, VulkanRenderer vkr)
    {
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {

            CreateBuffer(&UBO[i], vkr.allocator, &vkr.release_queue, sizeof(Camera::Data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            VkDescriptorBufferInfo info_descriptor_camera_buffer;
            AllocateDescriptorSets(vkr.device, vkr.descriptor_pool, 1, &vkr.set_layout_global, &set_global[i]);

            info_descriptor_camera_buffer.buffer = UBO[i].buffer;
            info_descriptor_camera_buffer.offset = 0;
            info_descriptor_camera_buffer.range  = sizeof(Camera::Data);

            VkWriteDescriptorSet write_camera_buffer = {};
            write_camera_buffer.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_camera_buffer.pNext                = NULL;
            write_camera_buffer.dstBinding           = 0; // we are going to write into binding number 0
            write_camera_buffer.dstSet               = set_global[i]; // of the global descriptor
            write_camera_buffer.descriptorCount      = 1;
            write_camera_buffer.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // and the type is uniform buffer
            write_camera_buffer.pBufferInfo          = &info_descriptor_camera_buffer;
            vkUpdateDescriptorSets(vkr.device, 1, &write_camera_buffer, 0, NULL);

            vmaMapMemory(allocator, UBO[i].allocation, &data_ptr[i]);
        }
    }

    void UnmapDataPtr(VmaAllocator allocator, uint32_t index)
    {
        vmaUnmapMemory(allocator, UBO[index].allocation);
    }

    /**
     * @brief A convevience function to unmap every data ptr
     */
    void UnmapDataPtrs(VmaAllocator allocator)
    {
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
            vmaUnmapMemory(allocator, UBO[i].allocation);
        }
    }

    void copyDataPtr(uint32_t frame_idx)
    {
        memcpy(data_ptr[frame_idx], &data, sizeof(data));
    }

    void DestroyBuffers(VkDevice device)
    {
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
            vkDestroyBuffer(device, UBO[i].buffer, NULL);
        }
    }

    /**
     ** @brief A convevience function to unmap every data ptr and destroy all buffers -
     ** do not mix with UnmapDataPtrs/ UnmapDataPtr or DestroyBuffers
     */
    void Destroy(VkDevice device, VmaAllocator allocator)
    {
        vkDeviceWaitIdle(device);

        UnmapDataPtrs(allocator);
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
            vmaDestroyBuffer(allocator, UBO[i].buffer, UBO[i].allocation);
        }
    }
};

// this number reflects the texture array size set in the "shaders/textureArray.frag" shader
// both must be equal
#define MAX_TEXTURE_COUNT 80

#define VK_CHECK(x)                                    \
    do {                                               \
        VkResult err = x;                              \
        if (err) {                                     \
            SDL_Log("Detected Vulkan error: %d", err); \
            abort();                                   \
        }                                              \
    } while (0)

#endif // VK_RENDERER
