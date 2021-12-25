#include "vk_renderer.h"
#include "vk_types.h"

extern VulkanRenderer vkr;

extern float camera_x, camera_y, camera_z;
extern float pos_x, pos_y, pos_z;

struct Triangle
{
    BufferObject buffer_object = {};
    Vertex       vertices[3]   = {};
    glm::mat4    transform_m4  = {};
};

Triangle triangle = {};

#define TEST_TRIANGLES_COUNT 100
Triangle test_triangles[TEST_TRIANGLES_COUNT];

void InitExamples(VkDevice device, VkPhysicalDevice gpu)
{
    triangle.vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    triangle.vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    triangle.vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };

    // for (size_t i = 0; i < TEST_TRIANGLES_COUNT; i++) {
    //     test_triangles[i].vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    //     test_triangles[i].vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    //     test_triangles[i].vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    // }

    BufferCreate(&triangle.buffer_object, sizeof(triangle.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    BufferFill(&triangle.vertices, sizeof(triangle.vertices), triangle.buffer_object.allocation);

    ////////////////////////////////////
    glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0., 0., 0.));
    glm::mat4 scale       = glm::scale(glm::mat4 { 1.0 }, glm::vec3(1., 1., 1.));

    triangle.transform_m4 = translation * scale;
}

// pipeline, piepeline_layout, descriptor_sets, shader --> custom to a mesh or defaults

void DrawExamples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer, double dt)
{
    // Bindings
    vkCmdBindPipeline(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 0, 1, descriptor_set, 0, NULL);
    VkDeviceSize offsets = { 0 };

    // camera view
    glm::vec3 cam_pos = { camera_x, 0.f + camera_y - 10, -30.f + camera_z };
    glm::mat4 view    = glm::translate(glm::mat4(1.f), cam_pos);

    view = glm::rotate(view, .80f, glm::vec3(1, 0, 0));

    glm::mat4 projection = glm::perspective(glm::radians(60.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    vkr.cam_data.projection = projection;
    vkr.cam_data.view       = view;
    vkr.cam_data.viewproj   = projection * view;

    BufferFill(&vkr.cam_data, sizeof(vkr.cam_data), (*camera_buffer).allocation);

    // push constants
    // make a model view matrix for rendering the object
    // model matrix is specific to the "thing" we want to draw
    // so each model or triangle has its matrix so that it can be positioned on screen with respect to the camera position
    vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, &triangle.buffer_object.buffer, &offsets);
    glm::vec3 triangle_pos = { pos_x, 0, pos_z };
    glm::mat4 model        = glm::translate(triangle.transform_m4, triangle_pos);

    // final render matrix, that we are calculating on the cpu
    // glm::mat4 mesh_matrix = projection * view * model;

    MeshPushConstants constants;
    constants.render_matrix = model;

    // upload the mesh constants to the GPU via pushconstants
    vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

    uint32_t size = (sizeof(triangle.vertices));
    vkCmdDraw(*cmd_buffer, sizeof(triangle.vertices), 1, 0, 0);
}

// misc
static void draw_Renderables(VkCommandBuffer cmd, RenderObject *first, int count)
{
    // make a model view matrix for rendering the object
    // camera view
    glm::vec3 cam_pos; //= { camera_x, 0.f + camera_y, -30.f + camera_z };

    glm::mat4 view = glm::translate(glm::mat4(1.f), cam_pos);
    // camera projection
    glm::mat4 projection = glm::perspective(glm::radians(60.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    // fill a GPU camera data struct
    CameraData cam_data;
    cam_data.projection = projection;
    cam_data.view       = view;
    cam_data.viewproj   = projection * view;

    // and copy it to the buffer
    void *data;
    vmaMapMemory(vkr.allocator, get_CurrentFrameData().camera_buffer.allocation, &data);
    memcpy(data, &cam_data, sizeof(CameraData));
    vmaUnmapMemory(vkr.allocator, get_CurrentFrameData().camera_buffer.allocation);

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
