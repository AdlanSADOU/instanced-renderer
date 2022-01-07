#include "vk_renderer.h"
#include "vk_types.h"

// mutually exclusive
#define EXAMPLE_DRAW_ONE_TRIANGLE_PER_BUFFER       0
#define EXAMPLE_CPU_BOUND_DRAW_TRIANGLE_CUBE_BATCH 0
#define EXAMPLE_DRAW_INDIRECT_CUBE_BATCH           0

extern VulkanRenderer vkr;

extern float pos_x, pos_y, pos_z;




struct Triangle
{
    Vertex       vertices[3]   = {};
    BufferObject buffer_object = {};
    glm::mat4    transform_m4  = {};
};

struct Quad
{
    struct Data
    {
        Vertex vertices[6] = {
            { { -1.f, -1.0f, 0.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } },
            { { -1.0f, 1.f, 0.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } },
            { { 1.0f, 1.0f, 0.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } },
            // { { -1.0f, -1.0f, 0.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } },
            // { { 1.0f, 1.0f, 0.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } },
            // { { 1.0f, -1.0f, 0.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } },
        };

        // uint16_t indices[6] = {
        //     0, 1, 2,
        //     // 2, 1, 3
        //     2, 3, 1
        // };
    } data;

    glm::mat4 transform_m4 = {};
};
BufferObject quads_bo = {};

std::vector<Quad> quads;

struct Instances
{
    std::vector<InstanceData> data {
        { { 0, 0, 0 }, { 0, 0, 0 } },
        { { 6, 6, 0 }, { 0, 0, 0 } },

    };

    BufferObject bo;
};

Instances instances;

Triangle triangle = {};
Quad     quad;


#define SIDE_LENGTH          8
#define TEST_TRIANGLES_COUNT (SIDE_LENGTH * SIDE_LENGTH * SIDE_LENGTH)

#if EXAMPLE_DRAW_ONE_TRIANGLE_PER_BUFFER
Triangle test_triangles[TEST_TRIANGLES_COUNT];
#endif




#if EXAMPLE_CPU_BOUND_DRAW_TRIANGLE_CUBE_BATCH
struct JustATriange
{
    Vertex vertices[3] = {};
} test_triangles[TEST_TRIANGLES_COUNT]          = {};
glm::mat4    transform_m4[TEST_TRIANGLES_COUNT] = {};
BufferObject triangles_vbo                      = {};
#endif


#if EXAMPLE_DRAW_INDIRECT_CUBE_BATCH
struct JustATriange
{
    Vertex vertices[3] = {};
} test_triangles[TEST_TRIANGLES_COUNT]        = {};
ModelData    model_data[TEST_TRIANGLES_COUNT] = {};
BufferObject triangles_vbo                    = {};
BufferObject triangles_ibo                    = {};
BufferObject indirect_commands_buffer         = {};

#endif // EXAMPLE_DRAW_INDIRECT_CUBE_BATCH
void *instances_data_ptr;
void  InitExamples()
{
    quads.push_back(quad);
    quads.push_back(quad);

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        CreateBuffer(&quads_bo, quads.size() * sizeof(Quad::Data), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
        AllocateFillBuffer(quads.data(), quads.size() * sizeof(Quad::Data), quads_bo.allocation);
    }
    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        CreateBuffer(&instances.bo, instances.data.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // AllocateFillBuffer(instances.data.data(), instances.data.size() * sizeof(InstanceData), instances.bo.allocation);

        vmaMapMemory(vkr.allocator, instances.bo.allocation, &instances_data_ptr);
        memcpy(instances_data_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));
    }


    // glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0., 2., 0.));
    // glm::mat4 scale       = glm::scale(glm::mat4 { 1.0 }, glm::vec3(1., 1., 1.));
    // quad.transform_m4     = translation * scale;




#if EXAMPLE_DRAW_ONE_TRIANGLE_PER_BUFFER
    for (size_t i = 0; i < TEST_TRIANGLES_COUNT; i++) {
        test_triangles[i].vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
        test_triangles[i].vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
        test_triangles[i].vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };

        CreateBuffer(&test_triangles[i].buffer_object, sizeof(Vertex) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        AllocateFillBuffer(&test_triangles->vertices, sizeof(Vertex) * 3, test_triangles[i].buffer_object.allocation);

        static int j = 0;
        if ((i % 10) == 0) j++;

        glm::mat4 translation          = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0. + i * .1, 0. + j * .1, 0.));
        glm::mat4 scale                = glm::scale(glm::mat4 { 1.0 }, glm::vec3(.2, .2, .2));
        test_triangles[i].transform_m4 = translation * scale;
    }
#endif // EXAMPLE_DRAW_ONE_TRIANGLE_PER_BUFFER





#if EXAMPLE_CPU_BOUND_DRAW_TRIANGLE_CUBE_BATCH

    for (size_t z = 0; z < SIDE_LENGTH; z++) {
        for (size_t y = 0; y < SIDE_LENGTH; y++) {
            for (size_t x = 0; x < SIDE_LENGTH; x++) {

                static int i = 0;

                test_triangles[i].vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.9f, .6f, .2f } };
                test_triangles[i].vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.9f, .6f, .2f } };
                test_triangles[i].vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.9f, .6f, .2f } };

                glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0. + x * .5, -10. + y * .5, 0. + z * .5));
                glm::mat4 scale       = glm::scale(glm::mat4 { 1.0 }, glm::vec3(.1, .1, .1));
                transform_m4[i]       = translation * scale;

                i++;
                if (i == TEST_TRIANGLES_COUNT) break;
            }
        }
    }

    CreateBuffer(&triangles_vbo, sizeof(test_triangles), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    AllocateFillBuffer(&test_triangles, sizeof(test_triangles), triangles_vbo.allocation);


#endif // EXAMPLE_CPU_BOUND_DRAW_TRIANGLE_CUBE_BATCH


#if EXAMPLE_DRAW_INDIRECT_CUBE_BATCH
    std::vector<VkDrawIndexedIndirectCommand> draw_indirect_commands;

    uint32_t indices[] = {
        0, 1, 2
    };

    CreateBuffer(&triangles_ibo, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    AllocateFillBuffer(&indices, sizeof(indices), triangles_ibo.allocation);

    for (size_t z = 0; z < SIDE_LENGTH; z++) {
        for (size_t y = 0; y < SIDE_LENGTH; y++) {
            for (size_t x = 0; x < SIDE_LENGTH; x++) {

                static int i = 0;

                test_triangles[i].vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.9f, .6f, .2f } };
                test_triangles[i].vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.9f, .6f, .2f } };
                test_triangles[i].vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.9f, .6f, .2f } };

                glm::mat4 translation   = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0. + x * .5, -0. + y * .5, 0. + z * .5));
                glm::mat4 scale         = glm::scale(glm::mat4 { 1.0 }, glm::vec3(.1, .1, .1));
                model_data[i].transform = translation * scale;


                i++;
                if (i == TEST_TRIANGLES_COUNT) break;
            }
        }
    }

    CreateBuffer(&triangles_vbo, sizeof(test_triangles), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    AllocateFillBuffer(&test_triangles, sizeof(test_triangles), triangles_vbo.allocation);

    // for (size_t i = 0; i < 1; i++)
    {
        VkDrawIndexedIndirectCommand draw_indirect_command = {
            // .vertexCount   = 3,//ARR_COUNT(test_triangles) * 3,
            // .instanceCount = 1,
            // .firstVertex   = 0,
            // .firstInstance = 0,
            .indexCount    = 3,
            .instanceCount = 1,
            .firstIndex    = 0,
            .vertexOffset  = (uint32_t)sizeof(JustATriange),
            .firstInstance = 0,
        };
        draw_indirect_commands.push_back(draw_indirect_command);
    }

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    CreateBuffer(&indirect_commands_buffer, draw_indirect_commands.size() * sizeof(VkDrawIndirectCommand), usage, VMA_MEMORY_USAGE_CPU_TO_GPU);
    AllocateFillBuffer(draw_indirect_commands.data(), draw_indirect_commands.size() * sizeof(VkDrawIndirectCommand), indirect_commands_buffer.allocation);

#endif // EXAMPLE_DRAW_INDIRECT_CUBE_BATCH
}




// pipeline, piepeline_layout, descriptor_sets, shader --> custom to a mesh or defaults

///////////////////////////////////////////////////////////////
/// Mesh rendering
void DrawExamples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer, double dt)
{
    // Bindings
    vkCmdBindPipeline(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline);

    const uint32_t dynamic_offsets = 0;
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 0, 1, &vkr.frames[vkr.frame_idx_inflight].global_descriptor, 0, NULL);

    {
        // push constants
        // make a model view matrix for rendering the object
        // model matrix is specific to the "thing" we want to draw
        // so each model or triangle has its matrix so that it can be positioned on screen with respect to the camera position

        VkBuffer     buffers[] = { quads_bo.buffer };
        VkDeviceSize offsets[] = { 0, /*offsetof(Quad, data.indices) */ };
        vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, buffers, offsets);
        // vkCmdBindIndexBuffer(*cmd_buffer, quad.buffer_object.buffer, sizeof(quad.data.vertices), VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(*cmd_buffer, 1, 1, &instances.bo.buffer, offsets);


        // glm::vec3 quad_pos = { pos_x, 0, pos_z };
        glm::mat4 model = glm::translate(quad.transform_m4, { 0, 0, 0 });

        MeshPushConstants constants;
        constants.render_matrix = model;
        vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        instances.data[1].pos = { pos_x, 0, pos_z };
        memcpy(instances_data_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));

        vkCmdDraw(*cmd_buffer, ARR_COUNT(quad.data.vertices), quads.size(), 0, 0);
        // vkCmdDrawIndexed(*cmd_buffer, 6, 2, 0, 0, 0);
    }





#if EXAMPLE_DRAW_ONE_TRIANGLE_PER_BUFFER

    for (size_t i = 0; i < TEST_TRIANGLES_COUNT; i++) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, &test_triangles[i].buffer_object.buffer, &offset);

        glm::vec3 triangle_pos = { pos_x, 0, pos_z };
        glm::mat4 model        = glm::translate(test_triangles[i].transform_m4, triangle_pos);

        // final render matrix, that we are calculating on the cpu
        // glm::mat4 mesh_matrix = projection * view * model;
        MeshPushConstants constants = {};
        constants.render_matrix     = model;
        vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        uint32_t vertex_count = sizeof(test_triangles->vertices) / sizeof(test_triangles->vertices[0]);
        vkCmdDraw(*cmd_buffer, vertex_count, 1, 0, 0);
    }
#endif // EXAMPLE_DRAW_ONE_TRIANGLE_PER_BUFFER





#if EXAMPLE_CPU_BOUND_DRAW_TRIANGLE_CUBE_BATCH

    for (size_t i = 0; i < TEST_TRIANGLES_COUNT; i++) {
        VkDeviceSize offset = sizeof(JustATriange) * i; // let's pray
        // glm::vec3 triangle_pos = { pos_x, 0, pos_z };

        if (i == 0) {
            vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, &triangles_vbo.buffer, &offset);
            // glm::vec3 triangle_pos         = { pos_x - 10, 0, pos_z };
            // glm::mat4 first_triangle_model = glm::translate(transform_m4[0], triangle_pos);

            // MeshPushConstants first_triangle_constant = {};
            // first_triangle_constant.render_matrix     = first_triangle_model;
            // vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &first_triangle_constant);
        }

        MeshPushConstants constants = {};

        glm::mat4 model         = transform_m4[i];
        constants.render_matrix = model;
        vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
        // // final render matrix, that we are calculating on the cpu
        // // glm::mat4 mesh_matrix = projection * view * model;

        // ---------- cant do this with only one draw call

        vkCmdDraw(*cmd_buffer, 3, 1, 0, 0);
    }
#endif





#if EXAMPLE_DRAW_INDIRECT_CUBE_BATCH

    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, &triangles_vbo.buffer, &offsets);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 1, 1, &vkr.frames[vkr.frame_idx_inflight].model_descriptor, 0, NULL);
    vkCmdBindIndexBuffer(*cmd_buffer, triangles_ibo.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

    // static int i = 0;
    // MeshPushConstants constants = {};
    // glm::mat4 model         = triangle_transforms[i++ % 8];
    // constants.render_matrix = model;
    // vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);


    // void *data;
    // vmaMapMemory(vkr.allocator, vkr.triangle_SSBO[vkr.frame_idx_inflight].allocation, &data);
    // vmaUnmapMemory(vkr.allocator, vkr.triangle_SSBO[vkr.frame_idx_inflight].allocation);
    // std::vector<VkDrawIndirectCommand> draw_indirect_commands;
    // for (uint32_t i = 0; i < 1; i++) {
    //     VkDrawIndirectCommand draw_indirect_command = {
    //         .vertexCount   = ARR_COUNT(test_triangles) * 3,
    //         .instanceCount = 1,
    //         .firstVertex   = 0,
    //         .firstInstance = 0,
    //     };
    //     draw_indirect_commands.push_back(draw_indirect_command);
    // }

    AllocateFillBuffer(&model_data, sizeof(model_data), vkr.triangle_SSBO[vkr.frame_idx_inflight].allocation);

    vkCmdDrawIndexedIndirect(*cmd_buffer, indirect_commands_buffer.buffer, sizeof(JustATriange) * 0, 1, sizeof(VkDrawIndexedIndirectCommand));
    // vkCmdDrawIndexedIndirect(*   cmd_buffer, &indirect_commands_buffer, 0, 1, sizeof(VkDrawIndirectCommand));
    // vkCmdDraw(cmd_buffer, ARR_COUNT(test_triangles) * 3, 1, 0, )
    // https://vkguide.dev/docs/chapter-4/storage_buffers/
#endif // EXAMPLE_DRAW_INDIRECT_CUBE_BATCH
}





/////////////////////////////////////////////////////////////////////////////////////
// just keeping this around for reference
static void draw_Renderables(VkCommandBuffer cmd, RenderObject *first, int count)
{
    // make a model view matrix for rendering the object
    // camera view
    glm::vec3 cam_pos; //= { camera_x, 0.f + camera_y, -30.f + camera_z };

    glm::mat4 view = glm::translate(glm::mat4(1.f), cam_pos);
    // camera projection
    glm::mat4 projection = glm::perspective(glm::radians(60.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    Mesh     *lastMesh     = NULL;
    Material *lastMaterial = NULL;

    for (int i = 0; i < count; i++) {
        RenderObject &object = first[i];

        // only bind the pipeline if it doesn't match with the already bound one
        if (object.material != lastMaterial) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_CurrentFrameData().global_descriptor, 0, NULL);
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
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertex_buffer.buffer, &offset);
            lastMesh = object.mesh;
        }
        // we can now draw
        uint32_t size = static_cast<uint32_t>(object.mesh->vertices.size());
        vkCmdDraw(cmd, size, 1, 0, 0);
    }
}
