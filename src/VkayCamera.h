#include <VkayRenderer.h>

struct Camera
{
    struct Data
    {
        glm::mat4 view       = {};
        glm::mat4 projection = {};
        glm::mat4 viewproj   = {};
    } data {};

    void           *m_data_ptr[FRAME_BUFFER_COUNT];
    BufferObject    m_UBO[FRAME_BUFFER_COUNT];
    VkDescriptorSet m_set_global[FRAME_BUFFER_COUNT] = {};
    VkayRenderer   *m_vkr;

    glm::vec3 m_position;
};

void VkayCameraCreate(VkayRenderer *vkr, Camera *camera)
{
    camera->m_vkr = vkr;

    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {

        CreateBuffer(&camera->m_UBO[i], vkr->allocator, sizeof(Camera::Data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info_descriptor_camera_buffer;
        AllocateDescriptorSets(vkr->device, vkr->descriptor_pool, 1, &vkr->set_layout_global, &camera->m_set_global[i]);

        info_descriptor_camera_buffer.buffer = camera->m_UBO[i].buffer;
        info_descriptor_camera_buffer.offset = 0;
        info_descriptor_camera_buffer.range  = sizeof(Camera::Data);

        VkWriteDescriptorSet write_camera_buffer = {};
        write_camera_buffer.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_camera_buffer.pNext                = NULL;
        write_camera_buffer.dstBinding           = 0; // we are going to write into binding number 0
        write_camera_buffer.dstSet               = camera->m_set_global[i]; // of the global descriptor
        write_camera_buffer.descriptorCount      = 1;
        write_camera_buffer.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // and the type is uniform buffer
        write_camera_buffer.pBufferInfo          = &info_descriptor_camera_buffer;
        vkUpdateDescriptorSets(vkr->device, 1, &write_camera_buffer, 0, NULL);

        vmaMapMemory(vkr->allocator, camera->m_UBO[i].allocation, &camera->m_data_ptr[i]);
    }
}

void VkayCameraUpdate(VkayRenderer *vkr, Camera *camera)
{
    vkCmdBindDescriptorSets(vkr->frames[vkr->frame_idx_inflight].cmd_buffer_gfx, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline_layout, 0, 1, &camera->m_set_global[vkr->frame_idx_inflight], 0, NULL);

    glm::mat4 translation = glm::translate(glm::mat4(1.f), camera->m_position);
    glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), glm::radians(0.f), glm::vec3(1, 0, 0));
    glm::mat4 view        = rotation * translation;
    // glm::mat4 projection  = glm::perspective(glm::radians(60.f), (float)vkr->window_extent.width / (float)vkr->window_extent.height, 0.1f, 1000.0f);
    glm::mat4 projection = glm::ortho(0.f, (float)vkr->window_extent.width, 0.f, (float)vkr->window_extent.height, .1f, 200.f);
    glm::mat4 V          = glm::lookAt(glm::vec3(0.f, 0.f, 2.0f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f));

    projection[1][1] *= -1;

    camera->data.projection = projection * V;
    camera->data.view       = view;
    camera->data.viewproj   = projection * view;
    memcpy(camera->m_data_ptr[vkr->frame_idx_inflight], &camera->data, sizeof(camera->data));
}

void VkayCameraDestroy(VkayRenderer *vkr, Camera *camera)
{
    vkDeviceWaitIdle(vkr->device);


    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
        vmaUnmapMemory(vkr->allocator, camera->m_UBO[i].allocation);
        vmaDestroyBuffer(vkr->allocator, camera->m_UBO[i].buffer, camera->m_UBO[i].allocation);
    }
}