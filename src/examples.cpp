#include "vk_renderer.h"
#include "vk_types.h"

#define DRAW_ONE_TRIANGLE_PER_BUFFER 0
#define DRAW_TRIANGLE_BATCH 1

extern VulkanRenderer vkr;

extern float camera_x, camera_y, camera_z;
extern float pos_x, pos_y, pos_z;

struct Triangle
{
    Vertex       vertices[3]   = {};
    BufferObject buffer_object = {};
    glm::mat4    transform_m4  = {};
};

Triangle triangle = {};

#if DRAW_ONE_TRIANGLE_PER_BUFFER
#define TEST_TRIANGLES_COUNT 10
Triangle test_triangles[TEST_TRIANGLES_COUNT];
#endif

#if DRAW_TRIANGLE_BATCH

#endif

void InitExamples(VkDevice device, VkPhysicalDevice gpu)
{
    triangle.vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    triangle.vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    triangle.vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };

    // got them into a buffer for now, we'll se if this does it
    BufferCreate(&triangle.buffer_object, sizeof(Vertex) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    BufferFill(&triangle.vertices, (sizeof(Vertex) * 3), triangle.buffer_object.allocation);

    //////////////////////////////////
    glm::mat4 translation = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0., 0., 0.));
    glm::mat4 scale       = glm::scale(glm::mat4 { 1.0 }, glm::vec3(1., 1., 1.));

    triangle.transform_m4 = translation * scale;

#if DRAW_ONE_TRIANGLE_PER_BUFFER
    // gotta get this somehow into a vertex buffer
    for (size_t i = 0; i < TEST_TRIANGLES_COUNT; i++) {
        test_triangles[i].vertices[0] = { { -1.f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
        test_triangles[i].vertices[1] = { { 0.0f, 0.0f, -1.f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
        test_triangles[i].vertices[2] = { { 1.0f, 0.0f, 1.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };

        BufferCreate(&test_triangles[i].buffer_object, sizeof(Vertex) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        BufferFill(&test_triangles->vertices, sizeof(Vertex) * 3, test_triangles[i].buffer_object.allocation);

        static int j = 0;
        if ((i % 10) == 0) j++;

        glm::mat4 translation          = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0.+i*.1, 0.+j*.1, 0.));
        glm::mat4 scale                = glm::scale(glm::mat4 { 1.0 }, glm::vec3(.2, .2, .2));
        test_triangles[i].transform_m4 = translation * scale;
    }
#endif // DRAW_ONE_TRIANGLE_PER_BUFFER

#if DRAW_TRIANGLE_BATCH

#endif //DRAW_TRIANGLE_BATCH
}

// pipeline, piepeline_layout, descriptor_sets, shader --> custom to a mesh or defaults

///////////////////////////////////////////////////////////////
/// Mesh rendering
void DrawExamples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer, double dt)
{
    // Bindings
    vkCmdBindPipeline(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 0, 1, descriptor_set, 0, NULL);

    // camera view
    glm::vec3 cam_pos     = { camera_x, 0.f + camera_y - 10, -30.f + camera_z };
    glm::mat4 translation = glm::translate(glm::mat4(1.f), cam_pos);
    glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), .66f, glm::vec3(1, 0, 0));

    glm::mat4 view       = rotation * translation;
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
    {
        VkDeviceSize offsets = { 0 };
        vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, &triangle.buffer_object.buffer, &offsets);
        glm::vec3 triangle_pos = { pos_x, 0, pos_z };
        glm::mat4 model        = glm::translate(triangle.transform_m4, triangle_pos);

        // final render matrix, that we are calculating on the cpu
        // glm::mat4 mesh_matrix = projection * view * model;
        MeshPushConstants constants;
        constants.render_matrix = model;
        vkCmdPushConstants(*cmd_buffer, vkr.default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        uint32_t vertex_count = sizeof(triangle.vertices) / sizeof(triangle.vertices[0]);
        vkCmdDraw(*cmd_buffer, vertex_count, 1, 0, 0);
    }

#if DRAW_ONE_TRIANGLE_PER_BUFFER

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

        uint32_t vertex_count = sizeof(test_triangles->vertices)/sizeof(test_triangles->vertices[0]);
        vkCmdDraw(*cmd_buffer, vertex_count, 1, 0, 0);
    }
#endif // DRAW_ONE_TRIANGLE_PER_BUFFER
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
