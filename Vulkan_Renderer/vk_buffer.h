#pragma once
#include "includes.h"

struct Buffer {
	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory BufferDeviceMemory = VK_NULL_HANDLE;
	uint32_t size = 0;
	int getSize() { return size; }

private:
	VkDevice device = VK_NULL_HANDLE;

public:
	int create_Buffer(VkDevice device, VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties, VkDeviceSize m_size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags m_memoryPropertyFlags) {
		this->device = device;
		size = m_size;
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = NULL;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = m_size;
		bufferCreateInfo.usage = bufferUsageFlags; // indicate that till buffer will contain vertex data
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0; // only when concurrent sharing mode is specified - presentation and graphics is done on the same queue so this buffer does not need to be shared with another queue
		bufferCreateInfo.pQueueFamilyIndices = NULL; // only when concurrent sharing mode is specified

		VK_CHECK(vkCreateBuffer(device, &bufferCreateInfo, NULL, &handle));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device, handle, &memoryRequirements);

		//  we must find a memoryType that support our memoryRequirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : our additional requirement is that memory needs to be host visible
		for (uint32_t i = 0; i < m_deviceMemoryProperties.memoryTypeCount; ++i) {
			if ((memoryRequirements.memoryTypeBits & (1 << i)) && // We want a SysRAM type of heap, whose index is 1
				(m_deviceMemoryProperties.memoryTypes[i].propertyFlags & m_memoryPropertyFlags)) // now within that heap, of index 1, we a type of memory that is HOST_VISIBLE
			{
				VkMemoryAllocateInfo memoryAllocateInfo = {
				  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	  // VkStructureType                        sType
				  NULL,										  // const void                            *pNext
				  memoryRequirements.size,					  // VkDeviceSize                           allocationSize
				  i                                           // uint32_t                               memoryTypeIndex
				};
				VK_CHECK(vkAllocateMemory(device, &memoryAllocateInfo, NULL, &BufferDeviceMemory));
			}
		}

		VK_CHECK(vkBindBufferMemory(device, handle, BufferDeviceMemory, 0));

		return 1;
	}

	void mapCopyData(VertexData(vertex_Data)[], uint32_t array_size) {
		void *BufferMemoryPtr;

		VK_CHECK(vkMapMemory(this->device, this->BufferDeviceMemory, 0, (array_size), 0, &BufferMemoryPtr));
		memcpy(BufferMemoryPtr, vertex_Data, (array_size));

		VkMappedMemoryRange flushRange{};
		flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		flushRange.pNext = NULL;
		flushRange.memory = BufferDeviceMemory;
		flushRange.offset = 0;
		flushRange.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(device, 1, &flushRange);

		vkUnmapMemory(device, BufferDeviceMemory);
	}

	VkDeviceSize offset = 0;
	void draw(VkCommandBuffer commandBuffer) {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &handle, &offset);
		vkCmdDraw(commandBuffer, size, 1, 0, 0);
	}
};

