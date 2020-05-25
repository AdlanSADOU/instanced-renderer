#pragma once

// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

struct SwapchainInfo {
	VkPresentModeKHR presentMode;
	VkSurfaceFormatKHR surfaceFormat;
};

struct Engine
{
	VkInstance instance				= VK_NULL_HANDLE;
	VkSurfaceKHR surface			= VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device					= VK_NULL_HANDLE;
	VkQueue queueGraphics			= VK_NULL_HANDLE;
	SDL_Window *window				= VK_NULL_HANDLE;
	VkSwapchainKHR swapchain		= VK_NULL_HANDLE;
	Uint32 graphicsFamilyIndex		= 0;
	SwapchainInfo swapchainInfo	{};
	std::vector<VkImage> swapchainImages {};

	VkSurfaceFormatKHR GetSurfaceFormatIfAvailable(std::vector<VkSurfaceFormatKHR> &surfaceFormats) {
		for (const auto &format : surfaceFormats)
		{
			if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB)
				return format;
		}
		printf("VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && VK_FORMAT_B8G8R8A8_SRGB are not supported ! FIX THIS !!!\n");
		return surfaceFormats[0];
	}

	VkPresentModeKHR GetPresentModeIfAvailable(std::vector<VkPresentModeKHR> &modes, VkPresentModeKHR desired) {
		for (const auto &mode : modes)
		{
			if (mode == desired)
				return mode;
		}

		printf("Desired Present Mode not found, fallback default mode returned\n");
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	int SetupEngine() {

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			std::cout << "Could not initialize SDL." << std::endl;
			return 1;
		}
		this->window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (window == NULL) {
			std::cout << "Could not create SDL window." << std::endl;
			return 1;
		}

		// Use validation layers if this is a debug build
		std::vector<const char*> layers;
		// Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
		unsigned extension_count;
		if (!SDL_Vulkan_GetInstanceExtensions(this->window, &extension_count, NULL)) {
			std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
			return 1;
		}
		std::vector<const char*> extensions(extension_count);
		if (!SDL_Vulkan_GetInstanceExtensions(this->window, &extension_count, extensions.data())) {
			std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
			return 1;
		}

#ifdef _DEBUG
		layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif
		extensions.push_back("VK_KHR_surface");
#ifdef _DEBUG
		extensions.push_back("VK_EXT_debug_report");
#endif

		//////////////////////////////////////////////////////////////////
		// INSTANCE
		VkApplicationInfo appInfo = {};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pNext = NULL;
				appInfo.pApplicationName = "Vulkan Program Template";
				appInfo.applicationVersion = 1;
				appInfo.pEngineName = "LunarG SDK";
				appInfo.engineVersion = 1;
				appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instInfo = {};
				instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				instInfo.pNext = NULL;
				instInfo.flags = 0;
				instInfo.pApplicationInfo = &appInfo;
				instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
				instInfo.ppEnabledExtensionNames = extensions.data();
				instInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
				instInfo.ppEnabledLayerNames = layers.data();

		VkResult result = vkCreateInstance(&instInfo, NULL, &this->instance);
		if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
			std::cout << "Unable to find a compatible Vulkan Driver." << std::endl;
			return 1;
		}
		else if (result) {
			std::cout << "Could not create a Vulkan instance (for unknown reasons)." << std::endl;
			return 1;
		}
		//////////////////////////////////////////////////////////////////
		// SURFACE
		if (!SDL_Vulkan_CreateSurface(window, this->instance, &this->surface)) {
			std::cout << "Could not create a Vulkan surface." << std::endl;
			return 1;
		}

		//////////////////////////////////////////////////////////////////
		// PHYSICAL DEVICE & QUEUE FAMILIES
		
		// Querry available GPU, we'll take the first one available.
		// With the physical device we'll querry for the device properties which we'll not use yet.
		// With the physical device we'll querry for supported number of queue families

		Uint32 physicalDeviceCount = 0;
		result = vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, NULL);
		if (result != VK_SUCCESS)
			printf("Error enumerating device count.");

		result = vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, &this->physicalDevice);
		if (result != VK_SUCCESS)
			printf("Error enumerating devices.");

		VkPhysicalDeviceProperties phDeviceProps;
		vkGetPhysicalDeviceProperties(this->physicalDevice, &phDeviceProps);

		// needed for Device createtion
		VkPhysicalDeviceFeatures deviceFeatures{};
		vkGetPhysicalDeviceFeatures(this->physicalDevice, &deviceFeatures);
		
		UINT32 queueFPcount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFPcount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFPcount);
		vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFPcount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				this->graphicsFamilyIndex = i;
			}
			++i;
		}

#pragma region NEEDED FOR SWAPCHAIN
		VkBool32 is_surfaceSupported = VK_FALSE;

		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physicalDevice, this->surface, &surfaceCapabilities);
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, this->graphicsFamilyIndex, this->surface, &is_surfaceSupported);
		if (!is_surfaceSupported)
			printf("ERROR: surface not supported...!");

		Uint32 surfaceFormatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &surfaceFormatCount, NULL);
		surfaceFormats.resize(surfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(this->physicalDevice, this->surface, &surfaceFormatCount, surfaceFormats.data());
		this->swapchainInfo.surfaceFormat = GetSurfaceFormatIfAvailable(surfaceFormats);

		Uint32 presentModesCount = 0;
		std::vector<VkPresentModeKHR> presentModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->surface, &presentModesCount, NULL);
		presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->surface, &presentModesCount, presentModes.data());
		swapchainInfo.presentMode = GetPresentModeIfAvailable(presentModes, VK_PRESENT_MODE_MAILBOX_KHR);
#pragma endregion

		//////////////////////////////////////////////////////////////////
		// DEVICE & QUEUES

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
		// /!\ if out queue does not support surfaces, we should querry for another queue family that does support it !

		result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &this->device);

		vkGetDeviceQueue(this->device, this->graphicsFamilyIndex, 0, &this->queueGraphics);

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

		// One mistake that is not catched by the validation layers is if you don't enable the device extension VK_KHR_SWAPCHAIN_EXTENSION_NAME
		result = vkCreateSwapchainKHR(this->device, &swapchainCreateInfo, nullptr, &this->swapchain);

		result == VK_SUCCESS ? printf("all good!\n") : printf("somethig went wrong");
		return result;
	}
};