#if !defined(VK_TEXTURE_H)
#define VK_TEXTURE_H

#include "VkayRenderer.h"
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vk_mem_alloc.h>

struct Texture
{
    VkImage        image;
    VkDeviceMemory memory;
    VmaAllocation  allocation;
    VkImageView    view;
    VkFormat       format;

    uint32_t width;
    uint32_t height;
    uint32_t num_channels;
    uint32_t id;
};

void VkayTextureCreate(const char *filepath, VkayRenderer *vkr, Texture *texture)
{
    texture->id = vkr->texture_array_index;

    int tex_width, tex_height, texChannels;

    // STBI_rgb_alpha forces alpha
    stbi_uc *pixels = stbi_load(filepath, &tex_width, &tex_height, &texChannels, STBI_rgb_alpha);
    if (!pixels)
        SDL_Log("failed to load %s", filepath);

    size_t imageSize = tex_width * tex_height * 4;


    texture->width        = tex_width;
    texture->height       = tex_height;
    texture->num_channels = texChannels;
    texture->format       = VK_FORMAT_R8G8B8A8_UNORM;


    VkImageCreateInfo ci_image = {};
    ci_image.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci_image.imageType         = VK_IMAGE_TYPE_2D;
    ci_image.format            = VK_FORMAT_R8G8B8A8_UNORM;
    ci_image.extent            = { texture->width, texture->height, 1 };
    ci_image.mipLevels         = 1;
    ci_image.arrayLayers       = 1;
    ci_image.samples           = VK_SAMPLE_COUNT_1_BIT;
    ci_image.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci_image.tiling            = VK_IMAGE_TILING_OPTIMAL; // 0
    ci_image.sharingMode       = VK_SHARING_MODE_EXCLUSIVE; // 0
    ci_image.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED; // 0

    VmaAllocationCreateInfo ci_allocation = {};
    ci_allocation.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(vkr->allocator, &ci_image, &ci_allocation, &texture->image, &texture->allocation, NULL));


    VkImageViewCreateInfo ci_image_view = {};
    ci_image_view.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci_image_view.image                 = texture->image;
    ci_image_view.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    ci_image_view.format                = VK_FORMAT_R8G8B8A8_UNORM;
    ci_image_view.components            = {}; // VK_COMPONENT_SWIZZLE_IDENTITY = 0
    ci_image_view.subresourceRange      = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VK_CHECK(vkCreateImageView(vkr->device, &ci_image_view, NULL, &texture->view));


    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = NULL;
    bufferInfo.size               = imageSize;
    bufferInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BufferObject staging_buffer;
    VK_CHECK(vmaCreateBuffer(vkr->allocator, &bufferInfo, &vmaallocInfo, &staging_buffer.buffer, &staging_buffer.allocation, NULL));

    void *staging_data;
    vmaMapMemory(vkr->allocator, staging_buffer.allocation, &staging_data);
    memcpy(staging_data, pixels, imageSize);
    vmaFlushAllocation(vkr->allocator, staging_buffer.allocation, 0, VK_WHOLE_SIZE);
    vmaUnmapMemory(vkr->allocator, staging_buffer.allocation);

    stbi_image_free(pixels);

    // begin staging command buffer
    VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
    cmd_buffer_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buffer_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(vkr->frames[0].cmd_buffer_gfx, &cmd_buffer_begin_info);

    // image layout transitioning
    VkImageSubresourceRange image_subresource_range;
    image_subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel   = 0;
    image_subresource_range.levelCount     = 1;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount     = 1;

    VkImageMemoryBarrier image_memory_barrier_from_undefined_to_transfer_dst = {};
    image_memory_barrier_from_undefined_to_transfer_dst.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier_from_undefined_to_transfer_dst.srcAccessMask        = 0;
    image_memory_barrier_from_undefined_to_transfer_dst.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier_from_undefined_to_transfer_dst.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier_from_undefined_to_transfer_dst.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier_from_undefined_to_transfer_dst.image                = texture->image;
    image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange     = image_subresource_range;
    image_memory_barrier_from_undefined_to_transfer_dst.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier_from_undefined_to_transfer_dst.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    {
        VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        vkCmdPipelineBarrier(vkr->frames[0].cmd_buffer_gfx, src_stage_flags, dst_stage_flags, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier_from_undefined_to_transfer_dst);
    }

    VkBufferImageCopy buffer_image_copy;
    buffer_image_copy.bufferOffset      = 0;
    buffer_image_copy.bufferRowLength   = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    buffer_image_copy.imageOffset       = { 0, 0, 0 }; // x, y, z
    buffer_image_copy.imageExtent       = { (uint32_t)tex_width, (uint32_t)tex_height, 1 };
    vkCmdCopyBufferToImage(vkr->frames[0].cmd_buffer_gfx, staging_buffer.buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);

    // Image layout transfer to SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier image_memory_barrier_from_transfer_to_shader_read = {};
    image_memory_barrier_from_transfer_to_shader_read.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier_from_transfer_to_shader_read.srcAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier_from_transfer_to_shader_read.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
    image_memory_barrier_from_transfer_to_shader_read.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier_from_transfer_to_shader_read.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_memory_barrier_from_transfer_to_shader_read.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier_from_transfer_to_shader_read.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier_from_transfer_to_shader_read.image                = texture->image;
    image_memory_barrier_from_transfer_to_shader_read.subresourceRange     = image_subresource_range;

    VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    vkCmdPipelineBarrier(vkr->frames[0].cmd_buffer_gfx, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_transfer_to_shader_read);

    VK_CHECK(vkEndCommandBuffer(vkr->frames[0].cmd_buffer_gfx));


    // Submit command buffer and copy data from staging buffer to a vertex buffer
    VkSubmitInfo submit_info       = {};
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &vkr->frames[0].cmd_buffer_gfx;

    VK_CHECK(vkQueueSubmit(vkr->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    vkDeviceWaitIdle(vkr->device);

    vmaDestroyBuffer(vkr->allocator, staging_buffer.buffer, staging_buffer.allocation);


    VkDescriptorImageInfo desc_image_image_info           = {};
    desc_image_image_info.sampler                         = NULL;
    desc_image_image_info.imageLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc_image_image_info.imageView                       = texture->view;
    vkr->descriptor_image_infos[vkr->texture_array_index] = desc_image_image_info;


    // descriptorImageInfos requires VkImageView(s) which requires VkImage(s) backing actual texture data
    // we need to finish texture loading in vk_texture.h
    // we must be able to load textures whenever necessary
    if (vkr->texture_array_index == 0) {
        VkDescriptorSetAllocateInfo desc_alloc_info = {};
        desc_alloc_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        desc_alloc_info.descriptorPool              = vkr->descriptor_pool;
        desc_alloc_info.descriptorSetCount          = 1;
        desc_alloc_info.pSetLayouts                 = &vkr->set_layout_array_of_textures;

        VK_CHECK(vkAllocateDescriptorSets(vkr->device, &desc_alloc_info, &vkr->set_array_of_textures));
    }

    VkWriteDescriptorSet setWrites[2];

    VkDescriptorImageInfo samplerInfo = {};
    samplerInfo.sampler               = vkr->sampler;
    setWrites[0]                      = {};
    setWrites[0].dstSet               = vkr->set_array_of_textures;
    setWrites[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrites[0].dstBinding           = 0;
    setWrites[0].dstArrayElement      = 0;
    setWrites[0].descriptorCount      = 1;
    setWrites[0].descriptorType       = VK_DESCRIPTOR_TYPE_SAMPLER;
    setWrites[0].pImageInfo           = &samplerInfo;
    setWrites[0].pBufferInfo          = 0;

    setWrites[1]                 = {};
    setWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrites[1].dstSet          = vkr->set_array_of_textures;
    setWrites[1].dstBinding      = 1;
    setWrites[1].dstArrayElement = 0;
    setWrites[1].descriptorCount = (uint32_t)vkr->descriptor_image_infos.size();
    setWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    setWrites[1].pImageInfo      = vkr->descriptor_image_infos.data();
    setWrites[1].pBufferInfo     = 0;
    vkUpdateDescriptorSets(vkr->device, 2, setWrites, 0, NULL);

    vkr->texture_array_index++;
}

void VkayTextureDestroy(VkayRenderer *vkr, Texture *texture)
{
    vkDestroyImageView(vkr->device, texture->view, NULL);
    vmaDestroyImage(vkr->allocator, texture->image, texture->allocation);
}

#endif // VK_TEXTURE_H
