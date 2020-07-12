#pragma once
#include "includes.h"
#include "vk_context.h"

struct Texture {
	VkImage			handle;
	VkSampler		sampler;
	VkImageLayout	layout;
	VkImageView		view;
	VkDeviceMemory	imageDeviceMemory;
	uint32_t		width, height;
	uint32_t		mipLevels;
	VkDescriptorImageInfo imageInfo{};

private:
	unsigned char *imageData = NULL;
	uint32_t imageSize = 0;
	int channels = 0;
	int x = 0, y = 0;
	
public:
	int load_ImageData(char *m_filepath);
	VkResult create_Image(VkDevice m_device);
	VkResult allocate_ImageMemory(VkDevice m_device, VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties, VkMemoryPropertyFlags m_memoryPropertyFlags);
	VkResult create_ImageView(VkDevice m_device);
	VkResult create_Sampler(VkDevice m_device);
	VkResult copy_TextureData(VkDevice m_device, VkCommandBuffer commandBuffer, VkPhysicalDeviceMemoryProperties memoryProperties);
};

////////////////////////////////////////////////
// <Texture> inline functions implementation

inline int Texture::load_ImageData(char *filepath)
{
	this->imageData = stbi_load(filepath, &this->x, &this->y, &this->channels, STBI_rgb_alpha);
	if (x == 0 || y == 0)
		printf("image loading: x or y == 0...\n");
	this->imageSize = this->x * this->y;
	return 1;
}

inline VkResult Texture::create_Image(VkDevice m_device)
{
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent.width = this->x;
	imageCreateInfo.extent.height = this->y;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	return vkCreateImage(m_device, &imageCreateInfo, NULL, &this->handle);
}

inline VkResult Texture::allocate_ImageMemory(VkDevice m_device, VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties, VkMemoryPropertyFlags m_memoryPropertyFlags) {
	VkMemoryRequirements memoryRequirements{};
	vkGetImageMemoryRequirements(m_device, this->handle, &memoryRequirements);

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
			VK_CHECK(vkAllocateMemory(m_device, &memoryAllocateInfo, NULL, &this->imageDeviceMemory));
		}
	}
	return vkBindImageMemory(m_device, this->handle, this->imageDeviceMemory, 0);
}

inline VkResult Texture::create_ImageView(VkDevice m_device)
{

	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.pNext = NULL;
	viewCreateInfo.flags = 0;
	viewCreateInfo.image = this->handle;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewCreateInfo.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY, };
	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;

	return vkCreateImageView(m_device, &viewCreateInfo, NULL, &this->view);
}

inline VkResult Texture::create_Sampler(VkDevice m_device)
{
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = NULL;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR; // Type of filtering (nearest or linear) used for magnification
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST; // Type of filtering (nearest or linear) used for minification
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; // Type of filtering (nearest or linear) used for mipmap lookup
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // Addressing mode for U coordinates that are outside of a < 0.0; 1.0 > range
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // Addressing mode for V coordinates that are outside of a <0.0; 1.0> range
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // Addressing mode for W coordinates that are outside of a <0.0; 1.0> range
	samplerCreateInfo.mipLodBias = 0.0f; // Value of bias added to mipmap’s level of detail calculations.If we want to offset fetching data from a specific mipmap, we can provide a value other than 0.0
	samplerCreateInfo.anisotropyEnable = VK_FALSE; // Parameter defining whether anisotropic filtering should be used
	samplerCreateInfo.maxAnisotropy = 1.f; // Maximal allowed value used for anisotropic filtering (clamping value)
	samplerCreateInfo.compareEnable = VK_FALSE; // Enables comparison against a reference value during texture lookups
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS; // Type of comparison performed during lookups if the compareEnable parameter is set to true
	samplerCreateInfo.minLod = 0.f; // Minimal allowed level of detail used during data fetching. If calculated level of detail (mipmap level) is lower than this value, it will be clamped
	samplerCreateInfo.maxLod = 0.f; // Maximal allowed level of detail used during data fetching. If the calculated level of detail (mipmap level) is greater than this value, it will be clamped
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK; // Specifies predefined color of border pixels. Border color is used when address mode includes clamping to border colors
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE; /* Usually (when this parameter is set to false) we provide texture coordinates using a normalized <0.0; 1.0> range.
															 When set to true, this parameter allows us to specify that we want to use unnormalized coordinates and address texture using texels
															 (in a <0; texture dimension> range, similar to OpenGL’s rectangle textures) */

	VkResult result = vkCreateSampler(m_device, &samplerCreateInfo, NULL, &this->sampler);

	imageInfo.sampler = this->sampler;
	imageInfo.imageView = this->view;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return result;
}

inline VkResult Texture::copy_TextureData(VkDevice m_device, VkCommandBuffer commandBuffer, VkPhysicalDeviceMemoryProperties memoryProperties)
{
	VkImageSubresourceRange subRange{};
	subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subRange.baseMipLevel = 0;
	subRange.levelCount = 1;
	subRange.baseArrayLayer = 0;
	subRange.layerCount = 1;

	/*
		Before we are able to copy data into our image memory, we need to tell the pipeline.
		we transition the image layout from UNDEFINED to TRANSFER_DST_OPTIMAL
	*/
	VkImageMemoryBarrier imageBarrier_undefined_to_tranfer_dst{};
	imageBarrier_undefined_to_tranfer_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier_undefined_to_tranfer_dst.pNext = NULL;
	imageBarrier_undefined_to_tranfer_dst.srcAccessMask = 0;
	imageBarrier_undefined_to_tranfer_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageBarrier_undefined_to_tranfer_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier_undefined_to_tranfer_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_undefined_to_tranfer_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier_undefined_to_tranfer_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier_undefined_to_tranfer_dst.image = this->handle;
	imageBarrier_undefined_to_tranfer_dst.subresourceRange = subRange;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_undefined_to_tranfer_dst);


	VkImageSubresourceLayers imageSubresource{};
	imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresource.mipLevel = 0;
	imageSubresource.baseArrayLayer = 0;
	imageSubresource.layerCount = 1;

	Buffer stagingBuffer;
	stagingBuffer.create_Buffer(m_device, memoryProperties, this->imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	void* BufferMemoryPtr = nullptr;

	(vkMapMemory(m_device, stagingBuffer.BufferDeviceMemory, 0, (this->imageSize), 0, &BufferMemoryPtr));
	memcpy(BufferMemoryPtr, this->imageData, (this->imageSize));

	VkMappedMemoryRange flushRange{};
	flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	flushRange.pNext = NULL;
	flushRange.memory = stagingBuffer.BufferDeviceMemory;
	flushRange.offset = 0;
	flushRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(m_device, 1, &flushRange);

	vkUnmapMemory(m_device, stagingBuffer.BufferDeviceMemory);

	VkBufferImageCopy bufferImageCopyInfo{};
	bufferImageCopyInfo.bufferOffset = 0;
	bufferImageCopyInfo.bufferRowLength = 0;
	bufferImageCopyInfo.bufferImageHeight;
	bufferImageCopyInfo.imageSubresource = imageSubresource;
	bufferImageCopyInfo.imageOffset = {0, 0, 0};
	bufferImageCopyInfo.imageExtent.width = WIDTH;
	bufferImageCopyInfo.imageExtent.height = HEIGHT;
	bufferImageCopyInfo.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.handle, this->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopyInfo);


	/*
		Before we are able to render the texture, we need to tell the pipeline.
		we transition the image layout from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	*/
	VkImageMemoryBarrier imageBarrier_transfer_to_shader_read{};
	imageBarrier_transfer_to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier_transfer_to_shader_read.pNext = NULL;
	imageBarrier_transfer_to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageBarrier_transfer_to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageBarrier_transfer_to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_transfer_to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageBarrier_transfer_to_shader_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier_transfer_to_shader_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier_transfer_to_shader_read.image = this->handle;
	imageBarrier_transfer_to_shader_read.subresourceRange = subRange;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_transfer_to_shader_read);
	return VkResult();
}
