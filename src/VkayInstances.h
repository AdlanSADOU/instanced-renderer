/*
    This file contains the implementation of IntanceBucket
    An InstanceBucket is a data structure that holds data about instances of a same base mesh
    which allows you to draw them all with only one draw call very efficiently
    using instanced rendering.

    i.e: you could add many sprites with different Texture(s) and Transform(s)
    and draw them all at once.

    This is particularly useful for something like a particle system.
*/

#include "VkayRenderer.h"

struct InstanceBucket
{
    void                     *mapped_data_ptr        = {};
    std::vector<InstanceData> instance_data_array    = {};
    VkayBuffer              instance_buffer_object = {};

    VkayBuffer quad_buffer_object = {};
};

void VkayInstanceBucketSetBaseMesh()
{
}

bool VkayInstancesBucketUpload(VkayRenderer *vkr, VmaAllocator allocator, InstanceBucket *bucket, Mesh base_mesh)
{
    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VK_CHECK(VkayCreateBuffer(&bucket->quad_buffer_object, allocator, sizeof(Vertex) * base_mesh.vertices.size(), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
        VK_CHECK(VkayMapMemcpyMemory(base_mesh.vertices.data(), sizeof(Vertex) * base_mesh.vertices.size(), allocator, bucket->quad_buffer_object.allocation));
    }

    {
        VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VK_CHECK(VkayCreateBuffer(&bucket->instance_buffer_object, allocator, bucket->instance_data_array.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
        VK_CHECK(vmaMapMemory(allocator, bucket->instance_buffer_object.allocation, &bucket->mapped_data_ptr));
        memcpy(bucket->mapped_data_ptr, bucket->instance_data_array.data(), bucket->instance_data_array.size() * sizeof(InstanceData));

        assert(bucket->mapped_data_ptr != NULL && "mapped_data_ptr is NULL");
    }

    VkDescriptorBufferInfo storage_buffer_info = {};
    // storage_buffer_info.buffer                 = instance_data_storage_buffer.buffer;
    storage_buffer_info.buffer = bucket->instance_buffer_object.buffer;
    storage_buffer_info.offset = 0;
    storage_buffer_info.range  = bucket->instance_data_array.size() * sizeof(InstanceData);

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

void VkayInstancesDestroyInstance(VkayRenderer *vkr, VmaAllocator allocator, uint32_t instance_index, InstanceBucket *bucket)
{
    if (instance_index >= bucket->instance_data_array.size()) {
        SDL_Log("InstanceData : element %d does not exist\n", instance_index);
        return;
    }

    bucket->instance_data_array.erase(bucket->instance_data_array.begin() + instance_index);
    vmaResizeAllocation(allocator, bucket->instance_buffer_object.allocation, bucket->instance_data_array.size() * sizeof(InstanceData));
    memcpy(bucket->mapped_data_ptr, bucket->instance_data_array.data(), bucket->instance_data_array.size() * sizeof(InstanceData));
}

void VkayInstancesDraw(VkCommandBuffer cmd_buffer, VkayRenderer *vkr, InstanceBucket *bucket, Mesh base_mesh)
{
    // todo(ad): the issue is that for each InstanceBucket drawn we are redundently re-binding these
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->default_pipeline_layout, 1, 1, &vkr->set_array_of_textures, 0, NULL);

    VkBuffer     buffers[] = { bucket->quad_buffer_object.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, buffers, offsets);
    vkCmdBindVertexBuffers(cmd_buffer, 1, 1, &bucket->instance_buffer_object.buffer, offsets);

    vkCmdDraw(cmd_buffer, (uint32_t)base_mesh.vertices.size(), (uint32_t)bucket->instance_data_array.size(), 0, 0);
}

void VkayInstancesDestroy(VkayRenderer *vkr, VmaAllocator allocator, InstanceBucket *bucket)
{
    vmaDestroyBuffer(allocator, bucket->quad_buffer_object.buffer, bucket->quad_buffer_object.allocation);
    vmaUnmapMemory(allocator, bucket->instance_buffer_object.allocation);
    vmaDestroyBuffer(allocator, bucket->instance_buffer_object.buffer, bucket->instance_buffer_object.allocation);
}