#pragma once

// Enable the WSI extensions
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <vector>

#define VK_CHECK(status) \
	(status < VK_SUCCESS ? printf("something went wrong, and also... DO BETTER ERROR CHECKING THAN THIS !") : 0); \

struct SwapchainInfo {
	VkPresentModeKHR presentMode;
	VkSurfaceFormatKHR surfaceFormat;
};

struct Engine
{
	SDL_Window			*window			= VK_NULL_HANDLE;
	VkInstance			instance		= VK_NULL_HANDLE;
	VkSurfaceKHR		surface			= VK_NULL_HANDLE;
	VkPhysicalDevice	physicalDevice	= VK_NULL_HANDLE;
	VkQueue				queueGraphics	= VK_NULL_HANDLE;
	VkDevice			device			= VK_NULL_HANDLE;
	VkCommandPool		commandPool		= VK_NULL_HANDLE;
	VkDescriptorPool	descriptorPool	= VK_NULL_HANDLE;
	VkSwapchainKHR		swapchain		= VK_NULL_HANDLE;
	Uint32				swhapchainImageCount = 0;
	VkRenderPass		renderPass		= VK_NULL_HANDLE;
	std::vector<VkImage>swapchainImages {};
	VkClearColorValue	clearColor{0.1f, 0, 0.5f, 1};
	VkCommandBuffer		commandBuffer	= VK_NULL_HANDLE;
	
private:
	Uint32						graphicsFamilyIndex	= 0;
	SwapchainInfo				swapchainInfo	{};
	std::vector<const char*>	validationLayers;
	std::vector<const char*>	instanceExtensions;
	VkPhysicalDeviceProperties	physicalDeviceProperties;
	VkSurfaceCapabilitiesKHR	surfaceCapabilities{};
	VkPhysicalDeviceFeatures deviceFeatures{};

	VkSurfaceFormatKHR	GetSurfaceFormatIfAvailable	(std::vector<VkSurfaceFormatKHR> &surfaceFormats);
	VkPresentModeKHR	GetPresentModeIfAvailable	(std::vector<VkPresentModeKHR> &modes, VkPresentModeKHR desired);
public:

	inline int Init_SDL();
	inline int Create_Window_SDL(const char *title, int x, int y, int w, int h, Uint32 flags);
	inline int Set_LayersAndInstanceExtensions();
	inline int Create_Surface_SDL();
	inline int Create_Instance();
	inline int Pick_PhysicalDevice();
	inline int Create_Device();
	inline int Create_Pool();
	inline int Create_Swapchain();
	inline int Create_renderPass();
};

//////////////////////////////////////////////////////////////////
// INITIALIZE SDL
inline int Engine::Init_SDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("Could not initialize SDL.\n");
		return 0;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////
// SDL WINDOW
inline int Engine::Create_Window_SDL(const char *title, int x, int y, int w, int h, Uint32 flags)
{
	this->window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (this->window == NULL) {
		
		printf("Could not create SDL window.\n");
		return -1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// VALIDATION LAYERS / INSTANCE EXTENSIONS
inline int Engine::Set_LayersAndInstanceExtensions()
{
	// Get WSI instanceExtensions from SDL (we can add more if we like - we just can't remove these)
	unsigned extension_count;
	if (!SDL_Vulkan_GetInstanceExtensions(this->window, &extension_count, NULL)) {
		printf("Could not get the number of required instance instanceExtensions from SDL.\n");
		return -1;
	}

	instanceExtensions.resize(extension_count);
	if (!SDL_Vulkan_GetInstanceExtensions(this->window, &extension_count, instanceExtensions.data())) {
		printf("Could not get the names of required instance instanceExtensions from SDL.\n");
		return -1;
	}

	// Use validation validationLayers if this is a debug build
//#ifdef _DEBUG
	//validationLayers.push_back("VK_LAYER_KHRONOS_validation");
	validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
//#endif
	instanceExtensions.push_back("VK_KHR_surface");
//#ifdef _DEBUG
	instanceExtensions.push_back("VK_EXT_debug_report");
//#endif

	return 0;
}

//////////////////////////////////////////////////////////////////
// INSTANCE
inline int Engine::Create_Instance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = "Vulkan Program Template";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "LunarG SDK";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo instInfo = {};
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pNext = NULL;
	instInfo.flags = 0;
	instInfo.pApplicationInfo = &appInfo;
	instInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	instInfo.ppEnabledExtensionNames = instanceExtensions.data();
	instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	instInfo.ppEnabledLayerNames = validationLayers.data();
	

	VkResult result = vkCreateInstance(&instInfo, NULL, &this->instance);
	if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
		printf("Unable to find a compatible Vulkan Driver.\n");
		return -1;
	}
	else if (result) {
		printf("Could not create a Vulkan instance (for unknown reasons).\n");
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////
// SURFACE
inline int Engine::Create_Surface_SDL()
{
	if (!SDL_Vulkan_CreateSurface(window, this->instance, &this->surface)) {
		printf("Could not create a Vulkan surface.\n");
		return 0;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE / QUEUE FAMILIES
inline int Engine::Pick_PhysicalDevice()
{
	//////////////////////////////////////////////////////////////////
	// PHYSICAL DEVICE & QUEUE FAMILIES
	// Querry available GPU, we'll take the first one available.
	// With the physical device we'll querry for the device properties which we'll not use yet.
	// With the physical device we'll querry for supported number of queue families
	VkResult result;

	Uint32 physicalDeviceCount = 0;
	result = vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, NULL);
	if (result != VK_SUCCESS)
		printf("Error enumerating device count.\n");

	result = vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, &this->physicalDevice);
	if (result != VK_SUCCESS)
		printf("Error enumerating devices.\n");

	vkGetPhysicalDeviceProperties(this->physicalDevice, &physicalDeviceProperties);

	// needed for Device creation
	vkGetPhysicalDeviceFeatures(this->physicalDevice, &deviceFeatures);

	Uint32 queueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyPropertiesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyPropertiesCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			this->graphicsFamilyIndex = i;
		}
		++i;
	}

	// Needed for swapchain, dunno were to move this yet
	VkBool32 is_surfaceSupported = VK_FALSE;

	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physicalDevice, this->surface, &surfaceCapabilities));
	VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, this->graphicsFamilyIndex, this->surface, &is_surfaceSupported));
	if (!is_surfaceSupported) {
		printf("ERROR: surface not supported...!\n");
		return result;
	}

	/*	TODO:
		Needs further implementation improvement
		A given GPU might support multiple surfaceFormats and there can be multiple GPUs

		Maybe create a GPU struct that holds that information so it can be easier to deal with ?:
		GPUs[i].surfaceFormats[j] ?
	*/
	
	Uint32 surfaceFormatCount = 0;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &surfaceFormatCount, NULL);
	surfaceFormats.resize(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &surfaceFormatCount, surfaceFormats.data());
	this->swapchainInfo.surfaceFormat = GetSurfaceFormatIfAvailable(surfaceFormats);

	/*	TODO:
		Needs Further implementation improvement
		A given GPU might support multiple presentModes
	*/

	Uint32 presentModesCount = 0;
	std::vector<VkPresentModeKHR> presentModes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->surface, &presentModesCount, NULL);
	presentModes.resize(presentModesCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->surface, &presentModesCount, presentModes.data());
	swapchainInfo.presentMode = GetPresentModeIfAvailable(presentModes, VK_PRESENT_MODE_MAILBOX_KHR);
	
	return result;
}

//////////////////////////////////////////////////////////////////
// LOGICAL DEVICE / QUEUES
inline int Engine::Create_Device()
{
	VkResult result;

	float queuePriority = 1.f;
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = this->graphicsFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	std::vector<const char *> device_extensions;
	device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = NULL;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = (Uint32)device_extensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = device_extensions.data();

	////////////////////////////////
	// the graphics queue family we querried might not support surfaces
	// that's what whe're checking for here
	// /!\ if our queue does not support surfaces, we should querry for another queue family that does support it !

	result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &this->device);
	VK_CHECK(result);
	vkGetDeviceQueue(this->device, this->graphicsFamilyIndex, 0, &this->queueGraphics);

	return result;
}

inline int Engine::Create_Pool()
{
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = this->graphicsFamilyIndex;
	
	VK_CHECK(vkCreateCommandPool(this->device, &createInfo, NULL, &this->commandPool));

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.commandPool = this->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(this->device, &allocInfo, &this->commandBuffer));

	//// TODO::
	//VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	//descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	//descriptorPoolCreateInfo.pNext;
	//descriptorPoolCreateInfo.flags;
	//descriptorPoolCreateInfo.maxSets;
	//descriptorPoolCreateInfo.poolSizeCount;
	//descriptorPoolCreateInfo.pPoolSizes;
	//vkCreateDescriptorPool(this->device, &descriptorPoolCreateInfo, NULL, &this->descriptorPool);

	return 1;
}

inline int Engine::Create_Swapchain()
{
	//////////////////////////////////////////////////////////////////
	// SWAPCHAIN
	// Needs to be recreated on window resize ?

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.surface = this->surface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = this->swapchainInfo.surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = this->swapchainInfo.surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
	swapchainCreateInfo.imageArrayLayers = surfaceCapabilities.maxImageArrayLayers;
	swapchainCreateInfo.imageUsage = surfaceCapabilities.supportedUsageFlags;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = NULL;
	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = swapchainInfo.presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// One mistake that is not catched by the validationLayers is if you don't enable the device extension VK_KHR_SWAPCHAIN_EXTENSION_NAME, swapchain will be NULL
	VK_CHECK(vkCreateSwapchainKHR(this->device, &swapchainCreateInfo, nullptr, &this->swapchain));

	//return result;

	/////////////////////////////////:
	// GET SWAPCHAIN IMAGES

	
	vkGetSwapchainImagesKHR(this->device, this->swapchain, &swhapchainImageCount, NULL);
	this->swapchainImages.resize(swhapchainImageCount);
	vkGetSwapchainImagesKHR(this->device, this->swapchain, &swhapchainImageCount, this->swapchainImages.data());

	return 0;
}

inline int Engine::Create_renderPass()
{

	VkAttachmentDescription attachments;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.flags = VK_RENDER_PASS_CREATE_TRANSFORM_BIT_QCOM;
	renderPassCreateInfo.pAttachments = &attachments;
	
	vkCreateRenderPass(this->device, &renderPassCreateInfo, NULL, &this->renderPass);

	return 1;
}

/////////////////////////////////////////////////////
// UTILITY FUNCTIONS
inline VkSurfaceFormatKHR Engine::GetSurfaceFormatIfAvailable(std::vector<VkSurfaceFormatKHR> &surfaceFormats) {
	for (const auto &format : surfaceFormats)
	{
		if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB)
			return format;
	}
	printf("VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && VK_FORMAT_B8G8R8A8_SRGB are not supported ! FIX THIS !!!\n");
	return surfaceFormats[0];
}

inline VkPresentModeKHR Engine::GetPresentModeIfAvailable(std::vector<VkPresentModeKHR> &modes, VkPresentModeKHR desired) {
	for (const auto &mode : modes)
	{
		if (mode == desired)
			return mode;
	}

	printf("Desired Present Mode not found, fallback default mode returned\n");
	return VK_PRESENT_MODE_FIFO_KHR;
}