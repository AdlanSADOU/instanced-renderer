#include <vulkan/vulkan.h>
#include "vk_types.h"

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

bool AllocateBufferMemory(VkDevice device, VkPhysicalDevice gpu, VkBuffer buffer, VkDeviceMemory *memory)
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

bool CreateUniformBuffer(VkDevice device, VkDeviceSize size, VkBuffer *out_buffer)
{
    VkBufferCreateInfo ci_buffer = {};
    ci_buffer.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci_buffer.flags       = 0;
    ci_buffer.size        = size;
    ci_buffer.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ci_buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return vkCreateBuffer(device, &ci_buffer, NULL, out_buffer);
}

bool AllocateImageMemory(VkDevice device, VkPhysicalDevice gpu, VkImage image, VkDeviceMemory *memory)
{

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /* | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT */;
    VkMemoryRequirements  image_memory_requirements;
    vkGetImageMemoryRequirements(device, image, &image_memory_requirements);

    VkPhysicalDeviceMemoryProperties gpu_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_memory_properties);

    uint32_t memory_type = FindProperties(&gpu_memory_properties, image_memory_requirements.memoryTypeBits, flags);

    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize       = image_memory_requirements.size;
    memory_allocate_info.memoryTypeIndex      = memory_type;

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, memory) == VK_SUCCESS)
        return true;
    return false;
}

bool AllocateImageMemory(VmaAllocator allocator, VkImage image, VmaAllocation *allocation, VmaMemoryUsage usage)
{
    VmaAllocationCreateInfo vma_ci_allocation = {};
    vma_ci_allocation.usage                   = usage; // VMA_MEMORY_USAGE_GPU_ONLY


    if (vmaAllocateMemoryForImage(allocator, image, &vma_ci_allocation, allocation, NULL) == VK_SUCCESS)
        return true;
    return false;
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