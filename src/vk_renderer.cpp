
#include "vk_renderer.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <vk_initializers.h>
#include <vk_types.h>
#include <fstream>

VulkanRenderer vkr;
extern float   camera_x, camera_y, camera_z;

void vk_Init()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    vkr.window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        vkr.window_extent.width,
        vkr.window_extent.height,
        window_flags);





    VkValidationFeatureEnableEXT enabled_validation_feature[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT };
    VkValidationFeaturesEXT      validation_features          = {};
    validation_features.sType                                 = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validation_features.enabledValidationFeatureCount         = ARR_COUNT(enabled_validation_feature);
    validation_features.pEnabledValidationFeatures            = enabled_validation_feature;

    uint32_t supported_extension_properties_count;
    vkEnumerateInstanceExtensionProperties(NULL, &supported_extension_properties_count, NULL);
    std::vector<VkExtensionProperties> supported_extention_properties(supported_extension_properties_count);
    vkEnumerateInstanceExtensionProperties(NULL, &supported_extension_properties_count, supported_extention_properties.data());

    uint32_t required_extensions_count;
    SDL_Vulkan_GetInstanceExtensions(vkr.window, &required_extensions_count, NULL);
    // std::vector<char *> required_instance_extensions(required_extensions_count);
    const char **required_instance_extensions = new const char *[required_extensions_count];
    SDL_Vulkan_GetInstanceExtensions(vkr.window, &required_extensions_count, required_instance_extensions);

    const char *validation_layers[] = {
        { "VK_LAYER_KHRONOS_validation" },
    };

    VkApplicationInfo application_info  = {};
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext              = NULL;
    application_info.pApplicationName   = "name";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    application_info.pEngineName        = "engine name";
    application_info.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    application_info.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info_instance = {};
    create_info_instance.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if _DEBUG
    create_info_instance.pNext               = &validation_features;
    create_info_instance.enabledLayerCount   = ARR_COUNT(validation_layers);
    create_info_instance.ppEnabledLayerNames = validation_layers;
#endif
    create_info_instance.flags                   = 0;
    create_info_instance.pApplicationInfo        = &application_info;
    create_info_instance.enabledExtensionCount   = required_extensions_count;
    create_info_instance.ppEnabledExtensionNames = required_instance_extensions;
    VK_CHECK(vkCreateInstance(&create_info_instance, NULL, &vkr.instance));





    if (!SDL_Vulkan_CreateSurface(vkr.window, vkr.instance, &vkr.surface)) {
        printf("SDL Failed to create Surface");
    }




    ////////////////////////////////////
    /// GPU Selection
    uint32_t gpu_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vkr.instance, &gpu_count, NULL));

    // todo(ad): we're arbitrarily picking the first gpu we encounter right now
    // wich often is the one we want anyways. Must be improved to let user chose based features/extensions and whatnot
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    VK_CHECK(vkEnumeratePhysicalDevices(vkr.instance, &gpu_count, gpus.data()));
    vkr.chosen_gpu = gpus[0];





    //////////////////////////////////////////
    /// Query for graphics & present Queue/s
    // for each queue family idx, try to find one that supports presenting
    uint32_t queue_family_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkr.chosen_gpu, &queue_family_properties_count, NULL);
    VkQueueFamilyProperties *queue_family_properties = new VkQueueFamilyProperties[queue_family_properties_count];
    vkGetPhysicalDeviceQueueFamilyProperties(vkr.chosen_gpu, &queue_family_properties_count, queue_family_properties);

    if (queue_family_properties == NULL) {
        // some error message
        return;
    }

    VkBool32 *queue_idx_supports_present = new VkBool32[queue_family_properties_count];
    for (uint32_t i = 0; i < queue_family_properties_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(vkr.chosen_gpu, i, vkr.surface, &queue_idx_supports_present[i]);
    }

    uint32_t graphics_queue_family_idx = UINT32_MAX;
    uint32_t present_queue_family_idx  = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_properties_count; i++) {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            if (graphics_queue_family_idx == UINT32_MAX)
                graphics_queue_family_idx = i;
        if (queue_idx_supports_present[i] == VK_TRUE) {
            graphics_queue_family_idx = i;
            present_queue_family_idx  = i;
            break;
        }
    }

    if (present_queue_family_idx == UINT32_MAX)
        for (uint32_t i = 0; i < queue_family_properties_count; i++) {
            if (queue_idx_supports_present[i]) {
                present_queue_family_idx = i;
                break;
            }
        }

    if ((graphics_queue_family_idx == UINT32_MAX) && (present_queue_family_idx == UINT32_MAX)) {
        // todo(ad): exit on error message
        printf("Failed to find Graphics and Present queues");
    }

    vkr.graphics_queue_family     = graphics_queue_family_idx;
    vkr.present_queue_family      = present_queue_family_idx;
    vkr.is_present_queue_separate = (graphics_queue_family_idx != present_queue_family_idx);

    const float queue_priorities[] = {
        { 1.0 }
    };





    /////////////////////////////
    /// Device
    VkPhysicalDeviceFeatures supported_gpu_features = {};
    vkGetPhysicalDeviceFeatures(vkr.chosen_gpu, &supported_gpu_features);

    // todo(ad): not used right now
    uint32_t device_properties_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(vkr.chosen_gpu, NULL, &device_properties_count, NULL));
    VkExtensionProperties *device_extension_properties = new VkExtensionProperties[device_properties_count];
    VK_CHECK(vkEnumerateDeviceExtensionProperties(vkr.chosen_gpu, NULL, &device_properties_count, device_extension_properties));

    const char *enabled_device_extension_names[] = {
        "VK_KHR_swapchain",
    };

    VkPhysicalDeviceVulkan11Features gpu_vulkan_11_features = {};
    gpu_vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    gpu_vulkan_11_features.shaderDrawParameters = VK_TRUE;

    VkDeviceQueueCreateInfo create_info_device_queue = {};
    create_info_device_queue.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_info_device_queue.pNext                   = NULL;
    create_info_device_queue.flags                   = 0;
    create_info_device_queue.queueFamilyIndex        = graphics_queue_family_idx;
    create_info_device_queue.queueCount              = 1;
    create_info_device_queue.pQueuePriorities        = queue_priorities;

    VkDeviceCreateInfo create_info_device   = {};
    create_info_device.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info_device.pNext                = &gpu_vulkan_11_features;
    create_info_device.flags                = 0;
    create_info_device.queueCreateInfoCount = 1;
    create_info_device.pQueueCreateInfos    = &create_info_device_queue;
    // create_info_device.enabledLayerCount       = ; // deprecated and ignored
    // create_info_device.ppEnabledLayerNames     = ; // deprecated and ignored
    create_info_device.enabledExtensionCount   = ARR_COUNT(enabled_device_extension_names);
    create_info_device.ppEnabledExtensionNames = enabled_device_extension_names;
    create_info_device.pEnabledFeatures        = &supported_gpu_features;

    VK_CHECK(vkCreateDevice(vkr.chosen_gpu, &create_info_device, NULL, &vkr.device));

    if (!vkr.is_present_queue_separate) {
        vkGetDeviceQueue(vkr.device, graphics_queue_family_idx, 0, &vkr.graphics_queue_idx);
    } else {
        // todo(ad): get seperate present queue
    }





    ///////////////////////////
    /// VMA Allocator
    VmaAllocatorCreateInfo allocatorInfo = {};

    allocatorInfo.physicalDevice = vkr.chosen_gpu;
    allocatorInfo.device         = vkr.device;
    allocatorInfo.instance       = vkr.instance;
    vmaCreateAllocator(&allocatorInfo, &vkr.allocator);





    ////////////////////////////////////////////
    /// Swapchain
    uint32_t surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkr.chosen_gpu, vkr.surface, &surface_format_count, NULL);
    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkr.chosen_gpu, vkr.surface, &surface_format_count, surface_formats.data());

    // todo(ad): .presentMode arbitrarily set right now, we need to check what the OS supports and pick one
    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkr.chosen_gpu, vkr.surface, &present_modes_count, NULL);
    std::vector<VkPresentModeKHR> present_modes(present_modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkr.chosen_gpu, vkr.surface, &present_modes_count, present_modes.data());

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkr.chosen_gpu, vkr.surface, &surface_capabilities);

    VkSwapchainCreateInfoKHR create_info_swapchain = {};
    create_info_swapchain.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info_swapchain.pNext                    = NULL;
    create_info_swapchain.flags                    = 0;
    create_info_swapchain.surface                  = vkr.surface;
    create_info_swapchain.minImageCount            = 2;
    create_info_swapchain.imageFormat              = surface_formats[0].format;
    create_info_swapchain.imageColorSpace          = surface_formats[0].colorSpace;
    create_info_swapchain.imageExtent              = surface_capabilities.currentExtent;
    create_info_swapchain.imageArrayLayers         = 1;
    create_info_swapchain.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info_swapchain.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    create_info_swapchain.queueFamilyIndexCount    = 0;
    create_info_swapchain.pQueueFamilyIndices      = NULL;
    create_info_swapchain.preTransform             = surface_capabilities.currentTransform;
    create_info_swapchain.compositeAlpha           = (VkCompositeAlphaFlagBitsKHR)surface_capabilities.supportedCompositeAlpha; // todo(ad): this might only work if there's only one flag
    // create_info_swapchain.presentMode              = VK_PRESENT_MODE_MAILBOX_KHR; // todo(ad): setting this arbitrarily, must check if supported and desired
    // create_info_swapchain.presentMode              = VK_PRESENT_MODE_FIFO_RELAXED_KHR; // todo(ad): setting this arbitrarily, must check if supported and desired
    create_info_swapchain.presentMode  = VK_PRESENT_MODE_IMMEDIATE_KHR; // todo(ad): setting this arbitrarily, must check if supported and desired
    create_info_swapchain.clipped      = VK_TRUE;
    create_info_swapchain.oldSwapchain = 0;

    vkCreateSwapchainKHR(vkr.device, &create_info_swapchain, NULL, &vkr.swapchain);

    vkr.release_queue.push_function([=]() {
        vkDestroySwapchainKHR(vkr.device, vkr.swapchain, NULL);
    });





    //////////////////////////////////////
    /// Swapchain Images acquisition
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(vkr.device, vkr.swapchain, &swapchain_image_count, NULL);
    vkr.swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(vkr.device, vkr.swapchain, &swapchain_image_count, vkr.swapchain_images.data());

    vkr.swapchain_image_views.resize(swapchain_image_count);

    for (size_t i = 0; i < vkr.swapchain_image_views.size(); i++) {
        VkImageViewCreateInfo image_view_create_info           = {};
        image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image                           = vkr.swapchain_images[i];
        image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                          = surface_formats[0].format;
        image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel   = 0;
        image_view_create_info.subresourceRange.levelCount     = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount     = 1;

        vkCreateImageView(vkr.device, &image_view_create_info, NULL, &vkr.swapchain_image_views[i]);
    }

    vkr.swapchain_image_format = surface_formats[0].format;




    ///////////////////////////////////
    /// Depth Image
    // depth image size will match the window
    VkExtent3D depthImageExtent = {
        vkr.window_extent.width,
        vkr.window_extent.height,
        1
    };

    vkr.depth_format = VK_FORMAT_D32_SFLOAT; // hardcoding the depth format to 32 bit float
    // the depth image will be an image with the format we selected and Depth Attachment usage flag
    VkImageCreateInfo create_info_depth_image = vkinit::image_create_info(vkr.depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    VmaAllocationCreateInfo create_info_depth_image_allocation = {}; // for the depth image, we want to allocate it from GPU local memory
    create_info_depth_image_allocation.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    create_info_depth_image_allocation.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(vkr.allocator, &create_info_depth_image, &create_info_depth_image_allocation, &vkr.depth_image._image, &vkr.depth_image.allocation, NULL);

    // build an image-view for the depth image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(vkr.depth_format, vkr.depth_image._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(vkr.device, &dview_info, NULL, &vkr.depth_image_view));

    vkr.release_queue.push_function([=]() {
        vkDestroyImageView(vkr.device, vkr.depth_image_view, NULL);
        vmaDestroyImage(vkr.allocator, vkr.depth_image._image, vkr.depth_image.allocation);
    });





    //////////////////////////////////////////////////////
    // CommandPools & CommandBuffers creation
    // create a command pool for commands submitted to the graphics queue.
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
        vkr.graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(vkr.device, &commandPoolInfo, NULL, &vkr.frames[i].command_pool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(vkr.frames[i].command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(vkr.device, &cmdAllocInfo, &vkr.frames[i].main_command_buffer));

        vkr.release_queue.push_function([=]() {
            vkDestroyCommandPool(vkr.device, vkr.frames[i].command_pool, NULL);
        });
    }





    /////////////////////////////////////////////////////
    // Default renderpass
    // Color attachment
    VkAttachmentDescription color_attachment = {};
    color_attachment.format                  = vkr.swapchain_image_format; // the attachment will have the format needed by the swapchain
    color_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT; // 1 sample, we won't be doing MSAA
    color_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR; // we Clear when this attachment is loaded
    color_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE; // we keep the attachment stored when the renderpass ends
    color_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about stencil
    color_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED; // we don't know or care about the starting layout of the attachment
    color_attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // after the renderpass ends, the image has to be on a layout ready for display

    /////
    // Depth attachment
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags                   = 0;
    depth_attachment.format                  = vkr.depth_format;
    depth_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /////
    // Subpass description
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment            = 0; // attachment number will index into the pAttachments array in the parent renderpass itself
    color_attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment            = 1;
    depth_attachment_ref.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // we are going to create 1 subpass, which is the minimum you can do
    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    /////
    // RenderPass creation
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};

    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2; // connect the color attachment to the info
    render_pass_info.pAttachments    = &attachments[0];
    render_pass_info.subpassCount    = 1; // connect the subpass to the info
    render_pass_info.pSubpasses      = &subpass;

    VK_CHECK(vkCreateRenderPass(vkr.device, &render_pass_info, NULL, &vkr.render_pass));

    vkr.release_queue.push_function([=]() {
        vkDestroyRenderPass(vkr.device, vkr.render_pass, NULL);
    });





    ///////////////////////////////////////////////
    // Framebuffer initialization
    //
    // create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext                   = NULL;
    fb_info.renderPass              = vkr.render_pass;
    fb_info.attachmentCount         = 1;
    fb_info.width                   = vkr.window_extent.width;
    fb_info.height                  = vkr.window_extent.height;
    fb_info.layers                  = 1;

    // grab how many images we have in the swapchain
    const size_t swapchain_imagecount = vkr.swapchain_images.size();
    vkr.framebuffers                  = std::vector<VkFramebuffer>(swapchain_imagecount);

    // create framebuffers for each of the swapchain image views
    for (size_t i = 0; i < swapchain_imagecount; i++) {
        VkImageView attachments[2] = {};

        attachments[0] = vkr.swapchain_image_views[i];
        attachments[1] = vkr.depth_image_view;

        fb_info.pAttachments    = attachments;
        fb_info.attachmentCount = 2;

        VK_CHECK(vkCreateFramebuffer(vkr.device, &fb_info, NULL, &vkr.framebuffers[i]));

        vkr.release_queue.push_function([=]() {
            vkDestroyFramebuffer(vkr.device, vkr.framebuffers[i], NULL);
            vkDestroyImageView(vkr.device, vkr.swapchain_image_views[i], NULL);
        });
    }





    ////////////////////////////////////////////////////////////////
    // Sync structures
    // Fence creation
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext             = NULL;
    fenceCreateInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT; // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)

    for (size_t i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(vkr.device, &fenceCreateInfo, NULL, &vkr.frames[i].render_fence));

        /////
        // Semaphores creation
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext                 = NULL;
        semaphoreCreateInfo.flags                 = 0;

        VK_CHECK(vkCreateSemaphore(vkr.device, &semaphoreCreateInfo, NULL, &vkr.frames[i].present_semaphore));
        VK_CHECK(vkCreateSemaphore(vkr.device, &semaphoreCreateInfo, NULL, &vkr.frames[i].render_semaphore));

        vkr.release_queue.push_function([=]() {
            vkDestroyFence(vkr.device, vkr.frames[i].render_fence, NULL);
            vkDestroySemaphore(vkr.device, vkr.frames[i].present_semaphore, NULL);
            vkDestroySemaphore(vkr.device, vkr.frames[i].render_semaphore, NULL);
        });
    }





    /////////////////////////////////////////////////////
    // Descriptors
    std::vector<VkDescriptorPoolSize> sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },

    };

    ////////////////////////////
    /// Set Pool
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = 0;
    pool_info.maxSets                    = 4; // max times we can vkAllocateDescriptorSets
    pool_info.poolSizeCount              = (uint32_t)sizes.size();
    pool_info.pPoolSizes                 = sizes.data();
    vkCreateDescriptorPool(vkr.device, &pool_info, NULL, &vkr.descriptor_pool);





    /////////////////////////
    /// Set 0
    VkDescriptorSetLayoutBinding desc_set_layout_binding_UBO = {};

    desc_set_layout_binding_UBO.binding         = 0;
    desc_set_layout_binding_UBO.descriptorCount = 1;
    desc_set_layout_binding_UBO.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_set_layout_binding_UBO.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    // Storage buffer binding for model matrices array
    // used by vkCmdDarIndirect

    std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings;
    desc_set_layout_bindings.push_back(desc_set_layout_binding_UBO);
    CreateDescriptorSetLayout(vkr.device, NULL, &vkr.global_set_layout, desc_set_layout_bindings.data(), (uint32_t)desc_set_layout_bindings.size());





    //////////////////////////
    /// Set 1
    VkDescriptorSetLayoutBinding desc_set_layout_binding_storage_buffer = {};

    desc_set_layout_binding_storage_buffer.binding         = 0;
    desc_set_layout_binding_storage_buffer.descriptorCount = 1;
    desc_set_layout_binding_storage_buffer.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    desc_set_layout_binding_storage_buffer.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> desc_set1_layout_bindings;
    desc_set1_layout_bindings.push_back(desc_set_layout_binding_storage_buffer);
    CreateDescriptorSetLayout(vkr.device, NULL, &vkr.model_set_layout, desc_set1_layout_bindings.data(), (uint32_t)desc_set1_layout_bindings.size());





    /////////////////////////////////////
    /// Allocate sets for each camera buffer, we have a camera buffer for each framebuffer
    for (int i = 0; i < FRAME_OVERLAP; i++) {

        //////////////////////////
        //// Set allocation
        // allocate one descriptor set for each frame

        AllocateDescriptorSets(vkr.device, vkr.descriptor_pool, 1, &vkr.global_set_layout, &vkr.frames[i].global_descriptor);
        AllocateDescriptorSets(vkr.device, vkr.descriptor_pool, 1, &vkr.model_set_layout, &vkr.frames[i].model_descriptor);


        CreateBuffer(&vkr.camera.UBO[i], sizeof(Camera::Data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        CreateBuffer(&vkr.triangle_SSBO[i], sizeof(glm::mat4) * 100000, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info_descriptor_camera_buffer;
        VkDescriptorBufferInfo info_descriptor_model_buffer;


        { // todo(ad): function
            info_descriptor_camera_buffer.buffer = vkr.camera.UBO[i].buffer;
            info_descriptor_camera_buffer.offset = 0;
            info_descriptor_camera_buffer.range  = sizeof(Camera::Data);

            VkWriteDescriptorSet write_camera_buffer = {};
            write_camera_buffer.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_camera_buffer.pNext                = NULL;
            write_camera_buffer.dstBinding           = 0; // we are going to write into binding number 0
            write_camera_buffer.dstSet               = vkr.frames[i].global_descriptor; // of the global descriptor
            write_camera_buffer.descriptorCount      = 1;
            write_camera_buffer.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // and the type is uniform buffer
            write_camera_buffer.pBufferInfo          = &info_descriptor_camera_buffer;
            vkUpdateDescriptorSets(vkr.device, 1, &write_camera_buffer, 0, NULL);





            info_descriptor_model_buffer.buffer = vkr.triangle_SSBO[i].buffer;
            info_descriptor_model_buffer.offset = 0;
            info_descriptor_model_buffer.range  = sizeof(glm::mat4) * 100000;

            VkWriteDescriptorSet write_model_buffer = {};
            write_model_buffer.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_model_buffer.pNext                = NULL;
            write_model_buffer.dstBinding           = 0; // we are going to write into binding number 0
            write_model_buffer.dstSet               = vkr.frames[i].model_descriptor; // of the global descriptor
            write_model_buffer.descriptorCount      = 1;
            write_model_buffer.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // and the type is uniform buffer
            write_model_buffer.pBufferInfo          = &info_descriptor_model_buffer;
            vkUpdateDescriptorSets(vkr.device, 1, &write_model_buffer, 0, NULL);
        }
    }





    //////////////////////////////////////////////////////
    // Pipeline

    //////
    // ShaderModule loading
    VkShaderModule vertexShader;
    if (!InitShaderModule("../shaders/triangleMesh.vert.spv", &vertexShader)) {
        printf("Error when building the vertex shader module. Did you compile the shaders?\n");

    } else {
        printf("fragment shader succesfully loaded\n");
    }

    VkShaderModule fragmentShader;
    if (!InitShaderModule("../shaders/coloredTriangle.frag.spv", &fragmentShader)) {
        printf("Error when building the fragment shader module. Did you compile the shaders?\n");
    } else {
        printf("vertex fragment shader succesfully loaded\n");
    }



    // this is where we provide the vertex shader with our matrices
    VkPushConstantRange push_constant;
    push_constant.offset     = 0;
    push_constant.size       = sizeof(MeshPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;




    ///////
    // PipelineLayout
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info(); // build the pipeline layout that controls the inputs/outputs of the shader
    pipeline_layout_info.pushConstantRangeCount     = 1;
    pipeline_layout_info.pPushConstantRanges        = &push_constant;


    VkDescriptorSetLayout set_layouts[] = {
        vkr.global_set_layout,
        vkr.model_set_layout
    };

    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts    = set_layouts;

    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(vkr.device, &pipeline_layout_info, NULL, &pipelineLayout));
    VK_CHECK(vkCreatePipelineLayout(vkr.device, &pipeline_layout_info, NULL, &vkr.default_pipeline_layout));

    /////
    // Pipeline creation
    PipelineBuilder pipelineBuilder;
    VkPipeline      pipeline;

    /////
    // Depth attachment
    // connect the pipeline builder vertex input info to the one we get from Vertex
    VertexInputDescription vertexDescription = GetVertexDescription(); // horseshit, make this a useful function with arguments

    pipelineBuilder.create_info_vertex_input_state                                 = vkinit::vertex_input_state_create_info();
    pipelineBuilder.create_info_vertex_input_state.pVertexAttributeDescriptions    = vertexDescription.attributes.data();
    pipelineBuilder.create_info_vertex_input_state.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

    pipelineBuilder.create_info_vertex_input_state.pVertexBindingDescriptions    = vertexDescription.bindings.data();
    pipelineBuilder.create_info_vertex_input_state.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();

    pipelineBuilder.create_info_shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    pipelineBuilder.create_info_shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));

    pipelineBuilder.create_info_input_assembly_state = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // build viewport and scissor from the swapchain extents
    pipelineBuilder.viewport.x        = 0.0f;
    pipelineBuilder.viewport.y        = 0.0f;
    pipelineBuilder.viewport.width    = (float)vkr.window_extent.width;
    pipelineBuilder.viewport.height   = (float)vkr.window_extent.height;
    pipelineBuilder.viewport.minDepth = 0.0f;
    pipelineBuilder.viewport.maxDepth = 1.0f;

    pipelineBuilder.scissor.offset = { 0, 0 };
    pipelineBuilder.scissor.extent = vkr.window_extent;

    pipelineBuilder.rasterization_state           = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL); // configure the rasterizer to draw filled triangles
    pipelineBuilder.create_info_multisample_state = vkinit::multisampling_state_create_info(); // we don't use multisampling, so just run the default one
    pipelineBuilder.attachment_state_color_blend  = vkinit::color_blend_attachment_state(); // a single blend attachment with no blending and writing to RGBA
    pipelineBuilder.pipeline_layout               = pipelineLayout; // use the triangle layout we created


    pipeline             = pipelineBuilder.BuildPipeline(vkr.device, vkr.render_pass);
    vkr.default_pipeline = pipelineBuilder.BuildPipeline(vkr.device, vkr.render_pass);




    // Right now, we are assigning a pipeline to a material. So we don't have any options
    // for setting shaders or whatever else we might wanna customize in the future
    // the @create_Material function should probably create it's pipeline in a customizable way
    create_Material(pipeline, pipelineLayout, "defaultmesh");




    vkDestroyShaderModule(vkr.device, vertexShader, NULL);
    vkDestroyShaderModule(vkr.device, fragmentShader, NULL);

    vkr.release_queue.push_function([=]() {
        vkDestroyPipeline(vkr.device, pipeline, NULL);
        vkDestroyPipelineLayout(vkr.device, pipelineLayout, NULL);
    });

    vkr.is_initialized = true;
}




/////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/// Rendering
VkFence         *g_inflight_render_fence;
VkSemaphore     *g_inflight_present_semaphore;
VkSemaphore     *g_inflight_render_semaphore;
VkCommandBuffer *g_inflight_main_command_buffer;
VkDescriptorSet *g_inflight_global_descriptor_set;
BufferObject    *buffer;

void VulkanUpdateAndRender(double dt)
{
    // wait until the GPU has finished rendering the last frame. Timeout of 1 second
    g_inflight_render_fence          = &vkr.frames[vkr.frame_idx_inflight % FRAME_OVERLAP].render_fence;
    g_inflight_present_semaphore     = &vkr.frames[vkr.frame_idx_inflight % FRAME_OVERLAP].present_semaphore;
    g_inflight_render_semaphore      = &vkr.frames[vkr.frame_idx_inflight % FRAME_OVERLAP].render_semaphore;
    g_inflight_main_command_buffer   = &vkr.frames[vkr.frame_idx_inflight % FRAME_OVERLAP].main_command_buffer;
    g_inflight_global_descriptor_set = &vkr.frames[vkr.frame_idx_inflight % FRAME_OVERLAP].global_descriptor;

    buffer = &vkr.camera.UBO[vkr.frame_idx_inflight % FRAME_OVERLAP];



    VK_CHECK(vkWaitForFences(vkr.device, 1, g_inflight_render_fence, true, SECONDS(1)));
    VK_CHECK(vkResetFences(vkr.device, 1, g_inflight_render_fence));

    uint32_t idx_swapchain_image;
    VK_CHECK(vkAcquireNextImageKHR(vkr.device, vkr.swapchain, SECONDS(1), *g_inflight_present_semaphore, NULL, &idx_swapchain_image));
    VK_CHECK(vkResetCommandBuffer(*g_inflight_main_command_buffer, 0)); // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.





    VkClearValue clear_value;
    clear_value.color = { { .1f, .1f, .1f } };

    // float flash       = abs(sin(vkr.frame_idx_inflight / 120.f));
    // clear_value.color = { { 0.0f, 0.0f, flash, 1.0f } };

    VkClearValue depth_clear;
    depth_clear.depth_stencil.depth   = 1.f;
    VkClearValue union_clear_values[] = { clear_value, depth_clear };




    //////////////////////
    // BEGIN renderpass
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext                 = NULL;
    rpInfo.renderPass            = vkr.render_pass;
    rpInfo.renderArea.offset.x   = 0;
    rpInfo.renderArea.offset.y   = 0;
    rpInfo.renderArea.extent     = vkr.window_extent;
    rpInfo.framebuffer           = vkr.framebuffers[idx_swapchain_image];
    rpInfo.clear_value_count     = 2;
    rpInfo.pClearValues          = &union_clear_values[0];




    // camera view
    glm::vec3 cam_pos     = { camera_x, 0.f + camera_y - 10, -30.f + camera_z };
    glm::mat4 translation = glm::translate(glm::mat4(1.f), cam_pos);
    glm::mat4 rotation    = glm::rotate(glm::mat4(1.f), .66f, glm::vec3(1, 0, 0));
    glm::mat4 view        = rotation * translation;
    glm::mat4 projection  = glm::perspective(glm::radians(60.f), (float)vkr.window_extent.width / (float)vkr.window_extent.height, 0.1f, 200.0f);
    projection[1][1] *= -1;

    vkr.camera.data.projection = projection;
    vkr.camera.data.view       = view;
    vkr.camera.data.viewproj   = projection * view;

    AllocateFillBuffer(&vkr.camera, sizeof(Camera::Data), (*buffer).allocation);




    ///////////////////////////////////////
    // Begin the command buffer recording
    VkCommandBufferBeginInfo cmd_begin_info = {};
    cmd_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext                    = NULL;
    cmd_begin_info.pInheritanceInfo         = NULL;
    cmd_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We will use this command buffer exactly once (per frame)
    VK_CHECK(vkBeginCommandBuffer(*g_inflight_main_command_buffer, &cmd_begin_info));

    vkCmdBeginRenderPass(*g_inflight_main_command_buffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);



    /////////////////////////////////////////////////////////////////////////////////////////////////
    /// Draw commands
    DrawExamples(g_inflight_main_command_buffer, g_inflight_global_descriptor_set, buffer, dt);
    // draw_Renderables(*g_inflight_main_command_buffer, vkr.renderables.data(), (uint32_t)vkr.renderables.size());



    //////////////////////////////////////////////////////////////////////////////////////////////////
    // END renderpass & command buffer recording
    vkCmdEndRenderPass(*g_inflight_main_command_buffer);
    VK_CHECK(vkEndCommandBuffer(*g_inflight_main_command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext        = NULL;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask  = &waitStage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = g_inflight_present_semaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = g_inflight_render_semaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = g_inflight_main_command_buffer;

    // submit command buffer to the queue and execute it.
    //  g_inflight_render_fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(vkr.graphics_queue_idx, 1, &submit_info, *g_inflight_render_fence));

    // this will put the image we just rendered into the visible window.
    // we want to g_inflight_render_semaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = NULL;

    presentInfo.pSwapchains    = &vkr.swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores    = g_inflight_render_semaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &idx_swapchain_image;

    VK_CHECK(vkQueuePresentKHR(vkr.graphics_queue_idx, &presentInfo));

    vkr.frame_idx_inflight++;
    vkr.frame_idx_inflight = vkr.frame_idx_inflight % FRAME_OVERLAP;
}





VkPipeline PipelineBuilder::BuildPipeline(VkDevice device, VkRenderPass pass)
{
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = NULL;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = NULL;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &attachment_state_color_blend;

    create_info_depth_stencil_state = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // build the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = NULL;

    pipelineInfo.stageCount          = (uint32_t)create_info_shader_stages.size();
    pipelineInfo.pStages             = create_info_shader_stages.data();
    pipelineInfo.pVertexInputState   = &create_info_vertex_input_state;
    pipelineInfo.pInputAssemblyState = &create_info_input_assembly_state;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization_state;
    pipelineInfo.pMultisampleState   = &create_info_multisample_state;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &create_info_depth_stencil_state;
    pipelineInfo.layout              = pipeline_layout;
    pipelineInfo.renderPass          = pass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &newPipeline)
        != VK_SUCCESS) {
        printf("failed to create pipline\n");
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    } else {
        return newPipeline;
    }
}




////////////////////////////////////////////////////
/// keeping around for reference
void InitMeshes()
{
    Mesh mesh_triangle;
    mesh_triangle.vertices.resize(3);

    mesh_triangle.vertices[0].position = { 0.2f, 0.2f, 0.0f };
    mesh_triangle.vertices[1].position = { -0.2f, 0.2f, 0.0f };
    mesh_triangle.vertices[2].position = { 0.f, -0.2f, 0.0f };

    mesh_triangle.vertices[0].color = { 0.f, 0.2f, 0.0f }; // pure green
    mesh_triangle.vertices[1].color = { 0.f, 0.2f, 0.0f }; // pure green
    mesh_triangle.vertices[2].color = { 0.f, 0.2f, 0.0f }; // pure green
    // we don't care about the vertex normals

    CreateMesh(mesh_triangle);

    Mesh mesh_monkey;
    mesh_monkey.LoadFromObj("../assets/monkey_smooth.obj");
    CreateMesh(mesh_monkey);

    vkr.meshes["monkey"]   = mesh_monkey;
    vkr.meshes["triangle"] = mesh_triangle;
}





void CreateMesh(Mesh &mesh)
{
    // allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
    // this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(vkr.allocator, &bufferInfo, &vmaallocInfo,
        &mesh.vertex_buffer.buffer,
        &mesh.vertex_buffer.allocation,
        NULL));

    // copy vertex data
    void *data;
    vmaMapMemory(vkr.allocator, mesh.vertex_buffer.allocation, &data); // gives a ptr to data
    memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex)); // copies vertices into data

    vmaUnmapMemory(vkr.allocator, mesh.vertex_buffer.allocation);
    // add the destruction of triangle mesh buffer to the deletion queue
    vkr.release_queue.push_function([=]() {
        vmaDestroyBuffer(vkr.allocator, mesh.vertex_buffer.buffer, mesh.vertex_buffer.allocation);
    });
}





static FrameData &get_CurrentFrameData()
{
    return vkr.frames[vkr.frame_idx_inflight % FRAME_OVERLAP];
}





bool InitShaderModule(const char *filepath, VkShaderModule *out_ShaderModule)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) return false;

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext                    = NULL;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode    = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vkr.device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    *out_ShaderModule = shaderModule;

    return true;
}





void vk_Cleanup()
{
    if (vkr.is_initialized) {
        vkWaitForFences(vkr.device, 1, &vkr.frames[0].render_fence, true, SECONDS(1));
        vkWaitForFences(vkr.device, 1, &vkr.frames[1].render_fence, true, SECONDS(1));

        vkr.release_queue.flush();

        vmaDestroyAllocator(vkr.allocator);

        vkDestroySurfaceKHR(vkr.instance, vkr.surface, NULL);
        // vkr.default_pipeline_layout
        vkDestroyPipelineLayout(vkr.device, vkr.default_pipeline_layout, NULL);
        vkDestroyPipeline(vkr.device, vkr.default_pipeline, NULL);
        vkDestroyDescriptorSetLayout(vkr.device, vkr.global_set_layout, NULL);
        vkDestroyDescriptorPool(vkr.device, vkr.descriptor_pool, NULL);
        vkDestroyDevice(vkr.device, NULL);

        // vkb::destroy_debug_utils_messenger(vkr.instance, vkr.debug_messenger);
        vkDestroyInstance(vkr.instance, NULL);
        SDL_DestroyWindow(vkr.window);
    }
}





VkResult CreateBuffer(BufferObject *dst_buffer, size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = NULL;

    bufferInfo.size  = alloc_size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memory_usage;

    // bind buffer buffer to allocation
    return vmaCreateBuffer(vkr.allocator, &bufferInfo, &vmaallocInfo, &dst_buffer->buffer, &dst_buffer->allocation, NULL);

    vkr.release_queue.push_function([=]() {
        vmaDestroyBuffer(vkr.allocator, dst_buffer->buffer, dst_buffer->allocation);
    });
}





VkResult AllocateFillBuffer(void *src, size_t size, VmaAllocation allocation)
{
    void *data;
    VkResult result = (vmaMapMemory(vkr.allocator, allocation, &data));
    memcpy(data, src, size);
    vmaUnmapMemory(vkr.allocator, allocation);

    return result;
}





///////////////////////////////////////
// todo(ad): must rethink these
Material *create_Material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name)
{
    Material mat;
    mat.pipeline        = pipeline;
    mat.pipelineLayout  = layout;
    vkr.materials[name] = mat;
    return &vkr.materials[name];
}





Material *get_Material(const std::string &name)
{
    // search for the object, and return nullpointer if not found
    auto it = vkr.materials.find(name);
    if (it == vkr.materials.end()) {
        return NULL;
    } else {
        return &(*it).second;
    }
}





Mesh *get_Mesh(const std::string &name)
{
    auto it = vkr.meshes.find(name);
    if (it == vkr.meshes.end()) {
        return NULL;
    } else {
        return &(*it).second;
    }
}





RenderObject *get_Renderable(std::string name)
{
    for (size_t i = 0; i < vkr.renderables.size(); i++) {
        if (vkr.renderables[i].mesh == get_Mesh(name))
            return &vkr.renderables[i];
    }
    return NULL;
}




VertexInputDescription GetVertexDescription()
{
    VertexInputDescription description;

    // we will have just 1 vertex buffer binding, with a per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};

    mainBinding.binding   = 0;
    mainBinding.stride    = sizeof(Vertex);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    description.bindings.push_back(mainBinding);

    // Position will be stored at Location 0
    VkVertexInputAttributeDescription positionAttribute = {};

    positionAttribute.binding  = 0;
    positionAttribute.location = 0;
    positionAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset   = offsetof(Vertex, position);

    // Normal will be stored at Location 1
    VkVertexInputAttributeDescription normalAttribute = {};

    normalAttribute.binding  = 0;
    normalAttribute.location = 1;
    normalAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset   = offsetof(Vertex, normal);

    // Color will be stored at Location 2
    VkVertexInputAttributeDescription colorAttribute = {};

    colorAttribute.binding  = 0;
    colorAttribute.location = 2;
    colorAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset   = offsetof(Vertex, color);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);

    return description;
}
///////////////////////////////////////