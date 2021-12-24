#include "vk_renderer.h"
#include "vk_types.h"

extern VmaAllocator     g_allocator;
extern VkPipeline       g_default_pipeline;
extern VkPipelineLayout g_default_pipeline_layout;
extern float            camera_x, camera_y, camera_z;

BufferObject triangle_vertex_buffer;
glm::mat4    triangle_transform_matrix = {};

struct Triangle
{
    BufferObject buffer_object;
    Vertex       vertices[3];
};

Triangle triangle;

void init_Examples(VkDevice device, VkPhysicalDevice gpu)
{
    triangle.vertices[0] = { { 0.2f, 0.2f, 0.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    triangle.vertices[1] = { { -0.2f, 0.2f, 0.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };
    triangle.vertices[2] = { { 0.f, -0.2f, 0.0f }, { 0.f, 0.f, -1.f }, { 0.2f, .6f, .2f } };

    /////////////////////////////////////////////////////////////
    /// getting vertex data to gpu without vma
    VkBufferCreateInfo create_info_vertex_buffer = {};
    create_info_vertex_buffer.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info_vertex_buffer.sharingMode        = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    create_info_vertex_buffer.size               = sizeof(triangle);
    create_info_vertex_buffer.usage              = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    // VkBuffer vertex_buffer;
    VK_CHECK(vkCreateBuffer(device, &create_info_vertex_buffer, NULL, &triangle_vertex_buffer.buffer));
    if (!allocateBufferMemory(device, gpu, triangle_vertex_buffer.buffer, &triangle.buffer_object.device_memory))
        printf("failed to allocate memory\n");

    // bind what we just allocated
    VK_CHECK(vkBindBufferMemory(device, triangle_vertex_buffer.buffer, triangle.buffer_object.device_memory, 0));

    VkDeviceSize offset = 0;
    void        *data;
    VK_CHECK(vkMapMemory(device, triangle.buffer_object.device_memory, offset, VK_WHOLE_SIZE, 0, &data));
    memcpy(data, &triangle.vertices, sizeof(triangle.vertices));
    vkUnmapMemory(device, triangle.buffer_object.device_memory);

    ////////////////////////////////////
    glm::mat4 translation     = glm::translate(glm::mat4 { 1.0 }, glm::vec3(0., 0., 0.));
    glm::mat4 scale           = glm::scale(glm::mat4 { 1.0 }, glm::vec3(1., 1., 1.));
    triangle_transform_matrix = translation * scale;
}

// pipeline, piepeline_layout, descriptor_sets, shader --> custom to a mesh or defaults

void draw_examples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer)
{
    // Bindings
    vkCmdBindPipeline(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_default_pipeline);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_default_pipeline_layout, 0, 1, descriptor_set, 0, NULL);
    VkDeviceSize offsets = { 0 };
    vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, &triangle_vertex_buffer.buffer, &offsets);

    // camera view
    glm::vec3 camPos = { camera_x, 0.f + camera_y, -30.f + camera_z };
    glm::mat4 view   = glm::translate(glm::mat4(1.f), camPos);
    // camera projection
    glm::mat4 projection = glm::perspective(glm::radians(60.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    CameraData camData;
    camData.projection = projection;
    camData.view       = view;
    camData.viewproj   = projection * view;

    // copy it to the buffer
    void *data;
    vmaMapMemory(g_allocator, (*camera_buffer)._allocation, &data);
    memcpy(data, &camData, sizeof(CameraData));
    vmaUnmapMemory(g_allocator, (*camera_buffer)._allocation);

    // push constants
    // make a model view matrix for rendering the object
    // model matrix is specific to the "thing" we want to draw
    // so each model or triangle has its matrix so that it can be positioned on screen with respect to the camera position
    glm::mat4 model = triangle_transform_matrix;
    // final render matrix, that we are calculating on the cpu
    glm::mat4 mesh_matrix = projection * view * model;

    MeshPushConstants constants;
    constants.render_matrix = triangle_transform_matrix;

    // upload the mesh constants to the GPU via pushconstants
    vkCmdPushConstants(*cmd_buffer, g_default_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

    uint32_t size = static_cast<uint32_t>(sizeof(triangle.vertices));
    vkCmdDraw(*cmd_buffer, sizeof(triangle.vertices), 1, 0, 0);
}

// misc
static void draw_Renderables(VkCommandBuffer cmd, RenderObject *first, int count)
{
    // make a model view matrix for rendering the object
    // camera view
    glm::vec3 camPos; //= { camera_x, 0.f + camera_y, -30.f + camera_z };

    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    // camera projection
    glm::mat4 projection = glm::perspective(glm::radians(60.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    // fill a GPU camera data struct
    CameraData camData;
    camData.projection = projection;
    camData.view       = view;
    camData.viewproj   = projection * view;

    // and copy it to the buffer
    void *data;
    vmaMapMemory(g_allocator, get_CurrentFrame().camera_buffer._allocation, &data);

    memcpy(data, &camData, sizeof(CameraData));

    vmaUnmapMemory(g_allocator, get_CurrentFrame().camera_buffer._allocation);

    Mesh     *lastMesh     = NULL;
    Material *lastMaterial = NULL;

    for (int i = 0; i < count; i++) {
        RenderObject &object = first[i];

        // only bind the pipeline if it doesn't match with the already bound one
        if (object.material != lastMaterial) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_CurrentFrame().global_descriptor, 0, NULL);
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
