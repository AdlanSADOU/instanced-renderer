
#include "vk_renderer.h"
#include <vk_initializers.h>
#include <vk_types.h>
#include <fstream>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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
    uint32_t compute_queue_family_idx  = UINT32_MAX;
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

    for (uint32_t i = 0; i < queue_family_properties_count; i++) {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            if (compute_queue_family_idx == UINT32_MAX) {
                compute_queue_family_idx = i;
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
    SDL_Log("Graphics queue family idx: %d\n", graphics_queue_family_idx);
    SDL_Log("Compute  queue family idx: %d\n", compute_queue_family_idx);
    SDL_Log("Present  queue family idx: %d\n", present_queue_family_idx);

    if ((graphics_queue_family_idx & compute_queue_family_idx))
        SDL_Log("Separate Graphics and Compute Queues!\n");


    if ((graphics_queue_family_idx == UINT32_MAX) && (present_queue_family_idx == UINT32_MAX)) {
        // todo(ad): exit on error message
        SDL_LogError(0, "Failed to find Graphics and Present queues");
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
    gpu_vulkan_11_features.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    gpu_vulkan_11_features.shaderDrawParameters             = VK_TRUE;

    std::vector<VkDeviceQueueCreateInfo> create_info_device_queues = {};

    VkDeviceQueueCreateInfo ci_graphics_queue = {};
    ci_graphics_queue.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    ci_graphics_queue.pNext                   = NULL;
    ci_graphics_queue.flags                   = 0;
    ci_graphics_queue.queueFamilyIndex        = graphics_queue_family_idx;
    ci_graphics_queue.queueCount              = 1;
    ci_graphics_queue.pQueuePriorities        = queue_priorities;
    create_info_device_queues.push_back(ci_graphics_queue);

    VkDeviceQueueCreateInfo ci_compute_queue = {};
    ci_compute_queue.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    ci_compute_queue.pNext                   = NULL;
    ci_compute_queue.flags                   = 0;
    ci_compute_queue.queueFamilyIndex        = compute_queue_family_idx;
    ci_compute_queue.queueCount              = 1;
    ci_compute_queue.pQueuePriorities        = queue_priorities;
    create_info_device_queues.push_back(ci_compute_queue);

    VkPhysicalDeviceRobustness2FeaturesEXT robustness_feature_ext = {};
    robustness_feature_ext.sType                                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    robustness_feature_ext.nullDescriptor                         = VK_TRUE;

    // gpu_vulkan_11_features.pNext = &robustness_feature_ext;

    VkDeviceCreateInfo create_info_device   = {};
    create_info_device.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info_device.pNext                = &robustness_feature_ext;
    create_info_device.flags                = 0;
    create_info_device.queueCreateInfoCount = 1;
    create_info_device.pQueueCreateInfos    = create_info_device_queues.data();
    // create_info_device.enabledLayerCount       = ; // deprecated and ignored
    // create_info_device.ppEnabledLayerNames     = ; // deprecated and ignored
    create_info_device.enabledExtensionCount   = ARR_COUNT(enabled_device_extension_names);
    create_info_device.ppEnabledExtensionNames = enabled_device_extension_names;
    create_info_device.pEnabledFeatures        = &supported_gpu_features;

    VK_CHECK(vkCreateDevice(vkr.chosen_gpu, &create_info_device, NULL, &vkr.device));

    if (!vkr.is_present_queue_separate) {
        vkGetDeviceQueue(vkr.device, graphics_queue_family_idx, 0, &vkr.graphics_queue);
    } else {
        // todo(ad): get seperate present queue
    }

    vkGetDeviceQueue(vkr.device, vkr.compute_queue_family, 0, &vkr.compute_queue);


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
    create_info_depth_image_allocation.requiredFlags           = (VkMemoryPropertyFlags)VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

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
    // Graphics
    VkCommandPoolCreateInfo ci_graphics_cmd_pool = vkinit::CommandPoolCreateInfo(vkr.graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(vkr.device, &ci_graphics_cmd_pool, NULL, &vkr.command_pool_graphics));

    // Compute
    VkCommandPoolCreateInfo ci_compute_cmd_pool = vkinit::CommandPoolCreateInfo(vkr.compute_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(vkr.device, &ci_compute_cmd_pool, NULL, &vkr.command_pool_compute));

    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmd_buffer_alloc_info_graphics = vkinit::CommandBufferAllocateInfo(vkr.command_pool_graphics);
        VK_CHECK(vkAllocateCommandBuffers(vkr.device, &cmd_buffer_alloc_info_graphics, &vkr.frames[i].cmd_buffer_gfx));

        VkCommandBufferAllocateInfo cmd_buffer_alloc_info_compute = vkinit::CommandBufferAllocateInfo(vkr.command_pool_compute);
        VK_CHECK(vkAllocateCommandBuffers(vkr.device, &cmd_buffer_alloc_info_compute, &vkr.frames[i].cmd_buffer_cmp));
    }

    vkr.release_queue.push_function([=]() {
        vkDestroyCommandPool(vkr.device, vkr.command_pool_graphics, NULL);
        vkDestroyCommandPool(vkr.device, vkr.command_pool_compute, NULL);
    });




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

    // we have to have at least 1 subpass
    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    /////
    // RenderPass creation
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount        = 2; // connect the color attachment to the info
    render_pass_info.pAttachments           = &attachments[0];
    render_pass_info.subpassCount           = 1; // connect the subpass to the info
    render_pass_info.pSubpasses             = &subpass;
    VK_CHECK(vkCreateRenderPass(vkr.device, &render_pass_info, NULL, &vkr.render_pass));

    vkr.release_queue.push_function([=]() {
        vkDestroyRenderPass(vkr.device, vkr.render_pass, NULL);
    });



    ///////////////////////////////////////////////
    // Framebuffer initialization
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

    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++) {
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
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
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
    desc_set_layout_binding_UBO.binding                      = 0;
    desc_set_layout_binding_UBO.descriptorCount              = 1;
    desc_set_layout_binding_UBO.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_set_layout_binding_UBO.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT;

    /// layout > Camera UBO
    std::vector<VkDescriptorSetLayoutBinding> global_set_layout_bindings;
    global_set_layout_bindings.push_back(desc_set_layout_binding_UBO);
    CreateDescriptorSetLayout(vkr.device, NULL, &vkr.set_layout_global, global_set_layout_bindings.data(), (uint32_t)global_set_layout_bindings.size());

    /////////////////////////////////////
    /// Allocate sets for each camera buffer, we have a camera buffer for each framebuffer
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {

        //////////////////////////
        //// Set allocation
        // allocate one descriptor set for each frame


    }


    /// Set 1
    VkDescriptorSetLayoutBinding desc_set_layout_binding_sampler = {};
    desc_set_layout_binding_sampler.binding                      = 0;
    desc_set_layout_binding_sampler.descriptorCount              = 1;
    desc_set_layout_binding_sampler.descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLER;
    desc_set_layout_binding_sampler.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding desc_set_layout_binding_sampled_image = {};
    desc_set_layout_binding_sampled_image.binding                      = 1;
    desc_set_layout_binding_sampled_image.descriptorCount              = (uint32_t)80; // texture count
    desc_set_layout_binding_sampled_image.descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    desc_set_layout_binding_sampled_image.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

    /// layout > array of textures & sampler
    std::vector<VkDescriptorSetLayoutBinding> array_of_textures_set_layout_bindings;
    array_of_textures_set_layout_bindings.push_back(desc_set_layout_binding_sampler);
    array_of_textures_set_layout_bindings.push_back(desc_set_layout_binding_sampled_image);
    CreateDescriptorSetLayout(vkr.device, NULL, &vkr.set_layout_array_of_textures, array_of_textures_set_layout_bindings.data(), (uint32_t)array_of_textures_set_layout_bindings.size());

    // Sampler
    VkSamplerCreateInfo ci_sampler = {};
    ci_sampler.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci_sampler.minFilter           = VK_FILTER_NEAREST;
    ci_sampler.magFilter           = VK_FILTER_NEAREST;
    ci_sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    ci_sampler.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    ci_sampler.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    ci_sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VK_CHECK(vkCreateSampler(vkr.device, &ci_sampler, NULL, &vkr.sampler));

    vkr.default_pipeline = CreateGraphicsPipeline();

    vkr.is_initialized = true;

    vkr.descriptor_image_infos.resize(MAX_TEXTURE_COUNT);
    memset(vkr.descriptor_image_infos.data(), VK_NULL_HANDLE, vkr.descriptor_image_infos.size() * sizeof(VkDescriptorImageInfo));
}





/////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/// Rendering

void vk_BeginRenderPass()
{
    // wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VkFence         *g_inflight_render_fence;
    VkSemaphore     *g_inflight_present_semaphore;
    VkSemaphore     *g_inflight_render_semaphore;
    VkCommandBuffer *g_inflight_main_command_buffer;
    // VkDescriptorSet *g_inflight_global_descriptor_set;

    g_inflight_render_fence          = &vkr.frames[vkr.frame_idx_inflight % FRAME_BUFFER_COUNT].render_fence;
    g_inflight_present_semaphore     = &vkr.frames[vkr.frame_idx_inflight % FRAME_BUFFER_COUNT].present_semaphore;
    g_inflight_render_semaphore      = &vkr.frames[vkr.frame_idx_inflight % FRAME_BUFFER_COUNT].render_semaphore;
    g_inflight_main_command_buffer   = &vkr.frames[vkr.frame_idx_inflight % FRAME_BUFFER_COUNT].cmd_buffer_gfx;
    // g_inflight_global_descriptor_set = &vkr.frames[vkr.frame_idx_inflight % FRAME_BUFFER_COUNT].set_global;


    VK_CHECK(vkWaitForFences(vkr.device, 1, g_inflight_render_fence, true, SECONDS(1)));
    VK_CHECK(vkResetFences(vkr.device, 1, g_inflight_render_fence));


    VK_CHECK(vkAcquireNextImageKHR(vkr.device, vkr.swapchain, SECONDS(1), *g_inflight_present_semaphore, NULL, &vkr.frames[vkr.frame_idx_inflight].idx_swapchain_image));
    // VK_CHECK(vkResetCommandBuffer(*g_inflight_main_command_buffer, 0)); // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.


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
    rpInfo.framebuffer           = vkr.framebuffers[vkr.frames[vkr.frame_idx_inflight].idx_swapchain_image];
    rpInfo.clear_value_count     = 2;
    rpInfo.pClearValues          = &union_clear_values[0];

    ///////////////////////////////////////
    // Begin the command buffer recording
    VkCommandBufferBeginInfo cmd_begin_info = {};
    cmd_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext                    = NULL;
    cmd_begin_info.pInheritanceInfo         = NULL;
    cmd_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We will use this command buffer exactly once (per frame)
    VK_CHECK(vkBeginCommandBuffer(*g_inflight_main_command_buffer, &cmd_begin_info));

    vkCmdBeginRenderPass(*g_inflight_main_command_buffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void vk_EndRenderPass()
{
    ////////////////////////////////////////////////
    // END renderpass & command buffer recording
    vkCmdEndRenderPass(get_CurrentFrameData().cmd_buffer_gfx);
    VK_CHECK(vkEndCommandBuffer(get_CurrentFrameData().cmd_buffer_gfx));

    VkSubmitInfo submit_info = {};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext        = NULL;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask  = &waitStage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores    = &get_CurrentFrameData().present_semaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &get_CurrentFrameData().render_semaphore;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &get_CurrentFrameData().cmd_buffer_gfx;

    // submit command buffer to the queue and execute it.
    //  g_inflight_render_fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(vkr.graphics_queue, 1, &submit_info, get_CurrentFrameData().render_fence));

    // this will put the image we just rendered into the visible window.
    // we want to g_inflight_render_semaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = NULL;

    presentInfo.pSwapchains    = &vkr.swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores    = &get_CurrentFrameData().render_semaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &vkr.frames[vkr.frame_idx_inflight].idx_swapchain_image;

    VK_CHECK(vkQueuePresentKHR(vkr.graphics_queue, &presentInfo));

    vkr.frame_idx_inflight++;
    vkr.frame_idx_inflight = vkr.frame_idx_inflight % FRAME_BUFFER_COUNT;
}

void VulkanUpdateAndRender(double dt)
{



    ////////////////////////////////////////////////
    /// Draw commands
    // draw_Renderables(*g_inflight_main_command_buffer, vkr.renderables.data(), (uint32_t)vkr.renderables.size());
}

VkPipeline CreateGraphicsPipeline()
{
    VkPipelineColorBlendAttachmentState attachment_state_color_blend;
    attachment_state_color_blend = vkinit::color_blend_attachment_state(); // a single blend attachment with no blending and writing to RGBA

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};

    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext             = NULL;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.pAttachments      = &attachment_state_color_blend;
    colorBlending.attachmentCount   = 1;
    colorBlending.blendConstants[0] = 0.0;
    colorBlending.blendConstants[1] = 0.0;
    colorBlending.blendConstants[2] = 0.0;
    colorBlending.blendConstants[3] = 0.0;

    VkPipelineDepthStencilStateCreateInfo create_info_depth_stencil_state;
    create_info_depth_stencil_state = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    /////
    // Depth attachment
    // connect the pipeline builder vertex input info to the one we get from Vertex
    // build viewport and scissor from the swapchain extents
    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)vkr.window_extent.width;
    viewport.height   = (float)vkr.window_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = vkr.window_extent;

    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = NULL;
    viewportState.viewportCount                     = 1;
    viewportState.pViewports                        = &viewport;
    viewportState.scissorCount                      = 1;
    viewportState.pScissors                         = &scissor;



    ////////////////////////////
    // ShaderModules
    VkShaderModule vertexShader;
    if (!CreateShaderModule("./shaders/triangleMesh.vert.spv", &vertexShader)) {
        printf("Error when building the vertex shader module. Did you compile the shaders?\n");

    } else {
        printf("fragment shader succesfully loaded\n");
    }

    VkShaderModule fragmentShader;
    if (!CreateShaderModule("./shaders/textureArray.frag.spv", &fragmentShader)) {
        printf("Error when building the fragment shader module. Did you compile the shaders?\n");
    } else {
        printf("vertex shader succesfully loaded\n");
    }

    std::vector<VkPipelineShaderStageCreateInfo> create_info_shader_stages;
    create_info_shader_stages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    create_info_shader_stages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));

    vkr.release_queue.push_function([=]() {
        vkDestroyShaderModule(vkr.device, vertexShader, NULL);
        vkDestroyShaderModule(vkr.device, fragmentShader, NULL);
    });

    ////////////////////////////
    /// Vertex Input Description
    VertexInputDescription               vertexDescription = GetVertexDescription(); // horseshit, make this a useful function with arguments
    VkPipelineVertexInputStateCreateInfo create_info_vertex_input_state;
    create_info_vertex_input_state                                 = vkinit::VertexInputStateCreateInfo();
    create_info_vertex_input_state.pVertexAttributeDescriptions    = vertexDescription.attributes.data();
    create_info_vertex_input_state.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();
    create_info_vertex_input_state.pVertexBindingDescriptions      = vertexDescription.bindings.data();
    create_info_vertex_input_state.vertexBindingDescriptionCount   = (uint32_t)vertexDescription.bindings.size();

    ////////////////////////////
    /// Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo create_info_input_assembly_state;
    create_info_input_assembly_state = vkinit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    ////////////////////////////
    /// Raster State
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    rasterization_state = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL); // configure the rasterizer to draw filled triangles

    ////////////////////////////
    /// Multisample State
    VkPipelineMultisampleStateCreateInfo create_info_multisample_state;
    create_info_multisample_state = vkinit::multisampling_state_create_info(); // we don't use multisampling, so just run the default one


    ///////
    // PipelineLayout
    // this is where we provide the vertex shader with our matrices
    // VkPushConstantRange push_constant;
    // push_constant.offset     = 0;
    // push_constant.size       = sizeof(MeshPushConstants);
    // push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info(); // build the pipeline layout that controls the inputs/outputs of the shader
    // pipeline_layout_info.pushConstantRangeCount     = 1;
    // pipeline_layout_info.pPushConstantRanges        = &push_constant;

    std::vector<VkDescriptorSetLayout> set_layouts = {
        vkr.set_layout_global,
        vkr.set_layout_array_of_textures,
    };

    pipeline_layout_info.setLayoutCount = (uint32_t)set_layouts.size();
    pipeline_layout_info.pSetLayouts    = set_layouts.data();
    VK_CHECK(vkCreatePipelineLayout(vkr.device, &pipeline_layout_info, NULL, &vkr.default_pipeline_layout));

    // build the actual pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = NULL;
    pipelineInfo.stageCount                   = (uint32_t)create_info_shader_stages.size();
    pipelineInfo.pStages                      = create_info_shader_stages.data();
    pipelineInfo.pVertexInputState            = &create_info_vertex_input_state;
    pipelineInfo.pInputAssemblyState          = &create_info_input_assembly_state;
    pipelineInfo.pTessellationState           = NULL;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterization_state;
    pipelineInfo.pMultisampleState            = &create_info_multisample_state;
    pipelineInfo.pDepthStencilState           = &create_info_depth_stencil_state;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = NULL;
    pipelineInfo.layout                       = vkr.default_pipeline_layout;
    pipelineInfo.renderPass                   = vkr.render_pass;
    pipelineInfo.subpass                      = 0;
    pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex            = 0;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(vkr.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &newPipeline) != VK_SUCCESS) {
        printf("failed to create pipline\n");
        vkDestroyShaderModule(vkr.device, vertexShader, NULL);
        vkDestroyShaderModule(vkr.device, fragmentShader, NULL);
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    } else {
        return newPipeline;
    }
}


FrameData &get_CurrentFrameData()
{
    return vkr.frames[vkr.frame_idx_inflight % FRAME_BUFFER_COUNT];
}



bool CreateShaderModule(const char *filepath, VkShaderModule *out_ShaderModule)
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

        vkDestroySampler(vkr.device, vkr.sampler, NULL);
        vkDestroySurfaceKHR(vkr.instance, vkr.surface, NULL);
        vkDestroyPipelineLayout(vkr.device, vkr.default_pipeline_layout, NULL);
        vkDestroyPipeline(vkr.device, vkr.default_pipeline, NULL);
        vkDestroyDescriptorSetLayout(vkr.device, vkr.set_layout_global, NULL);
        vkDestroyDescriptorSetLayout(vkr.device, vkr.set_layout_array_of_textures, NULL);
        vkDestroyDescriptorPool(vkr.device, vkr.descriptor_pool, NULL);
        vkDestroyDevice(vkr.device, NULL);
        vkDestroyInstance(vkr.instance, NULL);
        SDL_DestroyWindow(vkr.window);
    }
}

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

        // Color will be stored at Location 2
        VkVertexInputAttributeDescription color_attribute = {};
        color_attribute.binding                           = 0;
        color_attribute.location                          = 2;
        color_attribute.format                            = VK_FORMAT_R32G32B32_SFLOAT;
        color_attribute.offset                            = offsetof(Vertex, color);

        VkVertexInputAttributeDescription tex_uv_attribute = {};
        tex_uv_attribute.binding                           = 0;
        tex_uv_attribute.location                          = 3;
        tex_uv_attribute.format                            = VK_FORMAT_R32G32_SFLOAT;
        tex_uv_attribute.offset                            = offsetof(Vertex, tex_uv);


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
        tex_idx_attribute.offset                            = offsetof(InstanceData, tex_idx);

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


VkResult CreateBuffer(BufferObject *dst_buffer, size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = NULL;

    bufferInfo.size  = alloc_size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memory_usage;

    // bind buffer to allocation
    VkResult res = vmaCreateBuffer(vkr.allocator, &bufferInfo, &vmaallocInfo, &dst_buffer->buffer, &dst_buffer->allocation, NULL);

    if (res == VK_SUCCESS) {
        vkr.release_queue.push_function([=]() {
            if (dst_buffer->buffer != VK_NULL_HANDLE)
                vmaDestroyBuffer(vkr.allocator, dst_buffer->buffer, dst_buffer->allocation);
        });
    }

    return res;
}

VkResult MapMemcpyMemory(void *src, size_t size, VmaAllocation allocation)
{
    // if (src == NULL)
        SDL_LogInfo(0, "%d:%s src buffer is NULL\n", __LINE__, __func__);

    void    *data;
    VkResult result = (vmaMapMemory(vkr.allocator, allocation, &data));
    memcpy(data, src, size);
    vmaUnmapMemory(vkr.allocator, allocation);

    return result;
}
