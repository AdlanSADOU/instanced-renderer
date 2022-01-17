#include "vk_renderer.h"
#include "vk_types.h"
#include "vk_texture.h"
#include "vk_instances.h"

extern VulkanRenderer vkr;

extern float pos_x, pos_y, pos_z;
extern float camera_x, camera_y, camera_z;
extern bool  _key_r;

Instances instances;

Texture profile;
Texture itoldu;


const size_t ROW     = 1000;
const size_t COL     = 1000;
int          spacing = 50;

Camera camera;

// Compute
BufferObject instance_data_storage_buffer;
void        *instance_data_storage_buffer_ptr;

void InitExamples()
{

    profile.Create("./assets/texture.png", &vkr);
    itoldu.Create("./assets/bjarn_itoldu.jpg", &vkr);

    camera.MapDataPtr(vkr.allocator, vkr.device, vkr.chosen_gpu, &vkr);

    int MAX_ENTITES = ROW * COL;
    SDL_Log("max entites: %d\n", MAX_ENTITES);


    InstanceData instance_data;

    for (size_t i = 0, j = 0; i < MAX_ENTITES; i++) {
        static float _x     = 0;
        static float _y     = 0;
        float        _scale = .03f;

        if (i > 0 && (i % ROW) == 0) j++;

        if (i == 0) _scale = .1f;
        _x = (float)((profile.width + spacing) * (i % ROW) * _scale + 50);
        _y = (float)((profile.height + spacing) * j) * _scale + 50;


        instance_data.pos = { _x, -_y, -1 };
        if (i == 0) instance_data.pos = { _x + vkr.window_extent.width / 2 - profile.width * .1f, -_y - vkr.window_extent.height / 2, 1.1f };

        instance_data.texure_id = profile.id;
        instance_data.scale     = { profile.width, profile.height, 0 };
        instance_data.scale *= _scale;

        instances.m_data.push_back(instance_data);
    }
    instances.Upload(&vkr);

    // descriptorImageInfos requires VkImageView(s) which requires VkImage(s) backing actual texture data
    // we need to finish texture loading in vk_texture.h
    // we must be able to load textures whenever necessary
    VkDescriptorSetAllocateInfo desc_alloc_info = {};
    desc_alloc_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_alloc_info.descriptorPool              = vkr.descriptor_pool;
    desc_alloc_info.descriptorSetCount          = 1;
    desc_alloc_info.pSetLayouts                 = &vkr.set_layout_array_of_textures;

    VK_CHECK(vkAllocateDescriptorSets(vkr.device, &desc_alloc_info, &vkr.set_array_of_textures));

    VkWriteDescriptorSet setWrites[2];

    VkDescriptorImageInfo samplerInfo = {};
    samplerInfo.sampler               = vkr.sampler;
    setWrites[0]                      = {};
    setWrites[0].dstSet               = vkr.set_array_of_textures;
    setWrites[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrites[0].dstBinding           = 0;
    setWrites[0].dstArrayElement      = 0;
    setWrites[0].descriptorCount      = 1;
    setWrites[0].descriptorType       = VK_DESCRIPTOR_TYPE_SAMPLER;
    setWrites[0].pImageInfo           = &samplerInfo;
    setWrites[0].pBufferInfo          = 0;

    setWrites[1]                 = {};
    setWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrites[1].dstSet          = vkr.set_array_of_textures;
    setWrites[1].dstBinding      = 1;
    setWrites[1].dstArrayElement = 0;
    setWrites[1].descriptorCount = (uint32_t)vkr.descriptor_image_infos.size();
    setWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    setWrites[1].pImageInfo      = vkr.descriptor_image_infos.data();
    setWrites[1].pBufferInfo     = 0;
    vkUpdateDescriptorSets(vkr.device, 2, setWrites, 0, NULL);
}

void DestroyExamples()
{
    vkDeviceWaitIdle(vkr.device);

    profile.Destroy(&vkr);
    itoldu.Destroy(&vkr);

    vmaDestroyBuffer(vkr.allocator, instances.m_quads_bo.buffer, instances.m_quads_bo.allocation);

    vmaUnmapMemory(vkr.allocator, instances.m_bo.allocation);
    vmaDestroyBuffer(vkr.allocator, instances.m_bo.buffer, instances.m_bo.allocation);

    camera.Destroy(vkr.device, vkr.allocator);
}

void ComputeExamples()
{
    vkWaitForFences(vkr.device, 1, &vkr.frames[0].compute_fence, true, SECONDS(1));
    vkResetFences(vkr.device, 1, &vkr.frames[0].compute_fence);
    vkResetCommandBuffer(vkr.frames[0].cmd_buffer_cmp, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(vkr.frames[0].cmd_buffer_cmp, &begin_info);

    vkCmdBindPipeline(vkr.frames[0].cmd_buffer_cmp, VK_PIPELINE_BIND_POINT_COMPUTE, vkr.compute_pipeline);
    vkCmdBindDescriptorSets(vkr.frames[0].cmd_buffer_cmp, VK_PIPELINE_BIND_POINT_COMPUTE, vkr.compute_pipeline_layout, 0, 1, vkr.compute_descriptor_sets.data(), 0, 0);

    vkCmdPushConstants(vkr.frames[0].cmd_buffer_cmp, vkr.compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float_t), &pos_x);

    vkCmdDispatch(vkr.frames[0].cmd_buffer_cmp, (uint32_t)(instances.m_data.size()), 1, 1);
    // CopyBuffer(vkr.frames[0].cmd_buffer_cmp, instance_data_storage_buffer.buffer, instances.bo.buffer, instances.data.size() * sizeof(InstanceData));

    vkEndCommandBuffer(vkr.frames[0].cmd_buffer_cmp);

    VkSubmitInfo submit_info         = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 0;
    submit_info.pWaitSemaphores      = NULL;
    submit_info.pWaitDstStageMask    = NULL;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &vkr.frames[0].cmd_buffer_cmp;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores    = NULL;

    vkQueueSubmit(vkr.compute_queue, 1, &submit_info, vkr.frames[0].compute_fence);
}

// pipeline, piepeline_layout, descriptor_sets, shader --> custom to a mesh or defaults
void DrawExamples(VkCommandBuffer cmd_buffer, double dt)
{
    // Bindings


    camera.m_position = { camera_x, camera_y - 0.f, camera_z - 120.f };
    camera.Update(cmd_buffer);

    static int i = 0;
    if (_key_r) {
        instances.DestroyInstance(&vkr, i++);
    }

    // instances.data[0].pos.x += pos_x;
    // instances.data[0].pos.y += pos_y;
    // // instances.data[0].rot.z += pos_x;
    // instances.data[0].rot.x += (float)(pos_z * PI / 180);
    // instances.data[0].scale *= pos_z*.1f+1;

    // vkCmdDrawIndexed(cmd_buffer, 6, 2, 0, 0, 0);
    instances.Draw(cmd_buffer, &vkr);
}
