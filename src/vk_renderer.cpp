
#include "vk_renderer.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vk_initializers.h>
#include <vk_types.h>

// todo(ad): this dependency needs to be dropped at some point
#include <VkBootstrap.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <time.h>


SDL_Window *g_window = NULL;

//
// Vulkan initialization phase types
//
VkInstance               g_instance;
VkDebugUtilsMessengerEXT g_debug_messenger;
VkPhysicalDevice         g_chosen_gpu;
VkSurfaceKHR             g_surface;

//
// Swapchain
//
VkSwapchainKHR           g_swapchain;
VkFormat                 g_swapchain_image_format; // image format expected by the windowing system
std::vector<VkImage>     g_swapchain_images;
std::vector<VkImageView> g_swapchain_image_views;

uint32_t g_graphics_queue_family; // family of that queue
VkQueue  _graphicsQueue; // queue we will submit to

FrameData g_frames[FRAME_OVERLAP];

VkDevice              g_device;
VkPipeline            g_default_pipeline;
VkPipelineLayout      g_default_pipeline_layout;
VkDescriptorSetLayout g_global_set_layout;

//
// Descriptor sets
//
VkDescriptorPool g_descriptor_pool;

//
// RenderPass & Framebuffers
//
VkRenderPass               g_render_pass; // created before renderpass
std::vector<VkFramebuffer> g_framebuffers; // these are created for a specific renderpass

VkImageView    g_depth_image_view;
AllocatedImage g_depth_image;
VkFormat       g_depth_format;

ReleaseQueue g_main_deletion_queue;
VmaAllocator g_allocator;

// default array of renderable objects
std::vector<RenderObject> g_renderables;

std::unordered_map<std::string, Material> g_materials;
std::unordered_map<std::string, Mesh>     g_meshes;

bool     g_is_initialized     = false;
uint64_t g_frame_idx_inflight = 0;

VkExtent2D g_window_extent { 1700, 900 };

// we want to immediately abort when there is an error.
//  In normal engines this would give an error message to the user
//  or perform a dump of state.
using namespace std;

void vk_Init()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
    g_window                     = SDL_CreateWindow(
                            "Vulkan Engine",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            g_window_extent.width,
                            g_window_extent.height,
                            window_flags);

    // Vulkan Initialization
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Awesome Vulkan App")
#if defined(_DEBUG)
                        .request_validation_layers(true)
#else
                        .request_validation_layers(false)
#endif // _DEBUG
                        .require_api_version(1, 2, 0)
                        .use_default_debug_messenger()
                        .build();

    //
    // Instance / Surface / Physical Device selection
    //
    vkb::Instance vkb_inst = inst_ret.value();
    g_instance             = vkb_inst.instance;
    g_debug_messenger      = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(g_window, g_instance, &g_surface);

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice         physicalDevice = selector
                                             .set_minimum_version(1, 1)
                                             .set_surface(g_surface)
                                             .select()
                                             .value();

    //
    // Device
    //
    vkb::DeviceBuilder deviceBuilder { physicalDevice };
    vkb::Device        vkbDevice = deviceBuilder.build().value();

    g_device     = vkbDevice.device;
    g_chosen_gpu = physicalDevice.physical_device;

    //
    // Graphics queue
    //
    _graphicsQueue          = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    g_graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    //
    // Allocator
    //
    VmaAllocatorCreateInfo allocatorInfo = {};

    allocatorInfo.physicalDevice = g_chosen_gpu;
    allocatorInfo.device         = g_device;
    allocatorInfo.instance       = g_instance;
    vmaCreateAllocator(&allocatorInfo, &g_allocator);

    ///////////////////////////////////////////
    // Swapchain creation
    // todo(ad): relying on vkb (vkbootstrap) for now
    vkb::SwapchainBuilder swapchainBuilder { g_chosen_gpu, g_device, g_surface };
    vkb::Swapchain        vkbSwapchain = swapchainBuilder
                                      .use_default_format_selection()
                                      // use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(g_window_extent.width, g_window_extent.height)
                                      .build()
                                      .value();

    // store swapchain and its related images
    g_swapchain              = vkbSwapchain.swapchain;
    g_swapchain_images       = vkbSwapchain.get_images().value();
    g_swapchain_image_views  = vkbSwapchain.get_image_views().value();
    g_swapchain_image_format = vkbSwapchain.image_format;

    g_main_deletion_queue.push_function([=]() {
        vkDestroySwapchainKHR(g_device, g_swapchain, NULL);
    });

    // depth image size will match the window
    VkExtent3D depthImageExtent = {
        g_window_extent.width,
        g_window_extent.height,
        1
    };

    // hardcoding the depth format to 32 bit float
    g_depth_format = VK_FORMAT_D32_SFLOAT;

    // the depth image will be an image with the format we selected and Depth Attachment usage flag
    VkImageCreateInfo dimg_info = vkinit::image_create_info(g_depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    // for the depth image, we want to allocate it from GPU local memory
    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(g_allocator, &dimg_info, &dimg_allocinfo, &g_depth_image._image, &g_depth_image._allocation, NULL);

    // build an image-view for the depth image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(g_depth_format, g_depth_image._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(g_device, &dview_info, NULL, &g_depth_image_view));

    // add to deletion queues
    // this is dandy and all, but...
    g_main_deletion_queue.push_function([=]() {
        vkDestroyImageView(g_device, g_depth_image_view, NULL);
        vmaDestroyImage(g_allocator, g_depth_image._image, g_depth_image._allocation);
    });

    //////////////////////////////////////////////////////
    // CommandPools & CommandBuffers creation
    // create a command pool for commands submitted to the graphics queue.
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
        g_graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(g_device, &commandPoolInfo, NULL, &g_frames[i].command_pool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(g_frames[i].command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &g_frames[i].main_command_buffer));

        g_main_deletion_queue.push_function([=]() {
            vkDestroyCommandPool(g_device, g_frames[i].command_pool, NULL);
        });
    }

    /////////////////////////////////////////////////////
    // Default renderpass
    //
    // Color attachment
    //
    VkAttachmentDescription color_attachment = {};
    color_attachment.format                  = g_swapchain_image_format; // the attachment will have the format needed by the swapchain
    color_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT; // 1 sample, we won't be doing MSAA
    color_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR; // we Clear when this attachment is loaded
    color_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE; // we keep the attachment stored when the renderpass ends
    color_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about stencil
    color_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED; // we don't know or care about the starting layout of the attachment
    color_attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // after the renderpass ends, the image has to be on a layout ready for display

    /////
    // Depth attachment
    //
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags                   = 0;
    depth_attachment.format                  = g_depth_format;
    depth_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /////
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

    /////
    // RenderPass creation
    //
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};

    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2; // connect the color attachment to the info
    render_pass_info.pAttachments    = &attachments[0];
    render_pass_info.subpassCount    = 1; // connect the subpass to the info
    render_pass_info.pSubpasses      = &subpass;

    VK_CHECK(vkCreateRenderPass(g_device, &render_pass_info, NULL, &g_render_pass));

    g_main_deletion_queue.push_function([=]() {
        vkDestroyRenderPass(g_device, g_render_pass, NULL);
    });

    ///////////////////////////////////////////////
    // Framebuffer initialization
    //
    // create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext                   = NULL;

    fb_info.renderPass      = g_render_pass;
    fb_info.attachmentCount = 1;
    fb_info.width           = g_window_extent.width;
    fb_info.height          = g_window_extent.height;
    fb_info.layers          = 1;

    // grab how many images we have in the swapchain
    const size_t swapchain_imagecount = g_swapchain_images.size();
    g_framebuffers                    = std::vector<VkFramebuffer>(swapchain_imagecount);

    // create framebuffers for each of the swapchain image views
    for (size_t i = 0; i < swapchain_imagecount; i++) {

        VkImageView attachments[2] = {};

        attachments[0] = g_swapchain_image_views[i];
        attachments[1] = g_depth_image_view;

        fb_info.pAttachments    = attachments;
        fb_info.attachmentCount = 2;

        VK_CHECK(vkCreateFramebuffer(g_device, &fb_info, NULL, &g_framebuffers[i]));

        g_main_deletion_queue.push_function([=]() {
            vkDestroyFramebuffer(g_device, g_framebuffers[i], NULL);
            vkDestroyImageView(g_device, g_swapchain_image_views[i], NULL);
        });
    }

    ////////////////////////////////////////////////////////////////
    // Sync structures
    //
    // Fence creation
    //
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext             = NULL;
    // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < FRAME_OVERLAP; i++) {
        /* code */

        VK_CHECK(vkCreateFence(g_device, &fenceCreateInfo, NULL, &g_frames[i].render_fence));

        /////
        // Semaphores creation
        //
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext                 = NULL;
        semaphoreCreateInfo.flags                 = 0;

        VK_CHECK(vkCreateSemaphore(g_device, &semaphoreCreateInfo, NULL, &g_frames[i].present_semaphore));
        VK_CHECK(vkCreateSemaphore(g_device, &semaphoreCreateInfo, NULL, &g_frames[i].render_semaphore));

        g_main_deletion_queue.push_function([=]() {
            vkDestroyFence(g_device, g_frames[i].render_fence, NULL);
            vkDestroySemaphore(g_device, g_frames[i].present_semaphore, NULL);
            vkDestroySemaphore(g_device, g_frames[i].render_semaphore, NULL);
        });
    }

    /////////////////////////////////////////////////////
    // Descriptors
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

    vkCreateDescriptorPool(g_device, &pool_info, NULL, &g_descriptor_pool);

    // information about the binding.
    VkDescriptorSetLayoutBinding binding_descriptor_set_layout = {};
    binding_descriptor_set_layout.binding                      = 0;
    binding_descriptor_set_layout.descriptorCount              = 1;
    // it's a uniform buffer binding
    binding_descriptor_set_layout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    // we use it from the vertex shader
    binding_descriptor_set_layout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo setInfo = {};
    setInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext                           = NULL;

    // we are going to have 1 binding
    setInfo.bindingCount = 1;
    // no flags
    setInfo.flags = 0;
    // point to the camera buffer binding
    setInfo.pBindings = &binding_descriptor_set_layout;

    vkCreateDescriptorSetLayout(g_device, &setInfo, NULL, &g_global_set_layout);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        create_Buffer(&g_frames[i].camera_buffer, sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // allocate one descriptor set for each frame
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.pNext                       = NULL;
        allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        // using the pool we just set
        allocInfo.descriptorPool = g_descriptor_pool;
        // only 1 descriptor
        allocInfo.descriptorSetCount = 1;
        // using the global data layout
        allocInfo.pSetLayouts = &g_global_set_layout;

        vkAllocateDescriptorSets(g_device, &allocInfo, &g_frames[i].global_descriptor);

        // information about the buffer we want to point at in the descriptor
        VkDescriptorBufferInfo info_descriptor_buffer;
        // it will be the camera buffer
        info_descriptor_buffer.buffer = g_frames[i].camera_buffer.buffer;
        // at 0 offset
        info_descriptor_buffer.offset = 0;
        // of the size of a camera data struct
        info_descriptor_buffer.range = sizeof(CameraData);

        VkWriteDescriptorSet setWrite = {};
        setWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        setWrite.pNext                = NULL;

        // we are going to write into binding number 0
        setWrite.dstBinding = 0;
        // of the global descriptor
        setWrite.dstSet = g_frames[i].global_descriptor;

        setWrite.descriptorCount = 1;
        // and the type is uniform buffer
        setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        setWrite.pBufferInfo    = &info_descriptor_buffer;

        vkUpdateDescriptorSets(g_device, 1, &setWrite, 0, NULL);
    }

    //////////////////////////////////////////////////////
    // Pipeline
    //
    // ShaderModule loading
    //
    VkShaderModule vertexShader;
    if (!init_ShaderModule("../shaders/triangleMesh.vert.spv", &vertexShader)) {
        std::cout << "Error when building the vertex shader module. Did you compile the shaders?" << std::endl;

    } else {
        std::cout << "fragment shader succesfully loaded" << std::endl;
    }

    VkShaderModule fragmentShader;
    if (!init_ShaderModule("../shaders/coloredTriangle.frag.spv", &fragmentShader)) {
        std::cout << "Error when building the fragment shader module. Did you compile the shaders?" << std::endl;
    } else {
        std::cout << "vertex fragment shader succesfully loaded" << std::endl;
    }

    // this is where we provide the vertex shader with our matrices
    VkPushConstantRange push_constant;
    push_constant.offset     = 0;
    push_constant.size       = sizeof(MeshPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    ///////
    // PipelineLayout
    //
    VkPipelineLayout pipelineLayout;
    // build the pipeline layout that controls the inputs/outputs of the shader
    // we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();

    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges    = &push_constant;

    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts    = &g_global_set_layout;

    VK_CHECK(vkCreatePipelineLayout(g_device, &pipeline_layout_info, NULL, &pipelineLayout));
    VK_CHECK(vkCreatePipelineLayout(g_device, &pipeline_layout_info, NULL, &g_default_pipeline_layout));

    /////
    // Pipeline creation
    //
    PipelineBuilder pipelineBuilder;
    VkPipeline      pipeline;
    /////
    // Depth attachment
    //
    // connect the pipeline builder vertex input info to the one we get from Vertex
    VertexInputDescription vertexDescription = Vertex::get_vertex_description();
    pipelineBuilder.create_info_vertex_input = vkinit::vertex_input_state_create_info();

    pipelineBuilder.create_info_vertex_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineBuilder.create_info_vertex_input.pVertexAttributeDescriptions    = vertexDescription.attributes.data();
    pipelineBuilder.create_info_vertex_input.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

    pipelineBuilder.create_info_vertex_input.pVertexBindingDescriptions    = vertexDescription.bindings.data();
    pipelineBuilder.create_info_vertex_input.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();
    // build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage

    pipelineBuilder.create_info_shader_stages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    pipelineBuilder.create_info_shader_stages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
    // input assembly is the configuration for drawing triangle lists, strips, or individual points.
    // we are just going to draw triangle list
    pipelineBuilder.create_info_input_assembly_state = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // build viewport and scissor from the swapchain extents
    pipelineBuilder.viewport.x        = 0.0f;
    pipelineBuilder.viewport.y        = 0.0f;
    pipelineBuilder.viewport.width    = (float)g_window_extent.width;
    pipelineBuilder.viewport.height   = (float)g_window_extent.height;
    pipelineBuilder.viewport.minDepth = 0.0f;
    pipelineBuilder.viewport.maxDepth = 1.0f;

    pipelineBuilder.scissor.offset = { 0, 0 };
    pipelineBuilder.scissor.extent = g_window_extent;
    // configure the rasterizer to draw filled triangles
    pipelineBuilder.create_info_rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    // we don't use multisampling, so just run the default one
    pipelineBuilder.create_info_multisample_state = vkinit::multisampling_state_create_info();
    // a single blend attachment with no blending and writing to RGBA
    pipelineBuilder.create_info_attachment_state_color_blend = vkinit::color_blend_attachment_state();

    // use the triangle layout we created
    pipelineBuilder.create_info_pipeline_layout = pipelineLayout;
    // finally build the pipeline
    pipeline           = pipelineBuilder.buildPipeline(g_device, g_render_pass);
    g_default_pipeline = pipelineBuilder.buildPipeline(g_device, g_render_pass);

    // Right now, we are assigning a pipeline to a material. So we don't have any options
    // for setting shaders or whatever else we might wanna customize in the future
    // the @create_Material function should probably create it's pipeline in a customizable way
    create_Material(pipeline, pipelineLayout, "defaultmesh");

    vkDestroyShaderModule(g_device, vertexShader, NULL);
    vkDestroyShaderModule(g_device, fragmentShader, NULL);

    g_main_deletion_queue.push_function([=]() {
        vkDestroyPipeline(g_device, pipeline, NULL);
        vkDestroyPipelineLayout(g_device, pipelineLayout, NULL);
    });

    // todo(ad): these 2 calls are not part of the vulkan initialization phase
    init_Examples(g_device, g_chosen_gpu);
    // init_Meshes();
    // init_Scene();

    g_is_initialized = true;
}

void vk_BeginPass()
{
    // wait until the GPU has finished rendering the last frame. Timeout of 1 second

    VkFence         *inflight_render_fence          = &g_frames[g_frame_idx_inflight % FRAME_OVERLAP].render_fence;
    VkSemaphore     *inflight_present_semaphore     = &g_frames[g_frame_idx_inflight % FRAME_OVERLAP].present_semaphore;
    VkSemaphore     *inflight_render_semaphore      = &g_frames[g_frame_idx_inflight % FRAME_OVERLAP].render_semaphore;
    VkCommandBuffer *inflight_main_command_buffer   = &g_frames[g_frame_idx_inflight % FRAME_OVERLAP].main_command_buffer;
    VkDescriptorSet *inflight_global_descriptor_set = &g_frames[g_frame_idx_inflight % FRAME_OVERLAP].global_descriptor;
    BufferObject    *camera_buffer                  = &g_frames[g_frame_idx_inflight % FRAME_OVERLAP].camera_buffer;

    VK_CHECK(vkWaitForFences(g_device, 1, inflight_render_fence, true, SECONDS(1)));
    VK_CHECK(vkResetFences(g_device, 1, inflight_render_fence));

    // request image from the swapchain, one second timeout
    uint32_t idx_swapchain_image;
    VK_CHECK(vkAcquireNextImageKHR(g_device, g_swapchain, SECONDS(1), *inflight_present_semaphore, NULL, &idx_swapchain_image));

    // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(*inflight_main_command_buffer, 0));

    //
    // Begin the command buffer recording
    //
    VkCommandBufferBeginInfo cmd_begin_info = {};
    cmd_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext                    = NULL;
    cmd_begin_info.pInheritanceInfo         = NULL;
    cmd_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We will use this command buffer exactly once (per frame)

    VK_CHECK(vkBeginCommandBuffer(*inflight_main_command_buffer, &cmd_begin_info)); // start CmdBuffer recording

    //
    // BEGIN renderpass
    //
    // start the main renderpass.
    // We will use the clear color from above, and the framebuffer of the index the swapchain gave us
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext                 = NULL;
    rpInfo.renderPass            = g_render_pass;
    rpInfo.renderArea.offset.x   = 0;
    rpInfo.renderArea.offset.y   = 0;
    rpInfo.renderArea.extent     = g_window_extent;
    rpInfo.framebuffer           = g_framebuffers[idx_swapchain_image];

    VkClearValue clear_value;

    float flash       = abs(sin(g_frame_idx_inflight / 120.f));
    clear_value.color = { { 0.0f, 0.0f, flash, 1.0f } };

    // Clear depth at 1
    VkClearValue depth_clear;
    depth_clear.depth_stencil.depth = 1.f;

    // connect clear values
    rpInfo.clear_value_count          = 2;
    VkClearValue union_clear_values[] = { clear_value, depth_clear };
    rpInfo.pClearValues               = &union_clear_values[0];

    vkCmdBeginRenderPass(*inflight_main_command_buffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    draw_examples(inflight_main_command_buffer, inflight_global_descriptor_set, camera_buffer);
    // draw_Renderables(*inflight_main_command_buffer, g_renderables.data(), (uint32_t)g_renderables.size());

    /////////////////////////////////////////////////////////////////////
    /// Examples

    // vkCmdDrawIndexed(
    // VkCommandBuffer                             commandBuffer,
    // uint32_t                                    indexCount,
    // uint32_t                                    instanceCount,
    // uint32_t                                    firstIndex,
    // int32_t                                     vertexOffset,
    // uint32_t                                    firstInstance);

    ///
    // END renderpass & command buffer recording
    ///
    vkCmdEndRenderPass(*inflight_main_command_buffer);
    VK_CHECK(vkEndCommandBuffer(*inflight_main_command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext        = NULL;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit_info.pWaitDstStageMask = &waitStage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = inflight_present_semaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = inflight_render_semaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = inflight_main_command_buffer;

    // submit command buffer to the queue and execute it.
    //  inflight_render_fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit_info, *inflight_render_fence));

    // this will put the image we just rendered into the visible window.
    // we want to inflight_render_semaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = NULL;

    presentInfo.pSwapchains    = &g_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores    = inflight_render_semaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &idx_swapchain_image;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    g_frame_idx_inflight++;
}

VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass)
{
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = NULL;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = NULL;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &create_info_attachment_state_color_blend;

    create_info_depth_stencil_state = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // build the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = NULL;

    pipelineInfo.stageCount          = (uint32_t)create_info_shader_stages.size();
    pipelineInfo.pStages             = create_info_shader_stages.data();
    pipelineInfo.pVertexInputState   = &create_info_vertex_input;
    pipelineInfo.pInputAssemblyState = &create_info_input_assembly_state;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &create_info_rasterizer;
    pipelineInfo.pMultisampleState   = &create_info_multisample_state;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &create_info_depth_stencil_state;
    pipelineInfo.layout              = create_info_pipeline_layout;
    pipelineInfo.renderPass          = pass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &newPipeline)
        != VK_SUCCESS) {
        std::cout << "failed to create pipline\n";
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    } else {
        return newPipeline;
    }
}

void init_Meshes()
{
    Mesh mesh_triangle;
    mesh_triangle.vertices.resize(3);

    mesh_triangle.vertices[0].position = { 0.2f, 0.2f, 0.0f };
    mesh_triangle.vertices[1].position = { -0.2f, 0.2f, 0.0f };
    mesh_triangle.vertices[2].position = { 0.f, -0.2f, 0.0f };

    mesh_triangle.vertices[0].color = { 0.f, 0.2f, 0.0f }; // pure green
    mesh_triangle.vertices[1].color = { 0.f, 0.2f, 0.0f }; // pure green
    mesh_triangle.vertices[2].color = { 0.f, 0.2f, 0.0f }; // pure green
    // we don't care about the vertex normals

    create_Mesh(mesh_triangle);

    Mesh mesh_monkey;
    mesh_monkey.loadFromObj("../assets/monkey_smooth.obj");
    create_Mesh(mesh_monkey);

    g_meshes["monkey"]   = mesh_monkey;
    g_meshes["triangle"] = mesh_triangle;
}

void create_Mesh(Mesh &mesh)
{
    // allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
    // this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(g_allocator, &bufferInfo, &vmaallocInfo,
        &mesh.vertex_buffer.buffer,
        &mesh.vertex_buffer._allocation,
        NULL));

    // copy vertex data
    void *data;
    vmaMapMemory(g_allocator, mesh.vertex_buffer._allocation, &data); // gives a ptr to data
    memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex)); // copies vertices into data

    vmaUnmapMemory(g_allocator, mesh.vertex_buffer._allocation);
    // add the destruction of triangle mesh buffer to the deletion queue
    g_main_deletion_queue.push_function([=]() {
        vmaDestroyBuffer(g_allocator, mesh.vertex_buffer.buffer, mesh.vertex_buffer._allocation);
    });
}

void init_Scene()
{
    RenderObject monkey;
    monkey.mesh            = get_Mesh("monkey");
    monkey.material        = get_Material("defaultmesh");
    monkey.transformMatrix = glm::mat4 { 1.0f };

    int count = 0;
    g_renderables.push_back(monkey);

    for (int x = -10; x <= 10; x++) {
        for (int y = -10; y <= 10; y++) {
            for (int z = -10; z <= 10; z++) {

                RenderObject tri;
                tri.mesh              = get_Mesh("triangle");
                tri.material          = get_Material("defaultmesh");
                glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(x, z, y));
                glm::mat4 scale       = glm::scale(glm::mat4 { 1.0 }, glm::vec3(0.2, 0.2, 0.2));
                tri.transformMatrix   = translation * scale;

                g_renderables.push_back(tri);
                ++count;
            }
        }
    }
    SDL_Log("%d objects", count);
}

static FrameData &get_CurrentFrame()
{
    return g_frames[g_frame_idx_inflight % FRAME_OVERLAP];
}

bool init_ShaderModule(const char *filepath, VkShaderModule *outShaderModule)
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
    createInfo.pNext                    = NULL;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode    = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(g_device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    *outShaderModule = shaderModule;

    return true;
}

void vk_Cleanup()
{
    if (g_is_initialized) {
        vkWaitForFences(g_device, 1, &g_frames[0].render_fence, true, SECONDS(1));
        vkWaitForFences(g_device, 1, &g_frames[1].render_fence, true, SECONDS(1));

        g_main_deletion_queue.flush();

        vmaDestroyAllocator(g_allocator);

        vkDestroySurfaceKHR(g_instance, g_surface, NULL);
        vkDestroyDevice(g_device, NULL);
        vkb::destroy_debug_utils_messenger(g_instance, g_debug_messenger);
        vkDestroyInstance(g_instance, NULL);
        SDL_DestroyWindow(g_window);
    }
}

// todo(ad): must be removed as we drop depedency on VmaAllocator
void create_Buffer(BufferObject *newBuffer, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    // allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = NULL;

    bufferInfo.size  = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memoryUsage;

    // BufferObject newBuffer;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(g_allocator, &bufferInfo, &vmaallocInfo, &newBuffer->buffer, &newBuffer->_allocation, NULL));

    g_main_deletion_queue.push_function([=]() {
        vmaDestroyBuffer(g_allocator, newBuffer->buffer, newBuffer->_allocation);
    });

    // return newBuffer;
}

///////////////////////////////////////
// todo(ad): must rethink these
Material *create_Material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name)
{
    Material mat;
    mat.pipeline       = pipeline;
    mat.pipelineLayout = layout;
    g_materials[name]  = mat;
    return &g_materials[name];
}

Material *get_Material(const std::string &name)
{
    // search for the object, and return nullpointer if not found
    auto it = g_materials.find(name);
    if (it == g_materials.end()) {
        return NULL;
    } else {
        return &(*it).second;
    }
}

Mesh *get_Mesh(const std::string &name)
{
    auto it = g_meshes.find(name);
    if (it == g_meshes.end()) {
        return NULL;
    } else {
        return &(*it).second;
    }
}

RenderObject *get_Renderable(std::string name)
{
    for (size_t i = 0; i < g_renderables.size(); i++) {
        if (g_renderables[i].mesh == get_Mesh(name))
            return &g_renderables[i];
    }
    return NULL;
}
///////////////////////////////////////