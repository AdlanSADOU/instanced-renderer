#include <Vkay.h>

struct Camera
{
    enum Projection
    {
        PERSPECTIVE = 0,
        ORTHO,
    };

    struct Data
    {
        glm::mat4 view       = {};
        glm::mat4 projection = {};
        glm::mat4 viewproj   = {};
    } m_data {};

    void            *m_data_ptr[FRAME_BUFFER_COUNT];
    VkayBuffer       m_UBO[FRAME_BUFFER_COUNT];
    VkDescriptorSet  m_set_global[FRAME_BUFFER_COUNT] = {};
    VkPipelineLayout m_pipeline_layout;
    VkayRenderer    *m_vkr;
    Projection       m_projection = PERSPECTIVE;

    glm::vec3 m_position;
};

void VkayCameraCreate(VkayRenderer *vkr, Camera *camera)
{
    camera->m_vkr             = vkr;
    camera->m_pipeline_layout = vkr->default_pipeline_layout;

    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {

        VkayCreateBuffer(&camera->m_UBO[i], vkr->allocator, sizeof(Camera::Data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info_descriptor_camera_buffer;
        AllocateDescriptorSets(vkr->device, vkr->descriptor_pool, 1, &vkr->set_layout_camera, &camera->m_set_global[i]);

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

void VkayCameraUpdate(VkayRenderer *vkr, Camera *camera, VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(vkr->frames[vkr->frame_idx_inflight].cmd_buffer_gfx, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &camera->m_set_global[vkr->frame_idx_inflight], 0, NULL);
    glm::mat4 translation = glm::translate(glm::mat4(1.f), camera->m_position);
    glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), glm::radians(0.f), glm::vec3(1, 0, 0));
    glm::mat4 view        = rotation * translation;

    glm::mat4 projection = glm::mat4(1);
    if (camera->m_projection == Camera::PERSPECTIVE) {
        projection                = glm::perspective<float>(glm::radians(60.f), (float)vkr->window_extent.width / (float)vkr->window_extent.height, -1.f, 1000.0f);
        glm::mat4 V               = glm::lookAt(glm::vec3(0.f, 0.f, 2.0f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f));
        camera->m_data.projection = projection * V;
        projection[1][1] *= 1;
    }
    else if (camera->m_projection == Camera::ORTHO)
    {
        projection                = glm::ortho<float>(0.f, (float)vkr->window_extent.width, (float)vkr->window_extent.height, 0.f, 0.f, -1.f);
        camera->m_data.projection = projection;
    }


    camera->m_data.view     = view;
    camera->m_data.viewproj = projection * view;
    memcpy(camera->m_data_ptr[vkr->frame_idx_inflight], &camera->m_data, sizeof(camera->m_data));
}

void VkayCameraDestroy(VkayRenderer *vkr, Camera *camera)
{
    vkDeviceWaitIdle(vkr->device);

    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
        vmaUnmapMemory(vkr->allocator, camera->m_UBO[i].allocation);
        VkayDestroyBuffer(vkr->allocator, camera->m_UBO[i].buffer, camera->m_UBO[i].allocation);
    }
}