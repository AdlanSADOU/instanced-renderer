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

void        *instances_data_ptr;
TextureAsset profile;
TextureAsset itoldu;

InstanceData quad1_idata;
InstanceData quad2_idata;

const size_t ROW     = 500;
const size_t COL     = 500;
int          spacing = 200;

void InitExamples()
{
    CreateTextureAsset(&profile, "./assets/texture.png", &vkr);
    CreateTextureAsset(&itoldu, "./assets/bjarn_itoldu.jpg", &vkr);


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


        quad1_idata.pos = { _x, -_y, 1 };
        if (i == 0) quad1_idata.pos = { _x + vkr.window_extent.width / 2 - profile.width * .1f, -_y - vkr.window_extent.height / 2, 1.1f };

        quad1_idata.tex_idx = profile.array_index;
        quad1_idata.scale   = { profile.width, profile.height, 0 };
        quad1_idata.scale *= _scale;

        // quad2_idata.pos     = { i * 20, j * 20, 0 };
        // quad2_idata.tex_idx = itoldu.array_index;
        // quad2_idata.scale   = { itoldu.width, itoldu.height, 0 };
        // quad2_idata.scale *= _scale;

        quads.push_back(quad);
        instances.data.push_back(quad1_idata);
        // instances.data.push_back(quad2_idata);
    }

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        CreateBuffer(&quads_bo, quads.size() * sizeof(Quad::vertices), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
        AllocateFillBuffer(quads.data(), quads.size() * sizeof(Quad::vertices), quads_bo.allocation);
    }
    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        CreateBuffer(&instances.bo, instances.data.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // AllocateFillBuffer(instances.data.data(), instances.data.size() * sizeof(InstanceData), instances.bo.allocation);

        vmaMapMemory(vkr.allocator, instances.bo.allocation, &instances_data_ptr);
        memcpy(instances_data_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));
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
    setWrites[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrites[0].dstBinding           = 0;
    setWrites[0].dstArrayElement      = 0;
    setWrites[0].descriptorType       = VK_DESCRIPTOR_TYPE_SAMPLER;
    setWrites[0].descriptorCount      = 1;
    setWrites[0].dstSet               = vkr.set_array_of_textures;
    setWrites[0].pBufferInfo          = 0;
    setWrites[0].pImageInfo           = &samplerInfo;

    setWrites[1]                 = {};
    setWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrites[1].dstBinding      = 1;
    setWrites[1].dstArrayElement = 0;
    setWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    setWrites[1].descriptorCount = (uint32_t)vkr.descriptor_image_infos.size();
    setWrites[1].pBufferInfo     = 0;
    setWrites[1].dstSet          = vkr.set_array_of_textures;
    setWrites[1].pImageInfo      = vkr.descriptor_image_infos.data();
    vkUpdateDescriptorSets(vkr.device, 2, setWrites, 0, NULL);
}

void DestroyExamples()
{
    DestroyTextureAsset(&profile, &vkr);
    DestroyTextureAsset(&itoldu, &vkr);
    vmaUnmapMemory(vkr.allocator, instances.bo.allocation);
}

// pipeline, piepeline_layout, descriptor_sets, shader --> custom to a mesh or defaults
void DrawExamples(VkCommandBuffer *cmd_buffer, VkDescriptorSet *descriptor_set, BufferObject *camera_buffer, double dt)
{

    /////////////////////////////////////////////////
    // camera
    glm::vec3 cam_pos     = { camera_x, camera_y - 0.f, camera_z - 120.f };
    glm::mat4 translation = glm::translate(glm::mat4(1.f), cam_pos);
    glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), .0f, glm::vec3(1, 0, 0));
    glm::mat4 view        = rotation * translation;
    // glm::mat4 projection  = glm::perspective(glm::radians(60.f), (float)vkr.window_extent.width / (float)vkr.window_extent.height, 0.1f, 200.0f);
    glm::mat4 projection = glm::ortho(0.f, (float)vkr.window_extent.width, 0.f, (float)vkr.window_extent.height, .1f, 200.f);
    // float aspect_ratio= (float)vkr.window_extent.width / (float)vkr.window_extent.height;
    // glm::mat4 projection = glm::ortho(0.f, aspect_ratio, -1.0f, 1.0f, .1f, 200.f);
    glm::mat4 V = glm::lookAt(glm::vec3(0.f, 0.f, 2.0f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f));
    projection[1][1] *= -1;

    vkr.camera.data.projection = projection * V;
    vkr.camera.data.view       = view;
    vkr.camera.data.viewproj   = projection * view;
    AllocateFillBuffer(&vkr.camera, sizeof(Camera::Data), vkr.camera.UBO[vkr.frame_idx_inflight % FRAME_OVERLAP].allocation); // todo(ad): just map the data ptr once

    // Bindings
    vkCmdBindPipeline(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline);

    const uint32_t dynamic_offsets = 0;
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 0, 1, &vkr.frames[vkr.frame_idx_inflight].set_global, 0, NULL);
    vkCmdBindDescriptorSets(*cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline_layout, 1, 1, &vkr.set_array_of_textures, 0, NULL);

    {
        // push constants
        // make a model view matrix for rendering the object
        // model matrix is specific to the "thing" we want to draw
        // so each model or triangle has its matrix so that it can be positioned on screen with respect to the camera position

        VkBuffer     buffers[] = { quads_bo.buffer };
        VkDeviceSize offsets[] = { 0, /*offsetof(Quad, data.indices) */ };
        vkCmdBindVertexBuffers(*cmd_buffer, 0, 1, buffers, offsets);
        vkCmdBindVertexBuffers(*cmd_buffer, 1, 1, &instances.bo.buffer, offsets);


        instances.data[0].pos.x += pos_x;
        instances.data[0].pos.y += pos_y;
        // instances.data[0].rot.z += pos_x;
        instances.data[0].rot.x += (float)(pos_z * PI / 180);
        // instances.data[0].scale *= pos_z*.1f+1;


        memcpy(instances_data_ptr, instances.data.data(), instances.data.size() * sizeof(InstanceData));

        vkCmdDraw(*cmd_buffer, ARR_COUNT(quad.vertices), (uint32_t)quads.size(), 0, 0);
        // vkCmdDrawIndexed(*cmd_buffer, 6, 2, 0, 0, 0);
    }
}
