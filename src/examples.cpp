#include "vk_renderer.h"
#include "vk_types.h"
#include <vk_texture.h>

extern VulkanRenderer vkr;

extern float pos_x, pos_y, pos_z;
extern float camera_x, camera_y, camera_z;

Instances instances;

BufferObject      quads_bo = {};
std::vector<Quad> quads;
Quad              quad;

void *instances_data_ptr;

Texture profile;
Texture itoldu;

InstanceData quad1_idata;
InstanceData quad2_idata;

const size_t ROW     = 1;
const size_t COL     = 1;
int          spacing = 300;

Camera camera;

// Compute
BufferObject instance_data_storage_buffer;
void        *instance_data_storage_buffer_ptr;

void InitExamples()
{
    CreateTexture(&profile, "./assets/texture.png", &vkr);
    CreateTexture(&itoldu, "./assets/bjarn_itoldu.jpg", &vkr);

    camera.MapDataPtr(vkr.allocator, vkr.device, vkr.chosen_gpu, vkr);

    int MAX_ENTITES = ROW * COL;
    SDL_Log("max entites: %d\n", MAX_ENTITES);

    for (size_t i = 0, j = 0; i < MAX_ENTITES; i++) {
        static float _x     = 0;
        static float _y     = 0;
        float        _scale = .001f;

        if (i > 0 && (i % ROW) == 0) j++;

        if (i == 0) _scale = .1f;
        _x = (float)((profile.width + spacing) * (i % ROW) * _scale + 50);
        _y = (float)((profile.height + spacing) * j) * _scale + 50;


        quad1_idata.pos = { _x, -_y, -1 };
        if (i == 0) quad1_idata.pos = { _x + vkr.window_extent.width / 2 - profile.width * .1f, -_y - vkr.window_extent.height / 2, 1.1f };

        quad1_idata.tex_idx = profile.array_index;
        quad1_idata.scale   = { profile.width, profile.height, 0 };
        quad1_idata.scale *= _scale;

        // quad2_idata.pos     = { i * 20, j * 20, 0 };
        // quad2_idata.tex_idx = itoldu.array_index;
        // quad2_idata.scale   = { itoldu.width, itoldu.height, 0 };
        // quad2_idata.scale *= _scale;

        instances.data.push_back(quad1_idata);
        // instances.data.push_back(quad2_idata);
    }
    quads.push_back(quad);

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        CreateBuffer(&quads_bo, vkr.allocator, &vkr.release_queue, quads.size() * sizeof(Quad::vertices), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
        MapMemcpyMemory(quads.data(), quads.size() * sizeof(Quad::vertices), vkr.allocator, quads_bo.allocation);
    }

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        CreateBuffer(&instances.bo, vkr.allocator, &vkr.release_queue, instances.data.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);

        vmaMapMemory(vkr.allocator, instances.bo.allocation, &instances_data_ptr);
        memcpy(instances_data_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));
    }

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        CreateBuffer(&instance_data_storage_buffer, vkr.allocator, &vkr.release_queue, instances.data.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
        vmaMapMemory(vkr.allocator, instance_data_storage_buffer.allocation, &instance_data_storage_buffer_ptr);
        memcpy(instance_data_storage_buffer_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));
    }



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


    VkDescriptorBufferInfo storage_buffer_info = {};
    storage_buffer_info.buffer                 = instance_data_storage_buffer.buffer;
    storage_buffer_info.offset                 = 0;
    storage_buffer_info.range                  = instances.data.size() * sizeof(InstanceData);

    VkWriteDescriptorSet inst_data_descriptor_write = {};
    inst_data_descriptor_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    inst_data_descriptor_write.pNext;
    inst_data_descriptor_write.dstSet     = vkr.compute_descriptor_sets[0]; // set containing binding for InstanceData Storage Buffer
    inst_data_descriptor_write.dstBinding = 0; // Storage Buffer binding
    inst_data_descriptor_write.dstArrayElement;
    inst_data_descriptor_write.descriptorCount = 1;
    inst_data_descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inst_data_descriptor_write.pImageInfo;
    inst_data_descriptor_write.pBufferInfo = &storage_buffer_info;
    inst_data_descriptor_write.pTexelBufferView;
    vkUpdateDescriptorSets(vkr.device, 1, &inst_data_descriptor_write, 0, NULL);
}

void DestroyExamples()
{
    vkDeviceWaitIdle(vkr.device);

    DestroyTextureAsset(&profile, &vkr);
    DestroyTextureAsset(&itoldu, &vkr);

    vmaDestroyBuffer(vkr.allocator, quads_bo.buffer, quads_bo.allocation);

    vmaUnmapMemory(vkr.allocator, instances.bo.allocation);
    vmaDestroyBuffer(vkr.allocator, instances.bo.buffer, instances.bo.allocation);

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

    vkCmdDispatch(vkr.frames[0].cmd_buffer_cmp, (uint32_t)(256 / instances.data.size()), 1, 1);

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
void DrawExamples(VkCommandBuffer *cmd_buffer, double dt)
{

    /////////////////////////////////////////////////
    // camera
    { // camera.update();
        glm::vec3 cam_pos     = { camera_x, camera_y - 0.f, camera_z - 120.f };
        glm::mat4 translation = glm::translate(glm::mat4(1.f), cam_pos);
        glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), glm::radians(0.f), glm::vec3(1, 0, 0));
        glm::mat4 view        = rotation * translation;
        // glm::mat4 projection  = glm::perspective(glm::radians(60.f), (float)vkr.window_extent.width / (float)vkr.window_extent.height, 0.1f, 1000.0f);
        glm::mat4 projection = glm::ortho(0.f, (float)vkr.window_extent.width, 0.f, (float)vkr.window_extent.height, .1f, 200.f);
        glm::mat4 V          = glm::lookAt(glm::vec3(0.f, 0.f, 2.0f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f));

        projection[1][1] *= -1;

        camera.data.projection = projection * V;
        camera.data.view       = view;
        camera.data.viewproj   = projection * view;
        camera.copyDataPtr(vkr.frame_idx_inflight);
    }
    // Bindings
    vkCmdBindPipeline(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 0, 1, &camera.set_global[vkr.frame_idx_inflight], 0, NULL);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 1, 1, &vkr.set_array_of_textures, 0, NULL);

    {

        VkBuffer     buffers[] = { quads_bo.buffer };
        VkDeviceSize offsets[] = { 0, /*offsetof(Quad, data.indices) */ };
        vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, buffers, offsets);
        vkCmdBindVertexBuffers(*cmd_buffer, 1, 1, &instances.bo.buffer, offsets);
        // push constants
        // make a model view matrix for rendering the object
        // model matrix is specific to the "thing" we want to draw
        // so each model or triangle has its matrix so that it can be positioned on screen with respect to the camera position

        InstanceData i_transform = *((InstanceData *)instance_data_storage_buffer_ptr);
        SDL_Log("from shader -> position:   (%f, %f, %f)\n", i_transform.pos.x, i_transform.pos.y, i_transform.pos.z);
        SDL_Log("from shader -> rotation:   (%f, %f, %f)\n", i_transform.rot.x, i_transform.rot.y, i_transform.rot.z);
        SDL_Log("from shader -> scale:      (%f, %f, %f)\n", i_transform.scale.x, i_transform.scale.y, i_transform.scale.z);
        SDL_Log("from shader -> texture_id: (%d)\n", i_transform.tex_idx);

        SDL_Log("-> position:   (%f, %f, %f)\n", instances.data[0].pos.x, instances.data[0].pos.y, instances.data[0].pos.z);
        SDL_Log("-> rotation:   (%f, %f, %f)\n", instances.data[0].rot.x, instances.data[0].rot.y, instances.data[0].rot.z);
        SDL_Log("-> scale:      (%f, %f, %f)\n", instances.data[0].scale.x, instances.data[0].scale.y, instances.data[0].scale.z);
        SDL_Log("-> texture_id: (%d)\n", instances.data[0].tex_idx);
        // instances.data[0].pos.x += pos_x;
        // instances.data[0].pos.y += pos_y;
        // // instances.data[0].rot.z += pos_x;
        // instances.data[0].rot.x += (float)(pos_z * PI / 180);
        // instances.data[0].scale *= pos_z*.1f+1;

        // memcpy(instances_data_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));
        memcpy(instances_data_ptr, instance_data_storage_buffer_ptr, instances.data.size() * sizeof(InstanceData));

        vkCmdDraw(*cmd_buffer, ARR_COUNT(quad.vertices), (uint32_t)instances.data.size(), 0, 0);
        // vkCmdDrawIndexed(*cmd_buffer, 6, 2, 0, 0, 0);
    }
}
