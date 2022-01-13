#if !defined(VK_INITIALIZERS)
#define VK_INITIALIZERS

#include <vk_types.h>

namespace vkinit {

    VkCommandPoolCreateInfo                CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo            CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VkPipelineShaderStageCreateInfo        PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
    VkPipelineVertexInputStateCreateInfo   VertexInputStateCreateInfo();
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo(VkPrimitiveTopology topology);

    // In here is where we enable or disable backface culling, and set line width or wireframe drawing.
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);

    // allows us to configure MSAA for this pipeline.
    // we are going to default it to 1 sample and MSAA disabled.
    // If we wanted to enable MSAA, we would need to set rasterizationSamples to more than 1,
    // and enable sampleShading. Keep in mind that for MSAA to work,
    // our renderpass also has to support it, which complicates things significantly.
    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
    VkPipelineColorBlendAttachmentState  color_blend_attachment_state();
    VkPipelineLayoutCreateInfo           pipeline_layout_create_info();
    VkImageCreateInfo                    image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo                imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    // depthTestEnable holds if we should do any z-culling at all.
    // Set to VK_FALSE to draw on top of everything,
    // and VK_TRUE to not draw on top of other objects.
    // depthWriteEnable allows the depth to be written.
    // While DepthTest and DepthWrite will both be true most of the time,
    // there are cases where we might want to do depth write, but without doing depthtesting;
    // it’s sometimes used for some special effects.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
} // namespace vkinit
#endif // VK_INITIALIZERS
