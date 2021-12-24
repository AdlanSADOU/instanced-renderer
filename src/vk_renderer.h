// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#if !defined(VK_RENDERER)
#define VK_RENDERER
#include <SDL.h>
#include <SDL_vulkan.h>

#include <deque>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <vk_mem_alloc.h>
#include <vk_types.h>

#define VK_CHECK(x)                                    \
    do {                                               \
        VkResult err = x;                              \
        if (err) {                                     \
            SDL_Log("Detected Vulkan error: %d", err); \
            abort();                                   \
        }                                              \
    } while (0)

#define SECONDS(value) (1000000000 * value)
const uint32_t FRAME_OVERLAP = 2;

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

struct PipelineBuilder
{
    std::vector<VkPipelineShaderStageCreateInfo> create_info_shader_stages                = {};
    VkPipelineVertexInputStateCreateInfo         create_info_vertex_input                 = {};
    VkPipelineInputAssemblyStateCreateInfo       create_info_input_assembly_state         = {};
    VkPipelineRasterizationStateCreateInfo       create_info_rasterizer                   = {};
    VkPipelineColorBlendAttachmentState          create_info_attachment_state_color_blend = {};
    VkPipelineMultisampleStateCreateInfo         create_info_multisample_state            = {};
    VkPipelineDepthStencilStateCreateInfo        create_info_depth_stencil_state          = {};
    VkPipelineLayout                             create_info_pipeline_layout              = {};
    VkViewport                                   viewport                                 = {};
    VkRect2D                                     scissor                                  = {};

    VkPipeline buildPipeline(VkDevice device, VkRenderPass render_pass);
};

FrameData           &get_CurrentFrame();
static Mesh         *get_Mesh(const std::string &name);
static Material     *get_Material(const std::string &name);
static RenderObject *get_Renderable(std::string name);

static bool init_ShaderModule(const char *filepath, VkShaderModule *outShaderModule);
static void init_Scene();
static void init_Meshes();

// create material and add it to the map
static Material *create_Material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);
static void      create_Mesh(Mesh &mesh);
void             create_Buffer(BufferObject *newBuffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

static void draw_Renderables(VkCommandBuffer cmd, RenderObject *first, int count);

void vk_Init();
void vk_BeginPass();
void UpdateAndRender();
void vk_Cleanup();

// temporary examples
void draw_examples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer);
void init_Examples(VkDevice device, VkPhysicalDevice g_chosen_gpu);

// Helper functions
bool     allocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory);
uint32_t findProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties);

#endif // VK_RENDERER
