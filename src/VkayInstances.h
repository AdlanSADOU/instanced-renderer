#include "VkayRenderer.h"
#include <array>

struct Instances
{
    void                     *m_instances_data_ptr;
    std::vector<InstanceData> m_data;
    BufferObject              m_bo;

    BufferObject      m_quads_bo = {};
    Quad              m_quad;
};

bool VkayInstancesUpload(VkayRenderer *vkr, Instances *instances)
{
    // instances->m_quads.push_back(instances->m_quad);

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VK_CHECK(CreateBuffer(&instances->m_quads_bo, vkr->allocator, sizeof(Quad), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
        VK_CHECK(MapMemcpyMemory(&instances->m_quad, sizeof(Quad), vkr->allocator, instances->m_quads_bo.allocation));
    }

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VK_CHECK(CreateBuffer(&instances->m_bo, vkr->allocator, instances->m_data.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
        VK_CHECK(vmaMapMemory(vkr->allocator, instances->m_bo.allocation, &instances->m_instances_data_ptr));
        memcpy(instances->m_instances_data_ptr, instances->m_data.data(), instances->m_data.size() * sizeof(InstanceData));

        assert(instances->m_instances_data_ptr != NULL && "m_instances_data_ptr is NULL");
    }

    VkDescriptorBufferInfo storage_buffer_info = {};
    // storage_buffer_info.buffer                 = instance_data_storage_buffer.buffer;
    storage_buffer_info.buffer = instances->m_bo.buffer;
    storage_buffer_info.offset = 0;
    storage_buffer_info.range  = instances->m_data.size() * sizeof(InstanceData);

    VkWriteDescriptorSet inst_data_descriptor_write = {};
    inst_data_descriptor_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    inst_data_descriptor_write.pNext;
    inst_data_descriptor_write.dstSet     = vkr->compute_descriptor_sets[0]; // set containing binding for InstanceData Storage Buffer
    inst_data_descriptor_write.dstBinding = 0; // Storage Buffer binding
    inst_data_descriptor_write.dstArrayElement;
    inst_data_descriptor_write.descriptorCount = 1;
    inst_data_descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inst_data_descriptor_write.pImageInfo;
    inst_data_descriptor_write.pBufferInfo = &storage_buffer_info;
    inst_data_descriptor_write.pTexelBufferView;
    vkUpdateDescriptorSets(vkr->device, 1, &inst_data_descriptor_write, 0, NULL);
    return true;
}

void VkayInstancesDestroy(VkayRenderer *vkr, uint32_t instance_index, Instances *instances)
{
    if (instance_index >= instances->m_data.size()) {
        SDL_Log("InstanceData : element %d does not exist\n", instance_index);
        return;
    }

    instances->m_data.erase(instances->m_data.begin() + instance_index);
    vmaResizeAllocation(vkr->allocator, instances->m_bo.allocation, instances->m_data.size() * sizeof(InstanceData));
    memcpy(instances->m_instances_data_ptr, instances->m_data.data(), instances->m_data.size() * sizeof(InstanceData));
}

void VkayInstancesDraw(VkCommandBuffer cmd_buffer, VkayRenderer *vkr, Instances *instances)
{
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline_layout, 1, 1, &vkr->set_array_of_textures, 0, NULL);

    VkBuffer     buffers[] = { instances->m_quads_bo.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, buffers, offsets);
    vkCmdBindVertexBuffers(cmd_buffer, 1, 1, &instances->m_bo.buffer, offsets);

    vkCmdDraw(cmd_buffer, ARR_COUNT(instances->m_quad.vertices), (uint32_t)instances->m_data.size(), 0, 0);
}