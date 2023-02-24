#include "VkayBucket.h"
#include "Vkay.h"
#include "VkaySprite.h"
#include "VkayTexture.h"

namespace vkay {

    void BucketAddSpriteInstance(InstanceBucket *instanceBucket, Sprite *sprite)
    {
        InstanceData idata = {};
        idata.pos          = sprite->transform.position;
        idata.rot          = sprite->transform.rotation;
        idata.scale        = sprite->transform.scale;
        idata.texure_idx    = sprite->texture_idx;
        instanceBucket->instance_data_array.push_back(idata);
    }
    void InstanceBucketSetBaseMesh()
    {
    }

    bool BucketUpload(VkayRenderer *vkr, InstanceBucket *bucket, BaseMesh base_mesh)
    {
        {
            VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VKCHECK(VkayCreateBuffer(&bucket->mesh_buffer, vkr->allocator, sizeof(Vertex) * base_mesh.vertices.size(), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
            VKCHECK(VkayMapMemcpyMemory(base_mesh.vertices.data(), sizeof(Vertex) * base_mesh.vertices.size(), vkr->allocator, bucket->mesh_buffer.allocation));
        }

        {
            VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            VKCHECK(VkayCreateBuffer(&bucket->instance_buffer, vkr->allocator, bucket->instance_data_array.size() * sizeof(InstanceData), usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU));
            VKCHECK(vmaMapMemory(vkr->allocator, bucket->instance_buffer.allocation, &bucket->mapped_data_ptr));
            memcpy(bucket->mapped_data_ptr, bucket->instance_data_array.data(), bucket->instance_data_array.size() * sizeof(InstanceData));

            assert(bucket->mapped_data_ptr != NULL && "mapped_data_ptr is NULL");
        }

        VkDescriptorBufferInfo storage_buffer_info = {};
        // storage_buffer_info.buffer                 = instance_data_storage_buffer.buffer;
        storage_buffer_info.buffer = bucket->instance_buffer.buffer;
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

    void BucketDestroyInstance(VkayRenderer *vkr, uint32_t instance_index, InstanceBucket *bucket)
    {
        if (instance_index >= bucket->instance_data_array.size()) {
            SDL_Log("InstanceData : element %d does not exist\n", instance_index);
            return;
        }

        bucket->instance_data_array.erase(bucket->instance_data_array.begin() + instance_index);
        vmaResizeAllocation(vkr->allocator, bucket->instance_buffer.allocation, bucket->instance_data_array.size() * sizeof(InstanceData));
        memcpy(bucket->mapped_data_ptr, bucket->instance_data_array.data(), bucket->instance_data_array.size() * sizeof(InstanceData));
    }

    // todo(ad): we added BaseMesh here to basically be able to pass in any mesh to be instanced, not just Quads
    //
    void BucketDraw(VkCommandBuffer cmd_buffer, VkayRenderer *vkr, InstanceBucket *bucket, BaseMesh base_mesh)
    {
        // todo(ad): the issue is that for each InstanceBucket drawn we are redundently re-binding these
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->instanced_pipeline);
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr->instanced_pipeline_layout, 1, 1, &vkr->set_array_of_textures, 0, NULL);

        VkBuffer     buffers[] = { bucket->mesh_buffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, buffers, offsets);
        vkCmdBindVertexBuffers(cmd_buffer, 1, 1, &bucket->instance_buffer.buffer, offsets);

        vkCmdDraw(cmd_buffer, (uint32_t)base_mesh.vertices.size(), (uint32_t)bucket->instance_data_array.size(), 0, 0);
    }

    void BucketDestroy(VkayRenderer *vkr, InstanceBucket *bucket)
    {
        VkayDestroyBuffer(vkr->allocator, bucket->mesh_buffer.buffer, bucket->mesh_buffer.allocation);
        vmaUnmapMemory(vkr->allocator, bucket->instance_buffer.allocation);
        VkayDestroyBuffer(vkr->allocator, bucket->instance_buffer.buffer, bucket->instance_buffer.allocation);
    }
}
