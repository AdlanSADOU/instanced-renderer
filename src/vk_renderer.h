// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#if !defined(VK_RENDERER)
#define VK_RENDERER

#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

#include <vk_mem_alloc.h>
#include <vk_mesh.h>

#include <vk_types.h>

#define SECONDS(value) (1000000000 * value)

struct GPUCameraData
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewproj;
};

struct FrameData
{
    VkSemaphore present_semaphore, render_semaphore;
    VkFence     render_fence;

    VkCommandPool   _commandPool;
    VkCommandBuffer main_command_buffer;

    AllocatedBuffer cameraBuffer;
    VkDescriptorSet globalDescriptor;
};

struct Material
{
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
    Mesh     *mesh;
    Material *material;
    glm::mat4 transformMatrix;
};

struct MeshPushConstants
{
    glm::vec4 data;
    glm::mat4 render_matrix;
};

class PipelineBuilder;

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

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

static void load_meshes();
static void create_mesh(Mesh &mesh);

// create material and add it to the map
static Material     *create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);
static Material     *get_material(const std::string &name);
static Mesh         *get_mesh(const std::string &name);
static RenderObject *get_renderable(std::string name);
static bool          load_shader_module(const char *filepath, VkShaderModule *outShaderModule);
static FrameData    &frame_current_get();
static void          create_buffer(AllocatedBuffer *newBuffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
static void          draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);
static void          init_scene();

void vk_Init();
void vk_Render();
void UpdateAndRender();
void vk_Cleanup();

constexpr unsigned int FRAME_OVERLAP = 2;

class PipelineBuilder {
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

#endif // VK_RENDERER
