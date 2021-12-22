
#include "vk_renderer.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/gtx/transform.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include <VkBootstrap.h>

#include <fstream>
#include <iomanip>
#include <iostream>

// we want to immediately abort when there is an error.
//  In normal engines this would give an error message to the user
//  or perform a dump of state.
using namespace std;
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            std::cout << "Detected Vulkan error: " << err << std::endl; \
            abort();                                                    \
        }                                                               \
    } while (0)

void VulkanRenderer::init()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
    _window                      = SDL_CreateWindow(
                             "Vulkan Engine",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             _windowExtent.width,
                             _windowExtent.height,
                             window_flags);

    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
    init_descriptors();
    init_pipelines();

    load_meshes();

    init_scene();

    _isInitialized = true;
}

// #if defined(_DEBUG)
// #undef _DEBUG
// #endif // _DEBUG

void VulkanRenderer::init_vulkan()
{
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Awesome Vulkan App")
#if defined(_DEBUG)
                        .request_validation_layers(true)
#else
                        .request_validation_layers(false)
#endif // _DEBUG
                        .require_api_version(1, 1, 0)
                        .use_default_debug_messenger()
                        .build();

    //
    // Instance / Surface / Physical Device selection
    //
    vkb::Instance vkb_inst = inst_ret.value();
    _instance              = vkb_inst.instance;
    _debug_messenger       = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice         physicalDevice = selector
                                             .set_minimum_version(1, 1)
                                             .set_surface(_surface)
                                             .select()
                                             .value();

    //
    // Device
    //
    vkb::DeviceBuilder deviceBuilder { physicalDevice };
    vkb::Device        vkbDevice = deviceBuilder.build().value();

    _device    = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    //
    // Graphics queue
    //
    _graphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    //
    // Allocator
    //
    VmaAllocatorCreateInfo allocatorInfo = {};

    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device         = _device;
    allocatorInfo.instance       = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void VulkanRenderer::init_swapchain()
{
    vkb::SwapchainBuilder swapchainBuilder { _chosenGPU, _device, _surface };

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      .use_default_format_selection()
                                      // use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(_windowExtent.width, _windowExtent.height)
                                      .build()
                                      .value();

    // store swapchain and its related images
    _swapchain            = vkbSwapchain.swapchain;
    _swapchainImages      = vkbSwapchain.get_images().value();
    _swapchainImageViews  = vkbSwapchain.get_image_views().value();
    _swapchainImageFormat = vkbSwapchain.image_format;

    _mainDeletionQueue.push_function([=]() {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    });

    // depth image size will match the window
    VkExtent3D depthImageExtent = {
        _windowExtent.width,
        _windowExtent.height,
        1
    };

    // hardcoding the depth format to 32 bit float
    _depthFormat = VK_FORMAT_D32_SFLOAT;

    // the depth image will be an image with the format we selected and Depth Attachment usage flag
    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    // for the depth image, we want to allocate it from GPU local memory
    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

    // build an image-view for the depth image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

    // add to deletion queues
    _mainDeletionQueue.push_function([=]() {
        vkDestroyImageView(_device, _depthImageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
    });
}

void VulkanRenderer::init_commands()
{

    //
    // CommandPools & CommandBuffers creation
    //
    // create a command pool for commands submitted to the graphics queue.
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
        _graphicsQueueFamily,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

        _mainDeletionQueue.push_function([=]() {
            vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
        });
    }
}

void VulkanRenderer::init_default_renderpass()
{

    //
    // Color attachment
    //
    VkAttachmentDescription color_attachment = {};
    color_attachment.format                  = _swapchainImageFormat; // the attachment will have the format needed by the swapchain
    color_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT; // 1 sample, we won't be doing MSAA
    color_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR; // we Clear when this attachment is loaded
    color_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE; // we keep the attachment stored when the renderpass ends
    color_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about stencil
    color_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED; // we don't know or care about the starting layout of the attachment
    color_attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // after the renderpass ends, the image has to be on a layout ready for display

    //
    // Depth attachment
    //
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags                   = 0;
    depth_attachment.format                  = _depthFormat;
    depth_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //
    // Subpass description
    //
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment            = 0; // attachment number will index into the pAttachments array in the parent renderpass itself
    color_attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment            = 1;
    depth_attachment_ref.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // we are going to create 1 subpass, which is the minimum you can do
    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //
    // RenderPass creation
    //
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};

    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2; // connect the color attachment to the info
    render_pass_info.pAttachments    = &attachments[0];
    render_pass_info.subpassCount    = 1; // connect the subpass to the info
    render_pass_info.pSubpasses      = &subpass;

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(_device, _renderPass, nullptr);
    });
}

void VulkanRenderer::init_framebuffers()
{

    //
    // Framebuffer initialization
    //
    // create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext                   = nullptr;

    fb_info.renderPass      = _renderPass;
    fb_info.attachmentCount = 1;
    fb_info.width           = _windowExtent.width;
    fb_info.height          = _windowExtent.height;
    fb_info.layers          = 1;

    // grab how many images we have in the swapchain
    const size_t swapchain_imagecount = _swapchainImages.size();
    _framebuffers                       = std::vector<VkFramebuffer>(swapchain_imagecount);

    // create framebuffers for each of the swapchain image views
    for (size_t i = 0; i < swapchain_imagecount; i++) {

        VkImageView attachments[2] = {};

        attachments[0] = _swapchainImageViews[i];
        attachments[1] = _depthImageView;

        fb_info.pAttachments    = attachments;
        fb_info.attachmentCount = 2;

        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

        _mainDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        });
    }
}

void VulkanRenderer::init_sync_structures()
{

    //
    // Fence creation
    //
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext             = nullptr;
    // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < FRAME_OVERLAP; i++) {
        /* code */

        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

        //
        // Semaphore creation
        //
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext                 = nullptr;
        semaphoreCreateInfo.flags                 = 0;

        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

        _mainDeletionQueue.push_function([=]() {
            vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
            vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
        });
    }
}

bool VulkanRenderer::load_shader_module(const char *filepath, VkShaderModule *outShaderModule)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) return false;

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext                    = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode    = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

void VulkanRenderer::init_descriptors()
{

    // create a descriptor pool that will hold 10 uniform buffers
    std::vector<VkDescriptorPoolSize> sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = 0;
    pool_info.maxSets                    = 10;
    pool_info.poolSizeCount              = (uint32_t)sizes.size();
    pool_info.pPoolSizes                 = sizes.data();

    vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool);

    // information about the binding.
    VkDescriptorSetLayoutBinding camBufferBinding = {};
    camBufferBinding.binding                      = 0;
    camBufferBinding.descriptorCount              = 1;
    // it's a uniform buffer binding
    camBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    // we use it from the vertex shader
    camBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo setInfo = {};
    setInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext                           = nullptr;

    // we are going to have 1 binding
    setInfo.bindingCount = 1;
    // no flags
    setInfo.flags = 0;
    // point to the camera buffer binding
    setInfo.pBindings = &camBufferBinding;

    vkCreateDescriptorSetLayout(_device, &setInfo, nullptr, &_globalSetLayout);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i].cameraBuffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // allocate one descriptor set for each frame
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.pNext                       = nullptr;
        allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        // using the pool we just set
        allocInfo.descriptorPool = _descriptorPool;
        // only 1 descriptor
        allocInfo.descriptorSetCount = 1;
        // using the global data layout
        allocInfo.pSetLayouts = &_globalSetLayout;

        vkAllocateDescriptorSets(_device, &allocInfo, &_frames[i].globalDescriptor);

        // information about the buffer we want to point at in the descriptor
        VkDescriptorBufferInfo binfo;
        // it will be the camera buffer
        binfo.buffer = _frames[i].cameraBuffer._buffer;
        // at 0 offset
        binfo.offset = 0;
        // of the size of a camera data struct
        binfo.range = sizeof(GPUCameraData);

        VkWriteDescriptorSet setWrite = {};
        setWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        setWrite.pNext                = nullptr;

        // we are going to write into binding number 0
        setWrite.dstBinding = 0;
        // of the global descriptor
        setWrite.dstSet = _frames[i].globalDescriptor;

        setWrite.descriptorCount = 1;
        // and the type is uniform buffer
        setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        setWrite.pBufferInfo    = &binfo;

        vkUpdateDescriptorSets(_device, 1, &setWrite, 0, nullptr);
    }
}

void VulkanRenderer::init_pipelines()
{
    //
    // ShaderModule loading
    //
    VkShaderModule vertexShader;
    if (!load_shader_module("../shaders/triangleMesh.vert.spv", &vertexShader)) {
        std::cout << "Error when building the vertex shader module. Did you compile the shaders?" << std::endl;

    } else {
        std::cout << "fragment shader succesfully loaded" << std::endl;
    }

    VkShaderModule fragmentShader;
    if (!load_shader_module("../shaders/coloredTriangle.frag.spv", &fragmentShader)) {
        std::cout << "Error when building the fragment shader module. Did you compile the shaders?" << std::endl;
    } else {
        std::cout << "vertex fragment shader succesfully loaded" << std::endl;
    }

    // this is where we provide the vertex shader with our matrices
    VkPushConstantRange push_constant;
    push_constant.offset     = 0;
    push_constant.size       = sizeof(MeshPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //
    // PipelineLayout
    //
    VkPipelineLayout pipelineLayout;
    // build the pipeline layout that controls the inputs/outputs of the shader
    // we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();

    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges    = &push_constant;

    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts    = &_globalSetLayout;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &pipelineLayout));

    //
    // Pipeline creation
    //
    PipelineBuilder pipelineBuilder;
    VkPipeline      pipeline;
    //
    // Depth attachment
    //
    // connect the pipeline builder vertex input info to the one we get from Vertex
    VertexInputDescription vertexDescription = Vertex::get_vertex_description();
    pipelineBuilder._vertexInputInfo         = vkinit::vertex_input_state_create_info();

    pipelineBuilder._vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions    = vertexDescription.attributes.data();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions    = vertexDescription.bindings.data();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();
    // build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage

    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
    // input assembly is the configuration for drawing triangle lists, strips, or individual points.
    // we are just going to draw triangle list
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // build viewport and scissor from the swapchain extents
    pipelineBuilder._viewport.x        = 0.0f;
    pipelineBuilder._viewport.y        = 0.0f;
    pipelineBuilder._viewport.width    = (float)_windowExtent.width;
    pipelineBuilder._viewport.height   = (float)_windowExtent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;

    pipelineBuilder._scissor.offset = { 0, 0 };
    pipelineBuilder._scissor.extent = _windowExtent;
    // configure the rasterizer to draw filled triangles
    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    // we don't use multisampling, so just run the default one
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
    // a single blend attachment with no blending and writing to RGBA
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

    // use the triangle layout we created
    pipelineBuilder._pipelineLayout = pipelineLayout;
    // finally build the pipeline
    pipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

    // Right now, we are assigning a pipeline to a material. So we don't have any options
    // for setting shaders or whatever else we might wanna customize in the future
    // the @create_material function should probably create it's pipeline in a customizable way
    create_material(pipeline, pipelineLayout, "defaultmesh");

    vkDestroyShaderModule(_device, vertexShader, nullptr);
    vkDestroyShaderModule(_device, fragmentShader, nullptr);

    _mainDeletionQueue.push_function([=]() {
        vkDestroyPipeline(_device, pipeline, nullptr);
        vkDestroyPipelineLayout(_device, pipelineLayout, nullptr);
    });
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &_viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &_scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &_colorBlendAttachment;

    _depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // build the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.stageCount          = (uint32_t)_shaderStages.size();
    pipelineInfo.pStages             = _shaderStages.data();
    pipelineInfo.pVertexInputState   = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState   = &_multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &_depthStencil;
    pipelineInfo.layout              = _pipelineLayout;
    pipelineInfo.renderPass          = pass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline)
        != VK_SUCCESS) {
        std::cout << "failed to create pipline\n";
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    } else {
        return newPipeline;
    }
}

void VulkanRenderer::load_meshes()
{
    Mesh _triangleMesh;
    _triangleMesh._vertices.resize(3);

    _triangleMesh._vertices[0].position = { 0.2f, 0.2f, 0.0f };
    _triangleMesh._vertices[1].position = { -0.2f, 0.2f, 0.0f };
    _triangleMesh._vertices[2].position = { 0.f, -0.2f, 0.0f };

    _triangleMesh._vertices[0].color = { 0.f, 0.2f, 0.0f }; // pure green
    _triangleMesh._vertices[1].color = { 0.f, 0.2f, 0.0f }; // pure green
    _triangleMesh._vertices[2].color = { 0.f, 0.2f, 0.0f }; // pure green
    // we don't care about the vertex normals

    create_mesh(_triangleMesh);

    Mesh _monkeyMesh;
    _monkeyMesh.load_from_obj("../assets/monkey_smooth.obj");
    create_mesh(_monkeyMesh);

    _meshes["monkey"]   = _monkeyMesh;
    _meshes["triangle"] = _triangleMesh;
}

void VulkanRenderer::create_mesh(Mesh &mesh)
{
    // allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
    // this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
        &mesh._vertexBuffer._buffer,
        &mesh._vertexBuffer._allocation,
        nullptr));

    // copy vertex data
    void *data;
    vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data); // gives a ptr to data
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex)); // copies vertices into data

    vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
    // add the destruction of triangle mesh buffer to the deletion queue
    _mainDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
    });
}

Material *VulkanRenderer::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name)
{
    Material mat;
    mat.pipeline       = pipeline;
    mat.pipelineLayout = layout;
    _materials[name]   = mat;
    return &_materials[name];
}

Material *VulkanRenderer::get_material(const std::string &name)
{
    // search for the object, and return nullpointer if not found
    auto it = _materials.find(name);
    if (it == _materials.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

Mesh *VulkanRenderer::get_mesh(const std::string &name)
{
    auto it = _meshes.find(name);
    if (it == _meshes.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}

void VulkanRenderer::draw_objects(VkCommandBuffer cmd, RenderObject *first, int count)
{
    // make a model view matrix for rendering the object
    // camera view
    glm::vec3 camPos = { camera_x, 0.f + camera_y, -30.f + camera_z };

    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    // camera projection
    glm::mat4 projection = glm::perspective(glm::radians(60.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    // fill a GPU camera data struct
    GPUCameraData camData;
    camData.projection = projection;
    camData.view       = view;
    camData.viewproj   = projection * view;

    // and copy it to the buffer
    void *data;
    vmaMapMemory(_allocator, get_current_frame().cameraBuffer._allocation, &data);

    memcpy(data, &camData, sizeof(GPUCameraData));

    vmaUnmapMemory(_allocator, get_current_frame().cameraBuffer._allocation);

    Mesh     *lastMesh     = nullptr;
    Material *lastMaterial = nullptr;

    for (int i = 0; i < count; i++) {
        RenderObject &object = first[i];

        // only bind the pipeline if it doesn't match with the already bound one
        if (object.material != lastMaterial) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_current_frame().globalDescriptor, 0, nullptr);
        }

        glm::mat4 model = object.transformMatrix;
        // final render matrix, that we are calculating on the cpu
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = object.transformMatrix;

        // upload the mesh constants to the GPU via pushconstants
        vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        // only bind the mesh if it's a different one from last bind
        if (object.mesh != lastMesh) {
            // bind the mesh vertex buffer with offset 0
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
            lastMesh = object.mesh;
        }
        // we can now draw
        uint32_t size = static_cast<uint32_t>(object.mesh->_vertices.size());
        vkCmdDraw(cmd, size, 1, 0, 0);
    }
}

void VulkanRenderer::init_scene()
{
    RenderObject monkey;
    monkey.mesh            = get_mesh("monkey");
    monkey.material        = get_material("defaultmesh");
    monkey.transformMatrix = glm::mat4 { 1.0f };

    int count              = 0;
    _renderables.push_back(monkey);

    for (int x = -10; x <= 10; x++) {
        for (int y = -10; y <= 10; y++) {
            for (int z = -10; z <= 10; z++) {

                RenderObject tri;
                tri.mesh              = get_mesh("triangle");
                tri.material          = get_material("defaultmesh");
                glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(x, z, y));
                glm::mat4 scale       = glm::scale(glm::mat4 { 1.0 }, glm::vec3(0.2, 0.2, 0.2));
                tri.transformMatrix   = translation * scale;

                _renderables.push_back(tri);
                ++count;
            }
        }
    }
    SDL_Log("%d objects", count);
}

RenderObject *VulkanRenderer::get_renderable(std::string name)
{
    for (size_t i = 0; i < _renderables.size(); i++) {
        if (_renderables[i].mesh == get_mesh(name))
            return &_renderables[i];
    }
    return nullptr;
}

FrameData &VulkanRenderer::get_current_frame()
{
    return _frames[_frameNumber % FRAME_OVERLAP];
}

AllocatedBuffer VulkanRenderer::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    // allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = nullptr;

    bufferInfo.size  = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memoryUsage;

    AllocatedBuffer newBuffer;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
        &newBuffer._buffer,
        &newBuffer._allocation,
        nullptr));
    _mainDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(_allocator, newBuffer._buffer, newBuffer._allocation);
    });

    return newBuffer;
}

void VulkanRenderer::cleanup()
{
    if (_isInitialized) {
        vkWaitForFences(_device, 1, &_frames[0]._renderFence, true, 1000000000);
        vkWaitForFences(_device, 1, &_frames[1]._renderFence, true, 1000000000);

        _mainDeletionQueue.flush();

        vmaDestroyAllocator(_allocator);

        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }
}

void VulkanRenderer::draw()
{
    // wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    // request image from the swapchain, one second timeout
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));

    // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

    //
    // Begin the command buffer recording
    //
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext                    = nullptr;
    cmdBeginInfo.pInheritanceInfo         = nullptr;
    cmdBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We will use this command buffer exactly once (per frame)

    VK_CHECK(vkBeginCommandBuffer(get_current_frame()._mainCommandBuffer, &cmdBeginInfo)); // start CmdBuffer recording

    //
    // BEGIN renderpass
    //
    // start the main renderpass.
    // We will use the clear color from above, and the framebuffer of the index the swapchain gave us
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext                 = nullptr;
    rpInfo.renderPass            = _renderPass;
    rpInfo.renderArea.offset.x   = 0;
    rpInfo.renderArea.offset.y   = 0;
    rpInfo.renderArea.extent     = _windowExtent;
    rpInfo.framebuffer           = _framebuffers[swapchainImageIndex];

    VkClearValue clearValue;
    float        flash = abs(sin(_frameNumber / 120.f));
    clearValue.color   = { { 0.0f, 0.0f, flash, 1.0f } };

    // Clear depth at 1
    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    // connect clear values
    rpInfo.clearValueCount     = 2;
    VkClearValue clearValues[] = { clearValue, depthClear };
    rpInfo.pClearValues        = &clearValues[0];

    vkCmdBeginRenderPass(get_current_frame()._mainCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    draw_objects(get_current_frame()._mainCommandBuffer, _renderables.data(), (uint32_t)_renderables.size());

    ///
    // END renderpass & command buffer recording
    ///
    vkCmdEndRenderPass(get_current_frame()._mainCommandBuffer);
    VK_CHECK(vkEndCommandBuffer(get_current_frame()._mainCommandBuffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext        = nullptr;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit_info.pWaitDstStageMask = &waitStage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = &get_current_frame()._presentSemaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &get_current_frame()._renderSemaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &get_current_frame()._mainCommandBuffer;

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit_info, get_current_frame()._renderFence));

    // this will put the image we just rendered into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = nullptr;

    presentInfo.pSwapchains    = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores    = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber++;
}

void VulkanRenderer::run()
{
    SDL_Event e;
    bool      bQuit = false;

    while (!bQuit) {

        while (SDL_PollEvent(&e) != 0) {
            SDL_Keycode key = e.key.keysym.sym;
            if (e.type == SDL_QUIT) bQuit = true;

            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE) bQuit = true;

                if (key == SDLK_UP) _up = false;
                if (key == SDLK_DOWN) _down = false;
                if (key == SDLK_LEFT) _left = false;
                if (key == SDLK_RIGHT) _right = false;

                if (key == SDLK_w) _W = false;
                if (key == SDLK_s) _S = false;
                if (key == SDLK_a) _A = false;
                if (key == SDLK_d) _D = false;
            }
            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP) _up = true;
                if (key == SDLK_DOWN) _down = true;
                if (key == SDLK_LEFT) _left = true;
                if (key == SDLK_RIGHT) _right = true;

                if (key == SDLK_w) _W = true;
                if (key == SDLK_s) _S = true;
                if (key == SDLK_a) _A = true;
                if (key == SDLK_d) _D = true;
            }
        }

        if (_up) camera_z += .1f;
        if (_down) camera_z -= .1f;
        if (_left) camera_x -= .1f;
        if (_right) camera_x += .1f;

        static float posX = 0;
        static float posY = 0;

        if (_W) posY += .3f;
        if (_S) posY -= .3f;
        if (_A) posX -= .3f;
        if (_D) posX += .3f;

        static RenderObject *monkey = get_renderable("monkey");

        monkey->transformMatrix = glm::translate(monkey->transformMatrix, glm::vec3(posX, posY, 0));

        posX = posY = 0;

        draw();
    }
}
