#include <Vkay.h>
#include <VkayTexture.h>
#include <VkayInitializers.h>
#include <VkayCamera.h>
#include <VkayMesh.h>
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

void UpdateAndRender();

bool  _key_r  = false;
bool  _key_up = false, _key_down = false, _key_left = false, _key_right = false;
bool  _key_W = false, _key_A = false, _key_S = false, _key_D = false, _key_Q = false, _key_E = false;
float camera_x = 0, camera_y = 0, camera_z = 0;

float pos_x        = 0;
float pos_y        = 0;
float pos_z        = 0;
float camera_speed = 1.f;
float player_speed = 20.f;

const uint64_t MAX_DT_SAMPLES = 256;

double dt_samples[MAX_DT_SAMPLES] = {};
double dt_averaged                = 0;

VkayContext  vkc;
VkayRenderer vkr;
VkPipeline   graphics_pipeline;

cgltf_data *data;
VkayBuffer  ibo = {};
VkayBuffer  vbo = {};
Camera      camera;
Triangle    triangle           = Triangle();
VkayBuffer  triangle_ibo       = {};
VkayBuffer  triangle_vbo       = {};
Transform   triangle_transform = {};

int main(int argc, char *argv[])
{
    VkayContextInit(&vkc);
    VkayRendererInit(&vkc, &vkr);

    ///////////////
    // Pipeline
    VkPipelineColorBlendAttachmentState attachment_state_color_blend;
    attachment_state_color_blend = vkinit::color_blend_attachment_state(); // a single blend attachment with no blending and writing to RGBA

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};

    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext             = NULL;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.pAttachments      = &attachment_state_color_blend;
    colorBlending.attachmentCount   = 1;
    colorBlending.blendConstants[0] = 0.0;
    colorBlending.blendConstants[1] = 0.0;
    colorBlending.blendConstants[2] = 0.0;
    colorBlending.blendConstants[3] = 0.0;

    VkPipelineDepthStencilStateCreateInfo create_info_depth_stencil_state;
    create_info_depth_stencil_state = vkinit::depth_stencil_state_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    /////
    // Depth attachment
    // connect the pipeline builder vertex input info to the one we get from Vertex
    // build viewport and scissor from the swapchain extents
    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)vkr.window_extent.width;
    viewport.height   = (float)vkr.window_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = vkr.window_extent;

    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = NULL;
    viewportState.viewportCount                     = 1;
    viewportState.pViewports                        = &viewport;
    viewportState.scissorCount                      = 1;
    viewportState.pScissors                         = &scissor;



    ////////////////////////////
    // ShaderModules
    VkShaderModule vertexShader;
    if (!VkayCreateShaderModule("shaders/mesh.vert.spv", &vertexShader, vkr.device)) {
        // todo(ad): error
        SDL_Log("Failed to build vertex shader module. Did you compile the shaders?\n");
    } else {
        SDL_Log("vertex ShadeModule build successful\n");
    }

    VkShaderModule fragmentShader;
    if (!VkayCreateShaderModule("shaders/mesh.frag.spv", &fragmentShader, vkr.device)) {
        // todo(ad): error
        SDL_Log("Failed to built fragment shader module. Did you compile the shaders?\n");
    } else {
        SDL_Log("fragment ShadeModule build successful\n");
    }

    std::vector<VkPipelineShaderStageCreateInfo> create_info_shader_stages;
    create_info_shader_stages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    create_info_shader_stages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));

    vkr.release_queue.push_function([=]() {
        vkDestroyShaderModule(vkr.device, vertexShader, NULL);
        vkDestroyShaderModule(vkr.device, fragmentShader, NULL);
    });

    ////////////////////////////
    /// Vertex Input Description
    VertexInputDescription description;

    // we will have just 1 vertex buffer binding, with a per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding                         = 0;
    mainBinding.stride                          = sizeof(Vertex2);
    mainBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    description.bindings.push_back(mainBinding);

    // Position will be stored at Location 0
    {
        VkVertexInputAttributeDescription position_attribute = {};
        position_attribute.binding                           = 0;
        position_attribute.location                          = 0;
        position_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        position_attribute.offset                            = offsetof(Vertex2, position);

        //// Normal will be stored at Location 1
        // VkVertexInputAttributeDescription normal_attribute = {};
        // normal_attribute.binding = 0;
        // normal_attribute.location = 1;
        // normal_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        // normal_attribute.offset = offsetof(Vertex2, normal);

        // VkVertexInputAttributeDescription tex_uv_attribute = {};
        // tex_uv_attribute.binding = 0;
        // tex_uv_attribute.location = 2;
        // tex_uv_attribute.format = VK_FORMAT_R32G32_SFLOAT;
        // tex_uv_attribute.offset = offsetof(Vertex2, tex_uv);

        //// Color will be stored at Location 3
        // VkVertexInputAttributeDescription color_attribute = {};
        // color_attribute.binding = 0;
        // color_attribute.location = 3;
        // color_attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        // color_attribute.offset = offsetof(Vertex2, color);


        description.attributes.push_back(position_attribute);
        // description.attributes.push_back(normal_attribute);
        // description.attributes.push_back(color_attribute);
        // description.attributes.push_back(tex_uv_attribute);
    }

    VkPipelineVertexInputStateCreateInfo create_info_vertex_input_state;
    create_info_vertex_input_state                                 = vkinit::vertex_input_state_create_info();
    create_info_vertex_input_state.pVertexAttributeDescriptions    = description.attributes.data();
    create_info_vertex_input_state.vertexAttributeDescriptionCount = (uint32_t)description.attributes.size();
    create_info_vertex_input_state.pVertexBindingDescriptions      = description.bindings.data();
    create_info_vertex_input_state.vertexBindingDescriptionCount   = (uint32_t)description.bindings.size();

    ////////////////////////////
    /// Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo create_info_input_assembly_state;
    create_info_input_assembly_state = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    ////////////////////////////
    /// Raster State
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    rasterization_state = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_LINE); // configure the rasterizer to draw filled triangles

    ////////////////////////////
    /// Multisample State
    VkPipelineMultisampleStateCreateInfo create_info_multisample_state;
    create_info_multisample_state = vkinit::multisampling_state_create_info(); // we don't use multisampling, so just run the default one


    ///////
    // PipelineLayout
    // this is where we provide the vertex shader with our matrices
    VkPushConstantRange push_constant;
    push_constant.offset     = 0;
    push_constant.size       = sizeof(Transform);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info(); // build the pipeline layout that controls the inputs/outputs of the shader
    pipeline_layout_info.pushConstantRangeCount     = 1;
    pipeline_layout_info.pPushConstantRanges        = &push_constant;

    std::vector<VkDescriptorSetLayout> set_layouts = {
        vkr.set_layout_camera,
        // vkr.set_layout_array_of_textures,
    };

    pipeline_layout_info.setLayoutCount = (uint32_t)set_layouts.size();
    pipeline_layout_info.pSetLayouts    = set_layouts.data();
    VK_CHECK(vkCreatePipelineLayout(vkr.device, &pipeline_layout_info, NULL, &vkr.default_pipeline_layout));

    // build the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = NULL;
    pipelineInfo.stageCount                   = (uint32_t)create_info_shader_stages.size();
    pipelineInfo.pStages                      = create_info_shader_stages.data();
    pipelineInfo.pVertexInputState            = &create_info_vertex_input_state;
    pipelineInfo.pInputAssemblyState          = &create_info_input_assembly_state;
    pipelineInfo.pTessellationState           = NULL;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterization_state;
    pipelineInfo.pMultisampleState            = &create_info_multisample_state;
    pipelineInfo.pDepthStencilState           = &create_info_depth_stencil_state;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = NULL;
    pipelineInfo.layout                       = vkr.default_pipeline_layout;
    pipelineInfo.renderPass                   = vkr.render_pass;
    pipelineInfo.subpass                      = 0;
    pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex            = 0;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    if (vkCreateGraphicsPipelines(vkr.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphics_pipeline) != VK_SUCCESS) {
        SDL_Log("failed to create pipline\n");
        vkDestroyShaderModule(vkr.device, vertexShader, NULL);
        vkDestroyShaderModule(vkr.device, fragmentShader, NULL);
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }


    cgltf_options options = {};
    cgltf_parse_file(&options, "./assets/3D/khronos_logo/scene.gltf", &data);
    cgltf_result res = cgltf_load_buffers(&options, data, "./assets/3D/khronos_logo/scene.bin");

    for (size_t i = 0; i < data->meshes_count; i++) {
        for (size_t j = 0; j < data->meshes[i].primitives_count; j++) {
            cgltf_primitive_type type = data->meshes[i].primitives[j].type;
            SDL_Log("mesh[%d].primitive = %s", i, cgltf_primitive_type_str[type]);

            for (size_t k = 0; k < data->meshes[i].primitives[j].attributes_count; k++) {
                const char *attrib_type = data->meshes[i].primitives[j].attributes[k].name;
                SDL_Log("mesh[%d] attributes[%d] = %s", i, k, attrib_type);
            }
        }
    }


    // Mesh
    /////////////////
    // Index Buffer
    cgltf_mesh      mesh      = data->meshes[0];
    cgltf_primitive primitive = mesh.primitives[0];

    uint32_t indices_size = (uint32_t)primitive.indices->buffer_view->size;
    void    *indicies     = primitive.indices->buffer_view->buffer->data;
    VK_CHECK(VkayCreateBuffer(&ibo, vkr.allocator, indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY));

    VkayBuffer staging_buffer = {};
    VK_CHECK(VkayCreateBuffer(&staging_buffer, vkr.allocator, indices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY));
    VK_CHECK(VkayMapMemcpyMemory(indicies, indices_size, vkr.allocator, staging_buffer.allocation));

    VkayBeginCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VkBufferCopy region = {};
    region.size         = indices_size;
    vkCmdCopyBuffer(vkr.frames[0].cmd_buffer_gfx, staging_buffer.buffer, ibo.buffer, 1, &region);
    VkayEndCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VK_CHECK(VkayQueueSumbit(vkr.graphics_queue, &vkr.frames[0].cmd_buffer_gfx));
    vkDeviceWaitIdle(vkr.device);





    //////////////////
    // Vertex Buffer
    cgltf_accessor *vertex_accessor  = primitive.attributes[1].data;
    uint32_t        vertex_data_size = (uint32_t)vertex_accessor->buffer_view->size;
    void           *vertex_data      = vertex_accessor->buffer_view->buffer->data;

    VK_CHECK(VkayCreateBuffer(&vbo, vkr.allocator, vertex_data_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY));

    VkayBuffer vertex_staging_buffer = {};
    VK_CHECK(VkayCreateBuffer(&vertex_staging_buffer, vkr.allocator, vertex_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY));
    VK_CHECK(VkayMapMemcpyMemory(vertex_data, vertex_data_size, vkr.allocator, vertex_staging_buffer.allocation));

    VkayBeginCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VkBufferCopy vertex_buffer_region = {};
    vertex_buffer_region.size         = vertex_data_size;
    vkCmdCopyBuffer(vkr.frames[0].cmd_buffer_gfx, vertex_staging_buffer.buffer, vbo.buffer, 1, &vertex_buffer_region);
    VkayEndCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VK_CHECK(VkayQueueSumbit(vkr.graphics_queue, &vkr.frames[0].cmd_buffer_gfx));
    VK_CHECK(vkDeviceWaitIdle(vkr.device));





    VkayCameraCreate(&vkr, &camera);
    camera.m_position   = { 0, 0, 0 };
    camera.m_projection = Camera::PERSPECTIVE;

    VkayCreateBuffer(&triangle_ibo, vkr.allocator, sizeof(triangle.indicies), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    VkayMapMemcpyMemory(triangle.indicies, sizeof(triangle.indicies), vkr.allocator, triangle_ibo.allocation);
    VkayCreateBuffer(&triangle_vbo, vkr.allocator, triangle.mesh.vertices.size() * sizeof(Vertex2), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    VkayMapMemcpyMemory(triangle.mesh.vertices.data(), triangle.mesh.vertices.size() * sizeof(Vertex2), vkr.allocator, triangle_vbo.allocation);

    triangle_transform.position = { 0, 0, 0 };

    /////////////////////
    // Main loop
    UpdateAndRender();

    //////////////////////
    // Cleanup
    VK_CHECK(vkDeviceWaitIdle(vkr.device));

    VkayRendererCleanup(&vkr);
    VkayContextCleanup(&vkc);
    return 0;
}

void UpdateAndRender()
{
    SDL_Event e;
    bool      bQuit = false;

    while (!bQuit) {
        uint64_t start = SDL_GetPerformanceCounter();

        while (SDL_PollEvent(&e) != 0) {
            SDL_Keycode key = e.key.keysym.sym;
            if (e.type == SDL_QUIT) bQuit = true;

            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE) bQuit = true;

                if (key == SDLK_UP) _key_up = false;
                if (key == SDLK_DOWN) _key_down = false;
                if (key == SDLK_LEFT) _key_left = false;
                if (key == SDLK_RIGHT) _key_right = false;

                if (key == SDLK_w) _key_W = false;
                if (key == SDLK_a) _key_A = false;
                if (key == SDLK_s) _key_S = false;
                if (key == SDLK_d) _key_D = false;
                if (key == SDLK_q) _key_Q = false;
                if (key == SDLK_e) _key_E = false;
                if (key == SDLK_r) _key_r = false;
            }

            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP) _key_up = true;
                if (key == SDLK_DOWN) _key_down = true;
                if (key == SDLK_LEFT) _key_left = true;
                if (key == SDLK_RIGHT) _key_right = true;

                if (key == SDLK_w) _key_W = true;
                if (key == SDLK_s) _key_S = true;
                if (key == SDLK_a) _key_A = true;
                if (key == SDLK_d) _key_D = true;
                if (key == SDLK_q) _key_Q = true;
                if (key == SDLK_e) _key_E = true;
                if (key == SDLK_r) _key_r = true;
            }
        }

        if (_key_up) camera_z += camera_speed * (float)dt_averaged;
        if (_key_down) camera_z -= camera_speed * (float)dt_averaged;
        if (_key_left) camera_x += camera_speed * (float)dt_averaged;
        if (_key_right) camera_x -= camera_speed * (float)dt_averaged;
        if (_key_Q) camera_y -= player_speed * (float)dt_averaged;
        if (_key_E) camera_y += player_speed * (float)dt_averaged;

        if (_key_W) pos_y += player_speed * (float)dt_averaged;
        if (_key_S) pos_y -= player_speed * (float)dt_averaged;
        if (_key_A) pos_x -= player_speed * (float)dt_averaged;
        if (_key_D) pos_x += player_speed * (float)dt_averaged;


        camera.m_position = { camera_x, camera_y - 0.f, camera_z };
        ///////////////////////////////
        // Compute dispatch

        //////////////////////////////////

        VkayRendererBeginCommandBuffer(&vkr);
        VkayRendererBeginRenderPass(&vkr);

        VkCommandBuffer cmd_buffer_gfx = vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx;
        vkCmdBindPipeline(cmd_buffer_gfx, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        vkCmdPushConstants(cmd_buffer_gfx, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Transform), &triangle_transform);
        VkayCameraUpdate(&vkr, &camera, vkr.default_pipeline_layout);

        vkCmdBindIndexBuffer(cmd_buffer_gfx, ibo.buffer, 0, VK_INDEX_TYPE_UINT32);
        VkDeviceSize offsets = { data->meshes[0].primitives[0].attributes[1].data->buffer_view->offset };
        vkCmdBindVertexBuffers(cmd_buffer_gfx, 0, 1, &vbo.buffer, &offsets);
        vkCmdDrawIndexed(cmd_buffer_gfx, (uint32_t)data->meshes[0].primitives[0].indices->count, 1, 0, 0, 0);

        vkCmdBindIndexBuffer(cmd_buffer_gfx, triangle_ibo.buffer, 0, VK_INDEX_TYPE_UINT32);
        VkDeviceSize tri_offsets = { 0 };
        vkCmdBindVertexBuffers(cmd_buffer_gfx, 0, 1, &triangle_vbo.buffer, &tri_offsets);
        vkCmdDrawIndexed(cmd_buffer_gfx, (uint32_t)3, 1, 0, 0, 0);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererEndCommandBuffer(&vkr);
        VkayRendererPresent(&vkr);

        // DeltaTime
        pos_x = pos_y = pos_z = 0;
        camera_x = camera_y = camera_z = 0;

        uint64_t        end                = SDL_GetPerformanceCounter();
        static uint64_t idx                = 0;
        dt_samples[idx++ % MAX_DT_SAMPLES] = (end - start) / (double)SDL_GetPerformanceFrequency();
        double sum                         = 0;
        for (uint64_t i = 0; i < MAX_DT_SAMPLES; i++) {
            sum += dt_samples[i];
        }
        dt_averaged = sum / MAX_DT_SAMPLES;
    }
}
