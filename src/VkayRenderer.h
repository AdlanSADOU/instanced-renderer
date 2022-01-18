#if !defined(VK_RENDERER)
#define VK_RENDERER

#if defined(WIN32)
#define __func__ __FUNCTION__
#endif

#ifdef _WIN32
#define EXPORT //__declspec(dllexport)
#elif __GNUC__
#define EXPORT __attribute__((visibility("default")))
// #define EXPORT
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <VkayTypes.h>

struct VkayRenderer;

#define FRAME_BUFFER_COUNT 2

#define SECONDS(value) (1000000000 * value)



EXPORT void       VkayRendererInit(VkayRenderer *vkr);
EXPORT void       VkayRendererBeginCommandBuffer(VkayRenderer *vkr);
EXPORT void       VkayRendererEndCommandBuffer(VkayRenderer *vkr);
EXPORT void       VkayRendererBeginRenderPass(VkayRenderer *vkr);
EXPORT void       VkayRendererEndRenderPass(VkayRenderer *vkr);
EXPORT void       VkayRendererCleanup(VkayRenderer *vkr);
EXPORT FrameData *VkayRendererGetCurrentFrameData(VkayRenderer *vkr);

EXPORT VertexInputDescription GetVertexDescription();
bool                          CreateShaderModule(const char *filepath, VkShaderModule *out_ShaderModule, VkDevice device);
EXPORT VkResult               CreateBuffer(BufferObject *dst_buffer, VmaAllocator allocator, ReleaseQueue *queue, size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
EXPORT bool                   CreateUniformBuffer(VkDevice device, VkDeviceSize size, VkBuffer *out_buffer);
EXPORT VkResult               MapMemcpyMemory(void *src, size_t size, VmaAllocator allocator, VmaAllocation allocation);
EXPORT VkPipeline             CreateGraphicsPipeline(VkayRenderer *vkr);
EXPORT VkPipeline             CreateComputePipeline(VkayRenderer *vkr);
EXPORT bool                   AllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory);
EXPORT bool                   AllocateImageMemory(VkDevice device, VkPhysicalDevice gpu, VkImage image, VkDeviceMemory *memory);
EXPORT bool                   AllocateImageMemory(VmaAllocator allocator, VkImage image, VmaAllocation *allocation, VmaMemoryUsage usage);
EXPORT void                   CopyBuffer(VkCommandBuffer cmd_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
EXPORT uint32_t               FindProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);
EXPORT VkResult               CreateDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator, VkDescriptorSetLayout *set_layout, const VkDescriptorSetLayoutBinding *bindings, uint32_t binding_count);
EXPORT VkResult               AllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, uint32_t descriptor_set_count, const VkDescriptorSetLayout *set_layouts, VkDescriptorSet *descriptor_set);

struct VkayRenderer
{
    SDL_Window *window = NULL;
    // VkExtent2D  window_extent { 720, 480 };
    VkExtent2D window_extent { 1920, 1000 };

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

    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t present_queue_family;
    VkQueue  graphics_queue;
    VkQueue  compute_queue;
    VkQueue  present_queue;
    VkBool32 is_present_queue_separate;
    // todo(ad): must also check is_compute_queue_separate

    VkDevice         device;
    VkPipeline       default_pipeline;
    VkPipelineLayout default_pipeline_layout;

    ////////////////////
    // Graphics
    VkDescriptorPool descriptor_pool;

    VkDescriptorSetLayout set_layout_global; // todo(ad): camera set layout

    // Do not modify this
    uint32_t              texture_array_index = 0;
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

struct EXPORT Camera
{
    struct Data
    {
        glm::mat4 view       = {};
        glm::mat4 projection = {};
        glm::mat4 viewproj   = {};
    } data {};

    void           *m_data_ptr[FRAME_BUFFER_COUNT];
    BufferObject    m_UBO[FRAME_BUFFER_COUNT];
    VkDescriptorSet m_set_global[FRAME_BUFFER_COUNT] = {};
    glm::vec3       m_position;
    VkayRenderer   *m_vkr;

    void MapDataPtr(VmaAllocator allocator, VkDevice device, VkPhysicalDevice gpu, VkayRenderer *vkr)
    {
        this->m_vkr = vkr;
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {

            CreateBuffer(&m_UBO[i], m_vkr->allocator, &m_vkr->release_queue, sizeof(Camera::Data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            VkDescriptorBufferInfo info_descriptor_camera_buffer;
            AllocateDescriptorSets(m_vkr->device, m_vkr->descriptor_pool, 1, &m_vkr->set_layout_global, &m_set_global[i]);

            info_descriptor_camera_buffer.buffer = m_UBO[i].buffer;
            info_descriptor_camera_buffer.offset = 0;
            info_descriptor_camera_buffer.range  = sizeof(Camera::Data);

            VkWriteDescriptorSet write_camera_buffer = {};
            write_camera_buffer.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_camera_buffer.pNext                = NULL;
            write_camera_buffer.dstBinding           = 0; // we are going to write into binding number 0
            write_camera_buffer.dstSet               = m_set_global[i]; // of the global descriptor
            write_camera_buffer.descriptorCount      = 1;
            write_camera_buffer.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // and the type is uniform buffer
            write_camera_buffer.pBufferInfo          = &info_descriptor_camera_buffer;
            vkUpdateDescriptorSets(m_vkr->device, 1, &write_camera_buffer, 0, NULL);

            vmaMapMemory(allocator, m_UBO[i].allocation, &m_data_ptr[i]);
        }
    }

    void Update(VkCommandBuffer cmd_buffer)
    {
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkr->default_pipeline_layout, 0, 1, &m_set_global[m_vkr->frame_idx_inflight], 0, NULL);

        glm::mat4 translation = glm::translate(glm::mat4(1.f), m_position);
        glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), glm::radians(0.f), glm::vec3(1, 0, 0));
        glm::mat4 view        = rotation * translation;
        // glm::mat4 projection  = glm::perspective(glm::radians(60.f), (float)m_vkr->window_extent.width / (float)m_vkr->window_extent.height, 0.1f, 1000.0f);
        glm::mat4 projection = glm::ortho(0.f, (float)m_vkr->window_extent.width, 0.f, (float)m_vkr->window_extent.height, .1f, 200.f);
        glm::mat4 V          = glm::lookAt(glm::vec3(0.f, 0.f, 2.0f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f));

        projection[1][1] *= -1;

        data.projection = projection * V;
        data.view       = view;
        data.viewproj   = projection * view;
        copyDataPtr(m_vkr->frame_idx_inflight);
    }

    void UnmapDataPtr(VmaAllocator allocator, uint32_t index)
    {
        vmaUnmapMemory(allocator, m_UBO[index].allocation);
    }

    /**
     * @brief A convevience function to unmap every data ptr
     */
    void UnmapDataPtrs(VmaAllocator allocator)
    {
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
            vmaUnmapMemory(allocator, m_UBO[i].allocation);
        }
    }

    void copyDataPtr(uint32_t frame_idx)
    {
        memcpy(m_data_ptr[frame_idx], &data, sizeof(data));
    }

    void DestroyBuffers(VkDevice device)
    {
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
            vkDestroyBuffer(device, m_UBO[i].buffer, NULL);
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
            vmaDestroyBuffer(allocator, m_UBO[i].buffer, m_UBO[i].allocation);
        }
    }
};

// this number reflects the texture array size set in the "shaders/textureArray.frag" shader
// both must be equal
#define MAX_TEXTURE_COUNT 80

// -1000011001
static std::unordered_map<VkResult, std::string> vulkan_errors = {
    { (VkResult)0, "VK_SUCCESS" },
    { (VkResult)1, "VK_NOT_READY" },
    { (VkResult)2, "VK_TIMEOUT" },
    { (VkResult)3, "VK_EVENT_SET" },
    { (VkResult)4, "VK_EVENT_RESET" },
    { (VkResult)5, "VK_INCOMPLETE" },
    { (VkResult)-1, "VK_ERROR_OUT_OF_HOST_MEMORY" },
    { (VkResult)-2, "VK_ERROR_OUT_OF_DEVICE_MEMORY" },
    { (VkResult)-3, "VK_ERROR_INITIALIZATION_FAILED" },
    { (VkResult)-4, "VK_ERROR_DEVICE_LOST" },
    { (VkResult)-5, "VK_ERROR_MEMORY_MAP_FAILED" },
    { (VkResult)-6, "VK_ERROR_LAYER_NOT_PRESENT" },
    { (VkResult)-7, "VK_ERROR_EXTENSION_NOT_PRESENT" },
    { (VkResult)-8, "VK_ERROR_FEATURE_NOT_PRESENT" },
    { (VkResult)-9, "VK_ERROR_INCOMPATIBLE_DRIVER" },
    { (VkResult)-10, "VK_ERROR_TOO_MANY_OBJECTS" },
    { (VkResult)-11, "VK_ERROR_FORMAT_NOT_SUPPORTED" },
    { (VkResult)-12, "VK_ERROR_FRAGMENTED_POOL" },
    { (VkResult)-13, "VK_ERROR_UNKNOWN" },
    { (VkResult)-1000069000, "VK_ERROR_OUT_OF_POOL_MEMORY" },
    { (VkResult)-1000072003, "VK_ERROR_INVALID_EXTERNAL_HANDLE" },
    { (VkResult)-1000161000, "VK_ERROR_FRAGMENTATION" },
    { (VkResult)-1000257000, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" },
    { (VkResult)-1000000000, "VK_ERROR_SURFACE_LOST_KHR" },
    { (VkResult)-1000000001, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" },
    { (VkResult)1000001003, "VK_SUBOPTIMAL_KHR" },
    { (VkResult)-1000001004, "VK_ERROR_OUT_OF_DATE_KHR" },
    { (VkResult)-1000003001, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" },
    { (VkResult)-1000011001, "VK_ERROR_VALIDATION_FAILED_EXT" },
    { (VkResult)-1000012000, "VK_ERROR_INVALID_SHADER_NV" },
    { (VkResult)-1000158000, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" },
    { (VkResult)-1000174001, "VK_ERROR_NOT_PERMITTED_EXT" },
    { (VkResult)-1000255000, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" },
    { (VkResult)1000268000, "VK_THREAD_IDLE_KHR" },
    { (VkResult)1000268001, "VK_THREAD_DONE_KHR" },
    { (VkResult)1000268002, "VK_OPERATION_DEFERRED_KHR" },
    { (VkResult)1000268003, "VK_OPERATION_NOT_DEFERRED_KHR" },
    { (VkResult)1000297000, "VK_PIPELINE_COMPILE_REQUIRED_EXT" },
    { VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY_KHR" },
    { VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR" },
    { VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION_EXT" },
    { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" },
    { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR" },
    { VK_PIPELINE_COMPILE_REQUIRED_EXT, "VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT" },
    { (VkResult)0x7FFFFFFF, "VK_RESULT_MAX_ENUM" }
};

#define VK_CHECK(x)                                                           \
    do {                                                                      \
        VkResult err = x;                                                     \
        if (err) {                                                            \
            SDL_Log("Detected Vulkan error: %s", vulkan_errors[err].c_str()); \
            abort();                                                          \
        }                                                                     \
    } while (0)

#endif // VK_RENDERER
