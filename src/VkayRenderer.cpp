﻿
#include "Vkay.h"
#include <VkayInitializers.h>
#include <VkayTypes.h>
#include <fstream>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define DEPTH_FORMAT VK_FORMAT_D16_UNORM


static bool       VkayCreateShaderModule(const char *filepath, VkShaderModule *out_ShaderModule, VkDevice device);
static VkPipeline VkayCreateComputePipeline(VkayRenderer *vkr);

void VkayRendererClearColor(VkayRenderer *vkr, Color color)
{
    vkr->clear_value.color.float32[3] = color.a;
    vkr->clear_value.color.float32[0] = color.r;
    vkr->clear_value.color.float32[1] = color.g;
    vkr->clear_value.color.float32[2] = color.b;
}

static FrameData *VkayRendererGetCurrentFrameData(VkayRenderer *vkr)
{
    return &vkr->frames[vkr->frame_idx_inflight % FRAME_BUFFER_COUNT];
}

static void createFramebuffers(VkDevice device, std::vector<VkFramebuffer> *framebuffers, vkaySwapchain swapchain, VkRenderPass render_pass, VkImageView depth_image_view)
{

#ifndef NOPASS
    for (size_t i = 0; i < framebuffers->size(); i++) {
        vkDestroyFramebuffer(VkayGetContext()->device, (*framebuffers)[i], NULL);
    }


    const size_t swapchain_imagecount = swapchain.images.size();
    framebuffers->resize(swapchain_imagecount);

    // create framebuffers for each of the swapchain image views
    for (size_t i = 0; i < swapchain_imagecount; i++) {

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext                   = NULL;
        fb_info.renderPass              = render_pass;
        fb_info.attachmentCount         = 1;
        fb_info.width                   = VkayGetContext()->window_extent.width;
        fb_info.height                  = VkayGetContext()->window_extent.height;
        fb_info.layers                  = 1;

        VkImageView attachments[2] = {};
        attachments[0]             = swapchain.image_views[i];
        attachments[1]             = depth_image_view;
        fb_info.pAttachments       = attachments;
        fb_info.attachmentCount    = 2;

        VKCHECK(vkCreateFramebuffer(device, &fb_info, NULL, &(*framebuffers)[i]));
    }
#endif
};

void VkayRendererInit(VkayRenderer *vkr)
{
    VkayContext *vkc = VkayGetContext();

    vkr->device         = vkc->device;
    vkr->graphics_queue = vkc->graphics_queue;
    vkr->compute_queue  = vkc->compute_queue;
    vkr->present_queue  = vkc->present_queue;





    ///////////////////////////
    /// VMA Allocator
    VmaAllocatorCreateInfo allocatorInfo = {};

    allocatorInfo.physicalDevice = vkc->physical_device;
    allocatorInfo.device         = vkc->device;
    allocatorInfo.instance       = vkc->instance;
    vmaCreateAllocator(&allocatorInfo, &vkr->allocator);





    ////////////////////////////////////////////
    /// Swapchain
    vkr->swapchain.create(vkc->device, FRAME_BUFFER_COUNT, vkr->allocator, &vkr->depth_image, &vkr->depth_image_view);
    vkr->depth_format = DEPTH_FORMAT;
    //@recreate
    ///////////////////////////////////
    /// Depth Image
    // depth image size will match the window




#ifndef NOPASS
    /////////////////////////////////////////////////////
    // Default renderpass
    // Color attachment
    VkAttachmentDescription color_attachment = {};
    color_attachment.format                  = vkr->swapchain.image_format; // the attachment will have the format needed by the swapchain
    color_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT; // 1 sample, we won't be doing MSAA
    color_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR; // we Clear when this attachment is loaded
    color_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE; // we keep the attachment stored when the renderpass ends
    color_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about stencil
    color_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED; // we don't know or care about the starting layout of the attachment
    color_attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // after the renderpass ends, the image has to be on a layout ready for display

    /////
    // Depth attachment
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags                   = 0;
    depth_attachment.format                  = DEPTH_FORMAT;
    depth_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /////
    // Subpass description
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment            = 0; // attachment number will index into the pAttachments array in the parent renderpass itself
    color_attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment            = 1;
    depth_attachment_ref.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // we have to have at least 1 subpass
    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency attachmentDependencies[2] = {};
    // Depth buffer is shared between swapchain images
    attachmentDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    attachmentDependencies[0].dstSubpass      = 0;
    attachmentDependencies[0].srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    attachmentDependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    attachmentDependencies[0].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    attachmentDependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    attachmentDependencies[0].dependencyFlags = 0;

    // Image Layout Transition
    attachmentDependencies[1].srcSubpass      = VK_SUBPASS_EXTERNAL;
    attachmentDependencies[1].dstSubpass      = 0;
    attachmentDependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    attachmentDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    attachmentDependencies[1].srcAccessMask   = 0;
    attachmentDependencies[1].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    attachmentDependencies[1].dependencyFlags = 0;

    /////
    // RenderPass creation
    VkAttachmentDescription attachment_descriptions[2] = { color_attachment, depth_attachment };
#endif

#ifndef NOPASS
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount        = 2; // connect the color attachment to the info
    render_pass_info.pAttachments           = &attachment_descriptions[0];
    render_pass_info.subpassCount           = 1; // connect the subpass to the info
    render_pass_info.pSubpasses             = &subpass;
    render_pass_info.dependencyCount        = 2;
    render_pass_info.pDependencies          = attachmentDependencies;

    VKCHECK(vkCreateRenderPass(vkc->device, &render_pass_info, NULL, &vkr->render_pass));

    vkc->release_queue.push_function([=]() {
        vkDestroyRenderPass(vkc->device, vkr->render_pass, NULL);
    });

    //////////////////////////////////////////////
    // Framebuffer initialization
    // create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering

    // grab how many images we have in the swapchain

    createFramebuffers(vkr->device, &vkr->framebuffers, vkr->swapchain, vkr->render_pass, vkr->depth_image_view);
#endif





    //////////////////////////////////////////////////////
    // CommandPools & CommandBuffers creation
    // Graphics
    VkCommandPoolCreateInfo ci_graphics_cmd_pool = vkinit::CommandPoolCreateInfo(vkc->graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VKCHECK(vkCreateCommandPool(vkr->device, &ci_graphics_cmd_pool, NULL, &vkc->command_pool_graphics));

    // Compute
    VkCommandPoolCreateInfo ci_compute_cmd_pool = vkinit::CommandPoolCreateInfo(vkc->compute_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VKCHECK(vkCreateCommandPool(vkr->device, &ci_compute_cmd_pool, NULL, &vkc->command_pool_compute));

    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmd_buffer_alloc_info_graphics = vkinit::CommandBufferAllocateInfo(vkc->command_pool_graphics);
        VKCHECK(vkAllocateCommandBuffers(vkr->device, &cmd_buffer_alloc_info_graphics, &vkr->frames[i].cmd_buffer_gfx));

        VkCommandBufferAllocateInfo cmd_buffer_alloc_info_compute = vkinit::CommandBufferAllocateInfo(vkc->command_pool_compute);
        VKCHECK(vkAllocateCommandBuffers(vkr->device, &cmd_buffer_alloc_info_compute, &vkr->frames[i].cmd_buffer_cmp));
    }

    vkc->release_queue.push_function([=]() {
        vkDestroyCommandPool(vkr->device, vkc->command_pool_graphics, NULL);
        vkDestroyCommandPool(vkr->device, vkc->command_pool_compute, NULL);
    });





    ////////////////////////////////////////////////////////////////
    // Sync structures
    // Fence creation
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext             = NULL;
    fenceCreateInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT; // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)

    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
        VKCHECK(vkCreateFence(vkr->device, &fenceCreateInfo, NULL, &vkr->frames[i].render_fence));
        VKCHECK(vkCreateFence(vkr->device, &fenceCreateInfo, NULL, &vkr->frames[i].compute_fence));

        /////
        // Semaphores creation
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext                 = NULL;
        semaphoreCreateInfo.flags                 = 0;

        VKCHECK(vkCreateSemaphore(vkr->device, &semaphoreCreateInfo, NULL, &vkr->frames[i].present_semaphore));
        VKCHECK(vkCreateSemaphore(vkr->device, &semaphoreCreateInfo, NULL, &vkr->frames[i].render_semaphore));

        vkr->release_queue.push_function([=]() {
            vkDestroyFence(vkr->device, vkr->frames[i].render_fence, NULL);
            vkDestroyFence(vkr->device, vkr->frames[i].compute_fence, NULL);
            vkDestroySemaphore(vkr->device, vkr->frames[i].present_semaphore, NULL);
            vkDestroySemaphore(vkr->device, vkr->frames[i].render_semaphore, NULL);
        });
    }





    /////////////////////////////////////////////////////
    // Descriptors
    std::vector<VkDescriptorPoolSize> sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4 },
    };

    ////////////////////////////
    /// Set Pool
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = 0;
    pool_info.maxSets                    = 4; 
    pool_info.poolSizeCount              = (uint32_t)sizes.size();
    pool_info.pPoolSizes                 = sizes.data();
    vkCreateDescriptorPool(vkr->device, &pool_info, NULL, &vkr->descriptor_pool);



    /////////////////////////
    /// Camera DescriptorSet
    VkDescriptorSetLayoutBinding desc_set_layout_binding_UBO = {};
    desc_set_layout_binding_UBO.binding                      = 0;
    desc_set_layout_binding_UBO.descriptorCount              = 1;
    desc_set_layout_binding_UBO.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_set_layout_binding_UBO.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT;

    /// layout > Camera UBO
    std::vector<VkDescriptorSetLayoutBinding> camera_set_layout_bindings;
    camera_set_layout_bindings.push_back(desc_set_layout_binding_UBO);
    CreateDescriptorSetLayout(vkr->device, NULL, &vkr->set_layout_camera, camera_set_layout_bindings.data(), (uint32_t)camera_set_layout_bindings.size());

    //
    //
    //
    //
    //

    ///////////////////////////////////////////
    // Pipelines setup
    // ---------------
    // All the needed pipelines are created right here
    // 1. instanced sprite pipeline
    // 2. non-instanced sprite pipeline
    const VkPipelineColorBlendAttachmentState color_blend_attachment_state = vkinit::color_blend_attachment_state();
    VkPipelineColorBlendStateCreateInfo       colorBlending                = vkinit::color_blend_state_create_info(&color_blend_attachment_state);





    /////////////////////
    // Depth attachment
    VkPipelineDepthStencilStateCreateInfo create_info_depth_stencil_state;
    create_info_depth_stencil_state = vkinit::depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = NULL;
    viewportState.viewportCount                     = 1;
    viewportState.scissorCount                      = 1;

    // We are using dynamic viewport state so that we don't have to rebuild
    // all our pipelines when the window is resized
    VkPipelineDynamicStateCreateInfo ci_dynamic_state = {};
    const VkDynamicState             dynamic_state[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    ci_dynamic_state.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ci_dynamic_state.dynamicStateCount                = 2;
    ci_dynamic_state.pDynamicStates                   = dynamic_state;





    ////////////////////////////
    // ShaderModules
    VkShaderModule vertexShader;
    if (!VkayCreateShaderModule("./shaders/quad.vert.spv", &vertexShader, vkr->device)) {
        // todo(ad): error
        SDL_Log("Failed to build vertex shader module. Did you compile the shaders?\n");
    } else {
        SDL_Log("vertex ShadeModule build successful\n");
    }

    VkShaderModule fragmentShader;
    if (!VkayCreateShaderModule("./shaders/textureArray.frag.spv", &fragmentShader, vkr->device)) {
        // todo(ad): error
        SDL_Log("Failed to built fragment shader module. Did you compile the shaders?\n");
    } else {
        SDL_Log("fragment ShadeModule build successful\n");
    }

    std::vector<VkPipelineShaderStageCreateInfo> create_info_shader_stages;
    create_info_shader_stages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    create_info_shader_stages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));

    vkr->release_queue.push_function([=]() {
        vkDestroyShaderModule(vkr->device, vertexShader, NULL);
        vkDestroyShaderModule(vkr->device, fragmentShader, NULL);
    });





    /////////////////////////////////
    // Vertex input description
    VertexInputDescription vertexDescription;

    // we will have just 1 vertex buffer binding, with a per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding                         = 0;
    mainBinding.stride                          = sizeof(Vertex);
    mainBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexDescription.bindings.push_back(mainBinding);

    // Position will be stored at Location 0
    {
        VkVertexInputAttributeDescription position_attribute = {};
        position_attribute.binding                           = 0;
        position_attribute.location                          = 0;
        position_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        position_attribute.offset                            = offsetof(Vertex, position);

        // Normal will be stored at Location 1
        VkVertexInputAttributeDescription normal_attribute = {};
        normal_attribute.binding                           = 0;
        normal_attribute.location                          = 1;
        normal_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        normal_attribute.offset                            = offsetof(Vertex, normal);

        VkVertexInputAttributeDescription tex_uv_attribute = {};
        tex_uv_attribute.binding                           = 0;
        tex_uv_attribute.location                          = 2;
        tex_uv_attribute.format                            = VK_FORMAT_R32G32_SFLOAT;
        tex_uv_attribute.offset                            = offsetof(Vertex, tex_uv);

        // Color will be stored at Location 3
        VkVertexInputAttributeDescription color_attribute = {};
        color_attribute.binding                           = 0;
        color_attribute.location                          = 3;
        color_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        color_attribute.offset                            = offsetof(Vertex, color);


        vertexDescription.attributes.push_back(position_attribute);
        vertexDescription.attributes.push_back(normal_attribute);
        vertexDescription.attributes.push_back(color_attribute);
        vertexDescription.attributes.push_back(tex_uv_attribute);
    }

    {
        VkVertexInputAttributeDescription position_attribute = {};
        position_attribute.binding                           = 1;
        position_attribute.location                          = 4;
        position_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        position_attribute.offset                            = offsetof(InstanceData, pos);

        VkVertexInputAttributeDescription rotation_attribute = {};
        rotation_attribute.binding                           = 1;
        rotation_attribute.location                          = 5;
        rotation_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        rotation_attribute.offset                            = offsetof(InstanceData, rot);

        VkVertexInputAttributeDescription scale_attribute = {};
        scale_attribute.binding                           = 1;
        scale_attribute.location                          = 6;
        scale_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        scale_attribute.offset                            = offsetof(InstanceData, scale);

        VkVertexInputAttributeDescription tex_idx_attribute = {};
        tex_idx_attribute.binding                           = 1;
        tex_idx_attribute.location                          = 7;
        tex_idx_attribute.format                            = VK_FORMAT_R32_SINT;
        tex_idx_attribute.offset                            = offsetof(InstanceData, texure_idx);

        vertexDescription.attributes.push_back(position_attribute);
        vertexDescription.attributes.push_back(rotation_attribute);
        vertexDescription.attributes.push_back(scale_attribute);
        vertexDescription.attributes.push_back(tex_idx_attribute);
    }

    // InstanceData
    VkVertexInputBindingDescription instance_data_binding = {};
    instance_data_binding.binding                         = 1;
    instance_data_binding.stride                          = sizeof(InstanceData);
    instance_data_binding.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;
    vertexDescription.bindings.push_back(instance_data_binding);

    ////////////////////////////
    /// Vertex Input Description
    VkPipelineVertexInputStateCreateInfo create_info_vertex_input_state;
    create_info_vertex_input_state                                 = vkinit::vertex_input_state_create_info();
    create_info_vertex_input_state.pVertexAttributeDescriptions    = vertexDescription.attributes.data();
    create_info_vertex_input_state.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();
    create_info_vertex_input_state.pVertexBindingDescriptions      = vertexDescription.bindings.data();
    create_info_vertex_input_state.vertexBindingDescriptionCount   = (uint32_t)vertexDescription.bindings.size();





    ////////////////////////////
    /// Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo ci_input_assembly_state = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ////////////////////////////
    /// Raster State
    VkPipelineRasterizationStateCreateInfo ci_rasterization_state = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL); // configure the rasterizer to draw filled triangles
    ////////////////////////////
    /// Multisample State
    VkPipelineMultisampleStateCreateInfo ci_multisample_state = vkinit::multisampling_state_create_info(); // we don't use multisampling, so just run the default one





    VkDescriptorSetLayoutBinding desc_set_layout_binding_sampler = {};
    desc_set_layout_binding_sampler.binding                      = 0;
    desc_set_layout_binding_sampler.descriptorCount              = 1;
    desc_set_layout_binding_sampler.descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLER;
    desc_set_layout_binding_sampler.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding desc_set_layout_binding_sampled_image = {};
    desc_set_layout_binding_sampled_image.binding                      = 1;
    desc_set_layout_binding_sampled_image.descriptorCount              = MAX_TEXTURE_COUNT; // texture count
    desc_set_layout_binding_sampled_image.descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    desc_set_layout_binding_sampled_image.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    /// layout > array of textures & sampler
    std::vector<VkDescriptorSetLayoutBinding> array_of_textures_set_layout_bindings = {
        desc_set_layout_binding_sampler,
        desc_set_layout_binding_sampled_image
    };

    CreateDescriptorSetLayout(vkr->device, NULL, &vkr->set_layout_array_of_textures, array_of_textures_set_layout_bindings.data(), (uint32_t)array_of_textures_set_layout_bindings.size());

    // Sampler
    VkSamplerCreateInfo ci_sampler = {};
    ci_sampler.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci_sampler.minFilter           = VK_FILTER_NEAREST;
    ci_sampler.magFilter           = VK_FILTER_NEAREST;
    ci_sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci_sampler.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci_sampler.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci_sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VKCHECK(vkCreateSampler(vkr->device, &ci_sampler, NULL, &vkr->sampler));


    vkr->descriptor_image_infos.resize(MAX_TEXTURE_COUNT);
    memset(vkr->descriptor_image_infos.data(), VK_NULL_HANDLE, vkr->descriptor_image_infos.size() * sizeof(VkDescriptorImageInfo));



    ///////
    // PipelineLayout
    // this is where we provide the vertex shader with our matrices
    // VkPushConstantRange push_constant;
    // push_constant.offset     = 0;
    // push_constant.size       = sizeof(MeshPushConstants);
    // push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info(); // build the pipeline layout that controls the inputs/outputs of the shader
    // pipeline_layout_info.pushConstantRangeCount     = 1;
    // pipeline_layout_info.pPushConstantRanges        = &push_constant;

    std::vector<VkDescriptorSetLayout> set_layouts = {
        vkr->set_layout_camera,
        vkr->set_layout_array_of_textures,
    };

    pipeline_layout_info.setLayoutCount = (uint32_t)set_layouts.size();
    pipeline_layout_info.pSetLayouts    = set_layouts.data();
    VKCHECK(vkCreatePipelineLayout(vkr->device, &pipeline_layout_info, NULL, &vkr->instanced_pipeline_layout));

    VkPipelineRenderingCreateInfo ci_pipeline_rendering = {};
    ci_pipeline_rendering.sType                         = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    ci_pipeline_rendering.colorAttachmentCount          = 1;
    ci_pipeline_rendering.pColorAttachmentFormats       = &vkr->swapchain.image_format;
    ci_pipeline_rendering.depthAttachmentFormat         = DEPTH_FORMAT;
    ci_pipeline_rendering.stencilAttachmentFormat       = DEPTH_FORMAT;

    // build the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = &ci_pipeline_rendering;
    pipelineInfo.stageCount                   = (uint32_t)create_info_shader_stages.size();
    pipelineInfo.pStages                      = create_info_shader_stages.data();
    pipelineInfo.pVertexInputState            = &create_info_vertex_input_state;
    pipelineInfo.pInputAssemblyState          = &ci_input_assembly_state;
    pipelineInfo.pTessellationState           = NULL;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &ci_rasterization_state;
    pipelineInfo.pMultisampleState            = &ci_multisample_state;
    pipelineInfo.pDepthStencilState           = &create_info_depth_stencil_state;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = NULL;
    pipelineInfo.layout                       = vkr->instanced_pipeline_layout;

#ifndef NOPASS
    pipelineInfo.renderPass = vkr->render_pass;
#endif

    pipelineInfo.subpass            = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = 0;
    pipelineInfo.pDynamicState      = &ci_dynamic_state;

    if (vkCreateGraphicsPipelines(vkr->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkr->instanced_pipeline) != VK_SUCCESS) {
        SDL_Log("failed to create pipline\n");
        vkDestroyShaderModule(vkr->device, vertexShader, NULL);
        vkDestroyShaderModule(vkr->device, fragmentShader, NULL);
    }

    vkr->compute_pipeline = VkayCreateComputePipeline(vkr);
}





//
//
//
//                      Rendering
//
//
//
////////////////////////////////////////////////////////////////////////

bool VkayRendererBeginCommandBuffer(VkayRenderer *vkr)
{
    // wait until the GPU has finished rendering the last frame. Timeout of 1 second

    // todo(ad): @urgent why are these pointers? ...we're derefencing these multiple times per frame which is crazy
    VkFence         *g_inflight_render_fence;
    VkSemaphore     *g_inflight_present_semaphore;
    VkSemaphore     *g_inflight_render_semaphore;
    VkCommandBuffer *g_inflight_main_command_buffer;
    // VkDescriptorSet *g_inflight_global_descriptor_set;

    g_inflight_render_fence        = &vkr->frames[vkr->frame_idx_inflight].render_fence;
    g_inflight_present_semaphore   = &vkr->frames[vkr->frame_idx_inflight].present_semaphore;
    g_inflight_render_semaphore    = &vkr->frames[vkr->frame_idx_inflight].render_semaphore;
    g_inflight_main_command_buffer = &vkr->frames[vkr->frame_idx_inflight].cmd_buffer_gfx;
    // g_inflight_global_descriptor_set = &vkr->frames[vkr->frame_idx_inflight % FRAME_BUFFER_COUNT].set_global;


    VKCHECK(vkWaitForFences(vkr->device, 1, g_inflight_render_fence, true, SECONDS(1)));

    VkResult result = vkAcquireNextImageKHR(vkr->device, vkr->swapchain.handle, SECONDS(1), *g_inflight_present_semaphore, NULL, &vkr->frames[vkr->frame_idx_inflight].idx_swapchain_image);
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        vkDeviceWaitIdle(vkr->device);
        vkr->swapchain.create(VkayGetContext()->device, FRAME_BUFFER_COUNT, vkr->allocator, &vkr->depth_image, &vkr->depth_image_view);

#ifndef NOPASS
        createFramebuffers(vkr->device, &vkr->framebuffers, vkr->swapchain, vkr->render_pass, vkr->depth_image_view);
#endif

        return false;
    } else {
        VKCHECK(result);
    }


    VKCHECK(vkResetFences(vkr->device, 1, g_inflight_render_fence));
    VKCHECK(vkResetCommandBuffer(*g_inflight_main_command_buffer, 0)); // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.


    ///////////////////////////////////////
    // Begin the command buffer recording
    VkCommandBufferBeginInfo cmd_begin_info = {};
    cmd_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext                    = NULL;
    cmd_begin_info.pInheritanceInfo         = NULL;
    cmd_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We will use this command buffer exactly once (per frame)
    VKCHECK(vkBeginCommandBuffer(*g_inflight_main_command_buffer, &cmd_begin_info));


    // vkr->clear_value.color = { { .1f, .1f, .1f } };

    // float flash       = abs(sin(vkr->frame_idx_inflight / 120.f));
    // clear_value.color = { { 0.0f, 0.0f, flash, 1.0f } };

    VkClearValue depth_clear          = {};
    depth_clear.depthStencil.depth    = 1.f;
    VkClearValue union_clear_values[] = { vkr->clear_value, depth_clear };


    VkExtent2D window_extent = VkayGetContext()->window_extent;





#ifndef NOPASS
    //////////////////////
    // BEGIN renderpass
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext                 = NULL;
    rpInfo.renderPass            = vkr->render_pass;
    rpInfo.renderArea.offset.x   = 0;
    rpInfo.renderArea.offset.y   = 0;
    rpInfo.renderArea.extent     = window_extent;
    rpInfo.framebuffer           = vkr->framebuffers[vkr->frames[vkr->frame_idx_inflight].idx_swapchain_image];
    rpInfo.clearValueCount       = 2;
    rpInfo.pClearValues          = union_clear_values;
    vkCmdBeginRenderPass(*g_inflight_main_command_buffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
#endif





#ifdef NOPASS
    {
        VkImageMemoryBarrier swapchain_image_layout_transition = {};
        swapchain_image_layout_transition.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapchain_image_layout_transition.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        swapchain_image_layout_transition.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchain_image_layout_transition.newLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        swapchain_image_layout_transition.image                = vkr->swapchain.images[vkr->frame_idx_inflight];
        swapchain_image_layout_transition.subresourceRange     = vkr->swapchain.subresource_range;

        VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        vkCmdPipelineBarrier(vkr->frames[vkr->frame_idx_inflight % FRAME_BUFFER_COUNT].cmd_buffer_gfx,
            src_stage_flags, dst_stage_flags, 0, 0, NULL, 0, NULL, 1,
            &swapchain_image_layout_transition);
    }
    {
        VkImageMemoryBarrier depthStencil_image_layout_transition = {};
        depthStencil_image_layout_transition.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthStencil_image_layout_transition.dstAccessMask        = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthStencil_image_layout_transition.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        depthStencil_image_layout_transition.newLayout            = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthStencil_image_layout_transition.image                = vkr->depth_image.image;
        depthStencil_image_layout_transition.subresourceRange     = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

        VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        vkCmdPipelineBarrier(vkr->frames[vkr->frame_idx_inflight % FRAME_BUFFER_COUNT].cmd_buffer_gfx,
            src_stage_flags, dst_stage_flags, 0, 0, NULL, 0, NULL, 1,
            &depthStencil_image_layout_transition);
    }

    VkRenderingAttachmentInfoKHR color_attachment = {};
    color_attachment.sType                        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment.imageView                    = vkr->swapchain.image_views[vkr->frame_idx_inflight];
    color_attachment.imageLayout                  = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    color_attachment.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue                   = vkr->clear_value;

    VkRenderingAttachmentInfoKHR depthStencil_attachment = {};
    depthStencil_attachment.sType                        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depthStencil_attachment.imageView                    = vkr->depth_image_view;
    depthStencil_attachment.imageLayout                  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    depthStencil_attachment.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencil_attachment.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
    depthStencil_attachment.clearValue.depthStencil      = { 1.0f, 0 };

    VkRenderingInfoKHR rendering_info   = {};
    rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments    = &color_attachment;
    rendering_info.pDepthAttachment     = &depthStencil_attachment;
    rendering_info.pStencilAttachment   = &depthStencil_attachment;
    rendering_info.renderArea.offset    = { 0, 0 };
    rendering_info.renderArea.extent    = window_extent;
    vkCmdBeginRendering(*g_inflight_main_command_buffer, &rendering_info);
#endif





    VkViewport viewport = vkinit::viewport_state((float)window_extent.width, (float)window_extent.height);

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = window_extent;

    vkCmdSetViewport(*g_inflight_main_command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(*g_inflight_main_command_buffer, 0, 1, &scissor);

    return true;
}





void VkayRendererEndRenderPass(VkayRenderer *vkr)
{
    ////////////////////////////////////////////////
    // END renderpass & command buffer recording
#ifndef NOPASS
    vkCmdEndRenderPass(VkayRendererGetCurrentFrameData(vkr)->cmd_buffer_gfx);
#endif

#ifdef NOPASS
    vkCmdEndRendering(VkayRendererGetCurrentFrameData(vkr)->cmd_buffer_gfx);

    VkImageMemoryBarrier swapchain_image_layout_transition = {};
    swapchain_image_layout_transition.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapchain_image_layout_transition.srcAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    swapchain_image_layout_transition.oldLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swapchain_image_layout_transition.newLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapchain_image_layout_transition.image                = vkr->swapchain.images[vkr->frame_idx_inflight];
    swapchain_image_layout_transition.subresourceRange     = vkr->swapchain.subresource_range;

    VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    vkCmdPipelineBarrier(VkayRendererGetCurrentFrameData(vkr)->cmd_buffer_gfx,
        src_stage_flags, dst_stage_flags, 0, 0, NULL, 0, NULL, 1,
        &swapchain_image_layout_transition);
#endif





    VKCHECK(vkEndCommandBuffer(VkayRendererGetCurrentFrameData(vkr)->cmd_buffer_gfx));

    VkSubmitInfo submit_info = {};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext        = NULL;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask  = &waitStage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = &VkayRendererGetCurrentFrameData(vkr)->present_semaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &VkayRendererGetCurrentFrameData(vkr)->render_semaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &VkayRendererGetCurrentFrameData(vkr)->cmd_buffer_gfx;
    // submit command buffer to the queue and execute it.
    //  g_inflight_render_fence will now block until the graphic commands finish execution
    VKCHECK(vkQueueSubmit(vkr->graphics_queue, 1, &submit_info, VkayRendererGetCurrentFrameData(vkr)->render_fence));
}





void VkayRendererPresent(VkayRenderer *vkr)
{
    // this will put the image we just rendered into the visible window.
    // we want to g_inflight_render_semaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = NULL;

    presentInfo.pSwapchains    = &vkr->swapchain.handle;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores    = &VkayRendererGetCurrentFrameData(vkr)->render_semaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &vkr->frames[vkr->frame_idx_inflight].idx_swapchain_image;

    VkResult result;
    result = vkQueuePresentKHR(vkr->graphics_queue, &presentInfo);
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        vkr->swapchain.create(vkr->device, FRAME_BUFFER_COUNT, vkr->allocator, &vkr->depth_image, &vkr->depth_image_view);

#ifndef NOPASS
        createFramebuffers(vkr->device, &vkr->framebuffers, vkr->swapchain, vkr->render_pass, vkr->depth_image_view);
#endif

        return;
    } else {
        VKCHECK(result);
    }

    vkr->frame_idx_inflight++;
    vkr->frame_idx_inflight = vkr->frame_idx_inflight % FRAME_BUFFER_COUNT;
}





/////////////////////////////////////////////
//      Compute Pipeline

VkPipeline VkayCreateComputePipeline(VkayRenderer *vkr)
{
    VkShaderModule compute_shader_module;
    if (!VkayCreateShaderModule("./shaders/basicComputeShader.comp.spv", &compute_shader_module, vkr->device)) {
        // todo(ad): error
        SDL_Log("Failed to build compute shader module. Did you compile the shaders?\n");

    } else {
        SDL_Log("Compute ShadeModule build successfull\n");
    }

    VkPipelineShaderStageCreateInfo ci_shader_stage = {};
    ci_shader_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ci_shader_stage.flags                           = 0;
    ci_shader_stage.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
    ci_shader_stage.module                          = compute_shader_module;
    ci_shader_stage.pName                           = "main"; // compute shader entry point


    VkDescriptorSetLayoutBinding instanced_data_IN_set_binding = {};
    instanced_data_IN_set_binding.binding                      = 0;
    instanced_data_IN_set_binding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    instanced_data_IN_set_binding.descriptorCount              = 1;
    instanced_data_IN_set_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

    // VkDescriptorSetLayoutBinding instanced_data_OUT_set_binding = {};
    // instanced_data_OUT_set_binding.binding                      = 1;
    // instanced_data_OUT_set_binding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    // instanced_data_OUT_set_binding.descriptorCount              = 1;
    // instanced_data_OUT_set_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding instanced_data_uniform_set_binding = {};
    instanced_data_uniform_set_binding.binding                      = 1;
    instanced_data_uniform_set_binding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    instanced_data_uniform_set_binding.descriptorCount              = 1;
    instanced_data_uniform_set_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> instanced_data_set_layout_bindings = {};
    instanced_data_set_layout_bindings.push_back(instanced_data_IN_set_binding);
    // instanced_data_set_layout_bindings.push_back(instanced_data_OUT_set_binding);
    // instanced_data_set_layout_bindings.push_back(instanced_data_uniform_set_binding);

    VkDescriptorSetLayoutCreateInfo ci_instanced_data_set_layout = {};
    ci_instanced_data_set_layout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci_instanced_data_set_layout.flags                           = 0;
    ci_instanced_data_set_layout.bindingCount                    = (uint32_t)instanced_data_set_layout_bindings.size();
    ci_instanced_data_set_layout.pBindings                       = instanced_data_set_layout_bindings.data();
    VKCHECK(vkCreateDescriptorSetLayout(vkr->device, &ci_instanced_data_set_layout, NULL, &vkr->set_layout_instanced_data));

    std::vector<VkDescriptorSetLayout> compute_set_layouts;
    compute_set_layouts.push_back(vkr->set_layout_instanced_data);

    ///////////////////////////////////////////////
    // dummy descriptor set to be allocated
    // we should probably make a map of sets
    vkr->compute_descriptor_sets.push_back(VkDescriptorSet {});


    //////////////////////////
    // Push Constants
    VkPushConstantRange push_constant_range = {};
    push_constant_range.stageFlags          = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant_range.size                = sizeof(PushConstantsComputeShader);
    push_constant_range.offset              = 0;

    std::vector<VkPushConstantRange> push_constant_ranges {};
    push_constant_ranges.push_back(push_constant_range);


    ///////////////////////////
    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> pool_sizes = {};
    for (size_t i = 0; i < instanced_data_set_layout_bindings.size(); i++) {
        VkDescriptorPoolSize pool_size = {
            instanced_data_set_layout_bindings[i].descriptorType,
            instanced_data_set_layout_bindings[i].descriptorCount
        };
        pool_sizes.push_back(pool_size);
    }

    VkDescriptorPoolCreateInfo ci_descriptor_pool = {};
    ci_descriptor_pool.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci_descriptor_pool.flags                      = 0;
    ci_descriptor_pool.maxSets                    = (uint32_t)vkr->compute_descriptor_sets.size();
    ci_descriptor_pool.poolSizeCount              = (uint32_t)pool_sizes.size();
    ci_descriptor_pool.pPoolSizes                 = pool_sizes.data();
    VKCHECK(vkCreateDescriptorPool(vkr->device, &ci_descriptor_pool, NULL, &vkr->descriptor_pool_compute));


    ////////////////////////
    // Set allocation
    VkDescriptorSetAllocateInfo ci_set_alloc_info = {};
    ci_set_alloc_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ci_set_alloc_info.descriptorPool              = vkr->descriptor_pool_compute;
    ci_set_alloc_info.descriptorSetCount          = (uint32_t)vkr->compute_descriptor_sets.size();
    ci_set_alloc_info.pSetLayouts                 = &vkr->set_layout_instanced_data;
    VKCHECK(vkAllocateDescriptorSets(vkr->device, &ci_set_alloc_info, vkr->compute_descriptor_sets.data()));


    ////////////////////////////////////////////
    // Pipeline Layout & Pipeline creation
    VkPipelineLayoutCreateInfo ci_pipeline_layout = {};
    ci_pipeline_layout.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ci_pipeline_layout.flags                      = 0;
    ci_pipeline_layout.setLayoutCount             = (uint32_t)compute_set_layouts.size();
    ci_pipeline_layout.pSetLayouts                = compute_set_layouts.data();
    ci_pipeline_layout.pushConstantRangeCount     = (uint32_t)push_constant_ranges.size();
    ci_pipeline_layout.pPushConstantRanges        = push_constant_ranges.data();
    VKCHECK(vkCreatePipelineLayout(vkr->device, &ci_pipeline_layout, NULL, &vkr->compute_pipeline_layout));

    VkComputePipelineCreateInfo ci_compute_pipeline = {};
    ci_compute_pipeline.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci_compute_pipeline.stage                       = ci_shader_stage;
    ci_compute_pipeline.layout                      = vkr->compute_pipeline_layout;

    VkPipeline new_compute_pipeline;
    if (vkCreateComputePipelines(vkr->device, VK_NULL_HANDLE, 1, &ci_compute_pipeline, NULL, &new_compute_pipeline) != VK_SUCCESS) {
        // todo(ad): error
        SDL_Log("failed to create pipline\n");
        vkDestroyShaderModule(vkr->device, compute_shader_module, NULL);
        return VK_NULL_HANDLE;
    } else {
        vkr->release_queue.push_function([=]() {
            vkDestroyShaderModule(vkr->device, compute_shader_module, NULL);
        });
        return new_compute_pipeline;
    }
}

/* TODO(ad):
 * VkDevice should be the first argument
 * also ditch buffer vector, that's overkill, just allocate a raw array
 */
bool VkayCreateShaderModule(const char *filepath, VkShaderModule *out_ShaderModule, VkDevice device)
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

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    *out_ShaderModule = shaderModule;

    return true;
}

void VkayRendererCleanup(VkayRenderer *vkr)
{
    // if (vkr->is_initialized)
    {
        vkWaitForFences(vkr->device, 1, &vkr->frames[0].render_fence, true, SECONDS(1));
        vkWaitForFences(vkr->device, 1, &vkr->frames[1].render_fence, true, SECONDS(1));

        vkDeviceWaitIdle(vkr->device);

        vkr->release_queue.flush();
        // vmafree
        vmaDestroyAllocator(vkr->allocator);

        vkDestroySwapchainKHR(vkr->device, vkr->swapchain.handle, NULL);
        vkDestroySampler(vkr->device, vkr->sampler, NULL);

        vkDestroyPipelineLayout(vkr->device, vkr->compute_pipeline_layout, NULL);
        vkDestroyPipelineLayout(vkr->device, vkr->default_pipeline_layout, NULL);
        vkDestroyPipeline(vkr->device, vkr->default_pipeline, NULL);
        vkDestroyPipelineLayout(vkr->device, vkr->instanced_pipeline_layout, NULL);
        vkDestroyPipeline(vkr->device, vkr->instanced_pipeline, NULL);
        vkDestroyPipeline(vkr->device, vkr->compute_pipeline, NULL);
        vkDestroyDescriptorSetLayout(vkr->device, vkr->set_layout_instanced_data, NULL);
        vkDestroyDescriptorSetLayout(vkr->device, vkr->set_layout_camera, NULL);
        vkDestroyDescriptorSetLayout(vkr->device, vkr->set_layout_array_of_textures, NULL);
        vkDestroyDescriptorPool(vkr->device, vkr->descriptor_pool_compute, NULL);
        vkDestroyDescriptorPool(vkr->device, vkr->descriptor_pool, NULL);
    }
}

/////////////////////////////////////
// vkaySwapchain implementation
void vkaySwapchain::create(VkDevice device, uint32_t min_image_count, VmaAllocator allocator, AllocatedImage *depth_image, VkImageView *depth_image_view)
{
    if (handle != VK_NULL_HANDLE) {
        for (size_t i = 0; i < image_views.size(); i++) {
            vkDeviceWaitIdle(device);
            vkDestroyImageView(device, image_views[i], NULL);
        }
    }

    vkayPhysicalDeviceSurfaceInfo_KHR physical_device_info;
    physical_device_info.query(VkayGetContext()->physical_device, VkayGetContext()->surface);
    VkayGetContext()->window_extent = physical_device_info.capabilities.currentExtent;

    image_format = physical_device_info.formats[0].format;
    color_space  = physical_device_info.formats[0].colorSpace;

    VkSwapchainCreateInfoKHR create_info_swapchain = {};
    create_info_swapchain.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info_swapchain.pNext                    = NULL;
    create_info_swapchain.flags                    = 0;
    create_info_swapchain.surface                  = VkayGetContext()->surface;
    create_info_swapchain.minImageCount            = min_image_count;
    create_info_swapchain.imageFormat              = image_format;
    create_info_swapchain.imageColorSpace          = color_space;
    create_info_swapchain.imageExtent              = physical_device_info.capabilities.currentExtent;
    create_info_swapchain.imageArrayLayers         = 1;
    create_info_swapchain.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info_swapchain.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    create_info_swapchain.queueFamilyIndexCount    = 0;
    create_info_swapchain.pQueueFamilyIndices      = NULL;
    create_info_swapchain.preTransform             = physical_device_info.capabilities.currentTransform;
    create_info_swapchain.compositeAlpha           = (VkCompositeAlphaFlagBitsKHR)physical_device_info.capabilities.supportedCompositeAlpha; // todo(ad): this might only work if there's only one flag
    create_info_swapchain.presentMode              = VK_PRESENT_MODE_MAILBOX_KHR; // todo(ad): setting this arbitrarily, must check if supported and desired
    // create_info_swapchain.presentMode              = VK_PRESENT_MODE_FIFO_RELAXED_KHR; // todo(ad): setting this arbitrarily, must check if supported and desired
    // create_info_swapchain.presentMode  = VK_PRESENT_MODE_IMMEDIATE_KHR; // todo(ad): setting this arbitrarily, must check if supported and desired
    create_info_swapchain.clipped      = VK_TRUE;
    create_info_swapchain.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(device, &create_info_swapchain, NULL, &handle);

    //////////////////////////////////////
    /// Swapchain Images acquisition
    // this will request the amount of images we specified above 'minImageCount'
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(device, handle, &swapchain_image_count, NULL);
    images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, handle, &swapchain_image_count, images.data());

    image_views.resize(swapchain_image_count);

    subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel   = 0;
    subresource_range.levelCount     = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount     = 1;

    for (size_t i = 0; i < image_views.size(); i++) {

        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image                 = images[i];
        image_view_create_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                = image_format;
        image_view_create_info.components.r          = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g          = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b          = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a          = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange      = subresource_range;

        vkCreateImageView(device, &image_view_create_info, NULL, &image_views[i]);
    }





    VkExtent3D depthImageExtent = {
        VkayGetContext()->window_extent.width,
        VkayGetContext()->window_extent.height,
        1
    };

    // the depth image will be an image with the format we selected and Depth Attachment usage flag
    // VkImageCreateInfo create_info_depth_image = vkinit::image_create_info(DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
    VkImageCreateInfo create_info_depth_image = {};
    create_info_depth_image.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info_depth_image.pNext             = NULL;
    create_info_depth_image.imageType         = VK_IMAGE_TYPE_2D;
    create_info_depth_image.format            = DEPTH_FORMAT;
    create_info_depth_image.extent            = depthImageExtent;
    create_info_depth_image.mipLevels         = 1;
    create_info_depth_image.arrayLayers       = 1;
    create_info_depth_image.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info_depth_image.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info_depth_image.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    create_info_depth_image.flags             = 0;

    VmaAllocationCreateInfo create_info_depth_image_allocation = {}; // for the depth image, we want to allocate it from GPU local memory
    create_info_depth_image_allocation.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    create_info_depth_image_allocation.requiredFlags           = (VkMemoryPropertyFlags)VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vmaCreateImage(allocator, &create_info_depth_image, &create_info_depth_image_allocation, &depth_image->image, &depth_image->allocation, NULL);

    // build an image-view for the depth image to use for rendering
    // VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(DEPTH_FORMAT, vkr->depth_image->image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VkImageSubresourceRange depth_image_view_subrange = {};
    depth_image_view_subrange.aspectMask              = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_subrange.baseMipLevel            = 0;
    depth_image_view_subrange.levelCount              = 1;
    depth_image_view_subrange.baseArrayLayer          = 0;
    depth_image_view_subrange.layerCount              = 1;

    VkImageViewCreateInfo dview_info = {};
    dview_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    dview_info.pNext                 = NULL;
    dview_info.image                 = depth_image->image;
    dview_info.format                = DEPTH_FORMAT;
    dview_info.subresourceRange      = depth_image_view_subrange;
    dview_info.flags                 = 0;
    dview_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;

    VKCHECK(vkCreateImageView(device, &dview_info, NULL, depth_image_view));
}
