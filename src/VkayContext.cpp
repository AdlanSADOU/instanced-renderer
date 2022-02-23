
#include "Vkay.h"
#include "VkayInitializers.h"
#include "VkayTypes.h"
#include <vk_mem_alloc.h>

void VkayContextInit(VkayContext *vkc)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    vkc->window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        vkc->window_extent.width,
        vkc->window_extent.height,
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
    SDL_Vulkan_GetInstanceExtensions(vkc->window, &required_extensions_count, NULL);
    // std::vector<char *> required_instance_extensions(required_extensions_count);
    const char **required_instance_extensions = new const char *[required_extensions_count];
    SDL_Vulkan_GetInstanceExtensions(vkc->window, &required_extensions_count, required_instance_extensions);

    const char *validation_layers[] = {
#if _DEBUG
        { "VK_LAYER_KHRONOS_validation" },
#endif
        { "VK_LAYER_LUNARG_monitor" },
    };

    VkApplicationInfo application_info  = {};
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext              = NULL;
    application_info.pApplicationName   = "name";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    application_info.pEngineName        = "engine name";
    application_info.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    application_info.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info_instance    = {};
    create_info_instance.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info_instance.pNext                   = &validation_features;
    create_info_instance.enabledLayerCount       = ARR_COUNT(validation_layers);
    create_info_instance.ppEnabledLayerNames     = validation_layers;
    create_info_instance.flags                   = 0;
    create_info_instance.pApplicationInfo        = &application_info;
    create_info_instance.enabledExtensionCount   = required_extensions_count;
    create_info_instance.ppEnabledExtensionNames = required_instance_extensions;
    VK_CHECK(vkCreateInstance(&create_info_instance, NULL, &vkc->instance));



    if (!SDL_Vulkan_CreateSurface(vkc->window, vkc->instance, &vkc->surface)) {
        SDL_Log("SDL Failed to create Surface");
    }


    ////////////////////////////////////
    /// GPU Selection
    uint32_t gpu_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vkc->instance, &gpu_count, NULL));

    // todo(ad): we're arbitrarily picking the first gpu we encounter right now
    // wich often is the one we want anyways. Must be improved to let user chose based features/extensions and whatnot
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    VK_CHECK(vkEnumeratePhysicalDevices(vkc->instance, &gpu_count, gpus.data()));
    vkc->chosen_gpu = gpus[0];



    //////////////////////////////////////////
    /// Query for graphics & present Queue/s
    // for each queue family idx, try to find one that supports presenting
    uint32_t queue_family_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkc->chosen_gpu, &queue_family_properties_count, NULL);
    VkQueueFamilyProperties *queue_family_properties = new VkQueueFamilyProperties[queue_family_properties_count];
    vkGetPhysicalDeviceQueueFamilyProperties(vkc->chosen_gpu, &queue_family_properties_count, queue_family_properties);

    if (queue_family_properties == NULL) {
        // some error message
        return;
    }

    VkBool32 *queue_idx_supports_present = new VkBool32[queue_family_properties_count];
    for (uint32_t i = 0; i < queue_family_properties_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(vkc->chosen_gpu, i, vkc->surface, &queue_idx_supports_present[i]);
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

    vkc->graphics_queue_family     = graphics_queue_family_idx;
    vkc->present_queue_family      = present_queue_family_idx;
    vkc->is_present_queue_separate = (graphics_queue_family_idx != present_queue_family_idx);

    const float queue_priorities[] = {
        { 1.0 }
    };



    /////////////////////////////
    /// Device
    VkPhysicalDeviceFeatures supported_gpu_features = {};
    vkGetPhysicalDeviceFeatures(vkc->chosen_gpu, &supported_gpu_features);

    // todo(ad): not used right now
    uint32_t device_properties_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(vkc->chosen_gpu, NULL, &device_properties_count, NULL));
    VkExtensionProperties *device_extension_properties = new VkExtensionProperties[device_properties_count];
    VK_CHECK(vkEnumerateDeviceExtensionProperties(vkc->chosen_gpu, NULL, &device_properties_count, device_extension_properties));

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

    VK_CHECK(vkCreateDevice(vkc->chosen_gpu, &create_info_device, NULL, &vkc->device));

    if (!vkc->is_present_queue_separate) {
        vkGetDeviceQueue(vkc->device, graphics_queue_family_idx, 0, &vkc->graphics_queue);
    } else {
        // todo(ad): get seperate present queue
    }

    vkGetDeviceQueue(vkc->device, vkc->compute_queue_family, 0, &vkc->compute_queue);

    vkc->is_initialized = true;
}

void VkayContextCleanup(VkayContext *vkc)
{
    if (vkc->is_initialized) {
        vkDeviceWaitIdle(vkc->device);

        vkc->release_queue.flush();


        vkDestroySurfaceKHR(vkc->instance, vkc->surface, NULL);

        vkDestroyDevice(vkc->device, NULL);
        vkDestroyInstance(vkc->instance, NULL);
        SDL_DestroyWindow(vkc->window);
    }
}
