#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "VkayTypes.h"

VertexInputDescription GetVertexDescription()
{
    VertexInputDescription description;

    // we will have just 1 vertex buffer binding, with a per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding                         = 0;
    mainBinding.stride                          = sizeof(Vertex);
    mainBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    description.bindings.push_back(mainBinding);

    // Position will be stored at Location 0
    {
        VkVertexInputAttributeDescription position_attribute = {};
        position_attribute.binding                           = 0;
        position_attribute.location                          = 0;
        position_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        position_attribute.offset                            = offsetof(Vertex, position);

        // Normal will be stored at Location 1
        VkVertexInputAttributeDescription normal_attribute = {};
        normal_attribute.binding                           = 0;
        normal_attribute.location                          = 1;
        normal_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        normal_attribute.offset                            = offsetof(Vertex, normal);

        VkVertexInputAttributeDescription tex_uv_attribute = {};
        tex_uv_attribute.binding                           = 0;
        tex_uv_attribute.location                          = 2;
        tex_uv_attribute.format                            = VK_FORMAT_R32G32_SFLOAT;
        tex_uv_attribute.offset                            = offsetof(Vertex, tex_uv);

        // Color will be stored at Location 3
        VkVertexInputAttributeDescription color_attribute = {};
        color_attribute.binding                           = 0;
        color_attribute.location                          = 3;
        color_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        color_attribute.offset                            = offsetof(Vertex, color);


        description.attributes.push_back(position_attribute);
        description.attributes.push_back(normal_attribute);
        description.attributes.push_back(color_attribute);
        description.attributes.push_back(tex_uv_attribute);
    }

    {
        VkVertexInputAttributeDescription position_attribute = {};
        position_attribute.binding                           = 1;
        position_attribute.location                          = 4;
        position_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        position_attribute.offset                            = offsetof(InstanceData, pos);

        VkVertexInputAttributeDescription rotation_attribute = {};
        rotation_attribute.binding                           = 1;
        rotation_attribute.location                          = 5;
        rotation_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        rotation_attribute.offset                            = offsetof(InstanceData, rot);

        VkVertexInputAttributeDescription scale_attribute = {};
        scale_attribute.binding                           = 1;
        scale_attribute.location                          = 6;
        scale_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        scale_attribute.offset                            = offsetof(InstanceData, scale);

        VkVertexInputAttributeDescription tex_idx_attribute = {};
        tex_idx_attribute.binding                           = 1;
        tex_idx_attribute.location                          = 7;
        tex_idx_attribute.format                            = VK_FORMAT_R32_SINT;
        tex_idx_attribute.offset                            = offsetof(InstanceData, texure_idx);

        description.attributes.push_back(position_attribute);
        description.attributes.push_back(rotation_attribute);
        description.attributes.push_back(scale_attribute);
        description.attributes.push_back(tex_idx_attribute);
    }

    // InstanceData
    VkVertexInputBindingDescription instance_data_binding = {};
    instance_data_binding.binding                         = 1;
    instance_data_binding.stride                          = sizeof(InstanceData);
    instance_data_binding.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;
    description.bindings.push_back(instance_data_binding);

    return description;
}

void VkayAllocation()
{
    // vmaAllocateMemory
}

uint32_t FindProperties(const VkPhysicalDeviceMemoryProperties *pMemoryProperties, uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties)
{
    const uint32_t memoryCount = pMemoryProperties->memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) {
        const uint32_t memoryTypeBits       = (1 << memoryIndex);
        const bool     isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

        const VkMemoryPropertyFlags properties            = pMemoryProperties->memoryTypes[memoryIndex].propertyFlags;
        const bool                  hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
            return memoryIndex;
    }

    // failed to find memory type
    return -1;
}

bool CreateUniformBuffer(VkDevice device, VkDeviceSize size, VkBuffer *out_buffer)
{
    VkBufferCreateInfo ci_buffer = {};
    ci_buffer.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci_buffer.flags              = 0;
    ci_buffer.size               = size;
    ci_buffer.usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ci_buffer.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
    return vkCreateBuffer(device, &ci_buffer, NULL, out_buffer);
}

void GetDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers)
{

    // desc_set_layout_bindings.push_back(desc_set_layout_binding_storage_buffer);
}

VkResult CreateDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator, VkDescriptorSetLayout *set_layout, const VkDescriptorSetLayoutBinding *bindings, uint32_t binding_count)
{

    VkDescriptorSetLayoutCreateInfo create_info_desc_set_layout = {};
    create_info_desc_set_layout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info_desc_set_layout.pNext                           = NULL;

    create_info_desc_set_layout.flags        = 0;
    create_info_desc_set_layout.pBindings    = bindings;
    create_info_desc_set_layout.bindingCount = binding_count;

    return vkCreateDescriptorSetLayout(device, &create_info_desc_set_layout, NULL, set_layout);
}

VkResult AllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, uint32_t descriptor_set_count, const VkDescriptorSetLayout *set_layouts, VkDescriptorSet *descriptor_set)
{

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext                       = NULL;
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    allocInfo.descriptorPool     = descriptor_pool;
    allocInfo.descriptorSetCount = descriptor_set_count;
    allocInfo.pSetLayouts        = set_layouts;

    return vkAllocateDescriptorSets(device, &allocInfo, descriptor_set);
}

VkResult VkayCreateImage()
{
    return VK_SUCCESS;
}

VkResult VkayCreateBuffer(VkayBuffer *dst_buffer, VmaAllocator allocator, size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
    assert(alloc_size != 0 && "alloc_size is 0");

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = NULL;

    bufferInfo.size  = alloc_size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memory_usage;

    // bind buffer to allocation
    VkResult res = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &dst_buffer->buffer, &dst_buffer->allocation, NULL);

    return res;
}

VkResult VkayCreateBuffer(VkayBuffer *dst_buffer, VmaAllocator allocator, size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, short line, const char *filename)
{
    assert(alloc_size != 0 && "alloc_size is 0");

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = NULL;

    bufferInfo.size  = alloc_size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memory_usage;

    // bind buffer to allocation
    VkResult res = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &dst_buffer->buffer, &dst_buffer->allocation, NULL);

    SDL_Log("%s:%d: Created Buffer: handle[%ld] allocation[%ld]\n", filename, line, dst_buffer->buffer, dst_buffer->allocation);

    return res;
}

void VkayDestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void VkayDestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, short line, const char *filename)
{
    vmaDestroyBuffer(allocator, buffer, allocation);
    SDL_Log("%s:%d: Destroyed Buffer: handle[%ld] allocation[%ld]\n", filename, line, buffer, allocation);
}

bool VkayAllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory)
{

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryRequirements  buffer_memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &buffer_memory_requirements);

    VkPhysicalDeviceMemoryProperties gpu_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_memory_properties);

    uint32_t memory_type = FindProperties(&gpu_memory_properties, buffer_memory_requirements.memoryTypeBits, flags);

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // VkStructureType                        sType
        nullptr, // const void                            *pNext
        buffer_memory_requirements.size, // VkDeviceSize                           allocationSize
        memory_type // uint32_t                               memoryTypeIndex
    };

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, memory) == VK_SUCCESS)
        return true;
    return false;
}

bool VkayAllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory, short line, const char *filename)
{

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryRequirements  buffer_memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &buffer_memory_requirements);

    VkPhysicalDeviceMemoryProperties gpu_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_memory_properties);

    uint32_t memory_type = FindProperties(&gpu_memory_properties, buffer_memory_requirements.memoryTypeBits, flags);

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // VkStructureType                        sType
        nullptr, // const void                            *pNext
        buffer_memory_requirements.size, // VkDeviceSize                           allocationSize
        memory_type // uint32_t                               memoryTypeIndex
    };

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, memory) == VK_SUCCESS) {
        SDL_Log("%s:%d: Allocated Buffer memory: Buffer handle[%d] Memory handle[%d] size[%ld]\n", filename, line, buffer, memory, buffer_memory_requirements.size);
        return true;
    }
    return false;
}

bool VkayAllocateImageMemory(VmaAllocator allocator, VkImage image, VmaAllocation *allocation, VmaMemoryUsage usage)
{
    VmaAllocationCreateInfo vma_ci_allocation = {};
    vma_ci_allocation.usage                   = usage; // VMA_MEMORY_USAGE_GPU_ONLY


    if (vmaAllocateMemoryForImage(allocator, image, &vma_ci_allocation, allocation, NULL) == VK_SUCCESS)
        return true;
    return false;
}

bool VkayAllocateImageMemory(VmaAllocator allocator, VkImage image, VmaAllocation *allocation, VmaMemoryUsage usage, short line, const char *filename)
{
    VmaAllocationCreateInfo vma_ci_allocation = {};
    vma_ci_allocation.usage                   = usage; // VMA_MEMORY_USAGE_GPU_ONLY


    if (vmaAllocateMemoryForImage(allocator, image, &vma_ci_allocation, allocation, NULL) == VK_SUCCESS) {
        SDL_Log("%s:%d: Allocated Buffer memory: Buffer handle[%d] Allocation handle[%d] size[%ld]\n", filename, line, image, allocation);
        return true;
    }
    return false;
}


VkResult VkayMapMemcpyMemory(void *src, size_t size, VmaAllocator allocator, VmaAllocation allocation)
{

    void    *data;
    VkResult result = (vmaMapMemory(allocator, allocation, &data));
    memcpy(data, src, size);
    vmaUnmapMemory(allocator, allocation);

    return result;
}

VkResult VkayMapMemcpyMemory(void *src, size_t size, VmaAllocator allocator, VmaAllocation allocation, short line, const char *filename)
{
    void    *data;
    VkResult result = (vmaMapMemory(allocator, allocation, &data));
    memcpy(data, src, size);
    vmaUnmapMemory(allocator, allocation);

    SDL_Log("%s:%d: Mapped ptr\n", filename, line);
    return result;
}

void CopyBuffer(VkCommandBuffer cmd_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    VkBufferCopy regions = {};
    regions.srcOffset    = 0;
    regions.dstOffset    = 0;
    regions.size         = size;
    vkCmdCopyBuffer(cmd_buffer, src_buffer, dst_buffer, 1, &regions);
}