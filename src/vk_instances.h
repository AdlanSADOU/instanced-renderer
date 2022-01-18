#include "vk_renderer.h"
#include <array>

struct Instances
{
    void                     *m_instances_data_ptr;
    std::vector<InstanceData> m_data;
    BufferObject              m_bo;

    BufferObject      m_quads_bo = {};
    std::vector<Quad> m_quads;
    Quad              m_quad;

    bool Upload(VulkanRenderer *vkr)
    {
        m_quads.push_back(m_quad);

        {
            VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VK_CHECK(CreateBuffer(&m_quads_bo, vkr->allocator, &vkr->release_queue, m_quads.size() * sizeof(Quad::vertices), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
            VK_CHECK(MapMemcpyMemory(m_quads.data(), m_quads.size() * sizeof(Quad::vertices), vkr->allocator, m_quads_bo.allocation));
        }

        {
            VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            VK_CHECK(CreateBuffer(&m_bo, vkr->allocator, &vkr->release_queue, m_data.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
            VK_CHECK(vmaMapMemory(vkr->allocator, m_bo.allocation, &m_instances_data_ptr));
            memcpy(m_instances_data_ptr, m_data.data(), m_data.size() * sizeof(InstanceData));

            assert(m_instances_data_ptr != NULL && "m_instances_data_ptr is NULL");
        }

        VkDescriptorBufferInfo storage_buffer_info = {};
        // storage_buffer_info.buffer                 = instance_data_storage_buffer.buffer;
        storage_buffer_info.buffer = m_bo.buffer;
        storage_buffer_info.offset = 0;
        storage_buffer_info.range  = m_data.size() * sizeof(InstanceData);

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

    void DestroyInstance(VulkanRenderer *vkr, uint32_t instance_index)
    {
        // erase from container


        // std::vector<InstanceData>::iterator it = (std::find(m_data.begin(), m_data.end(), m_data.data()[instance_index]));
        if (instance_index >= m_data.size()) {
            SDL_Log("InstanceData : element %d does not exist\n", instance_index);
            return;
        }

        m_data.erase(m_data.begin() + instance_index);
        // resize container?
        vmaResizeAllocation(vkr->allocator, m_bo.allocation, m_data.size() * sizeof(InstanceData));
        memcpy(m_instances_data_ptr, m_data.data(), m_data.size() * sizeof(InstanceData));
        // resize allocation
        // memcpy
    }

    void Draw(VkCommandBuffer cmd_buffer, VulkanRenderer *vkr)
    {
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline);
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline_layout, 1, 1, &vkr->set_array_of_textures, 0, NULL);

        VkBuffer     buffers[] = { m_quads_bo.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, buffers, offsets);
        vkCmdBindVertexBuffers(cmd_buffer, 1, 1, &m_bo.buffer, offsets);

        vkCmdDraw(cmd_buffer, ARR_COUNT(m_quad.vertices), (uint32_t)m_data.size(), 0, 0);
    }
};