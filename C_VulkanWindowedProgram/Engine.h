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

#include <cstdio>
#include <cstdlib>
#include <vector>

#define VK_CHECK(status) \
	(status < VK_SUCCESS ? printf("something went wrong, and also... DO BETTER ERROR CHECKING THAN THIS !\nLine: %d\n", __LINE__) : 0); \

#define WIDTH 1280
#define HEIGHT 720
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define MAX_FRAMES_IN_FLIGHT 2

struct Texture {
	VkSampler		sampler;
	VkImage			image;
	VkImageLayout	layout;
	VkDeviceMemory	memory;
	Uint32			width, height;
	UINT32			mipLevels;
};

struct SwapchainInfo {
	VkSurfaceCapabilitiesKHR capabilities;
	VkPresentModeKHR presentMode;
	VkSurfaceFormatKHR surfaceFormat;
	std::vector<VkImage>Images {};
	std::vector<VkImageView>ImageViews{};
	Uint32 imageIndex = 0; // Index of currently acquired swapchain image
	std::vector<VkFramebuffer> framebuffers;
};

struct Renderer
{
public:
	SDL_Window			*window			= VK_NULL_HANDLE;
	VkInstance			instance		= VK_NULL_HANDLE;
	VkSurfaceKHR		surface			= VK_NULL_HANDLE;
	VkPhysicalDevice	physicalDevice	= VK_NULL_HANDLE;

	VkQueue				queueGraphics	= VK_NULL_HANDLE;
	VkDevice			device			= VK_NULL_HANDLE;
	
	VkCommandPool		commandPool		= VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers{};
	
	VkSwapchainKHR		swapchain		= VK_NULL_HANDLE;
	SwapchainInfo		swapchainInfo	{};
	Uint32				swhapchainImageCount = 0;
	VkRenderPass		renderPass		= VK_NULL_HANDLE;
	VkPipeline			pipeline		= VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout		= VK_NULL_HANDLE;

	VkDescriptorPool	descriptorPool	= VK_NULL_HANDLE;
	std::vector< VkDescriptorSetLayout> descriptorSetLayouts{};
	std::vector<char> vertShaderSPV{};
	std::vector<char> fragShaderSPV{};
	VkShaderModule vertModule = VK_NULL_HANDLE;
	VkShaderModule fragModule = VK_NULL_HANDLE;
	VkViewport viewport{};
	VkRect2D scissor{};

	std::vector<VkSemaphore> imageAcquireSemaphores{};
	std::vector<VkSemaphore> queueSubmittedSemaphore{};
	std::vector<VkFence> inFlightFences{};
	std::vector<VkFence> imagesInFlight{};
	size_t currentFrame = 0;

	VkClearColorValue	clearColor		{0.1f, 0.f, 0.f, 1.f};
	VkClearValue		clearValue{ clearColor};
	
private:
	Uint32						graphicsFamilyIndex	= 0;
	std::vector<const char*>	validationLayers;
	std::vector<const char*>	instanceExtensions;
	VkPhysicalDeviceProperties	physicalDeviceProperties;
	VkSurfaceCapabilitiesKHR	surfaceCapabilities{};
	VkPhysicalDeviceFeatures deviceFeatures{};
	

	inline VkSurfaceFormatKHR	GetSurfaceFormatIfAvailable	(std::vector<VkSurfaceFormatKHR> &surfaceFormats);
	inline VkPresentModeKHR	GetPresentModeIfAvailable	(std::vector<VkPresentModeKHR> &modes, VkPresentModeKHR desired);
	inline void Create_framebuffer(std::vector<VkFramebuffer> &framebuffers);
	inline int Create_SwapchainImageViews(VkImage image, VkImageView *imageView);
public:
	inline VkResult BeginDraw();
	inline void Create_Semaphores();
	inline VkResult Create_ShaderModule(VkShaderModule *shader_module, std::vector<char> &bytecode);
	inline int Load_Shader(char *filepath, std::vector<char> &bytecode);

	inline int Init_SDL();
	inline int Create_Window_SDL(const char *title, int x, int y, int w, int h, Uint32 flags);
	inline int Set_LayersAndInstanceExtensions();
	inline int Create_Surface_SDL();
	inline int Create_Instance();
	inline int Pick_PhysicalDevice();
	inline int Create_Device();
	inline int Create_CommandBuffers();
	inline int Create_Swapchain();
	inline int Create_renderPass();
	inline int Create_Pipeline();
};

//////////////////////////////////////////////////////////////////
// INITIALIZE SDL
inline int Renderer::Init_SDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("Could not initialize SDL.\n");
		return 0;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////
// SDL WINDOW
inline int Renderer::Create_Window_SDL(const char *title, int x, int y, int w, int h, Uint32 flags)
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
inline int Renderer::Set_LayersAndInstanceExtensions()
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
#ifdef _DEBUG
	validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif
	instanceExtensions.push_back("VK_KHR_surface");
#ifdef _DEBUG
	instanceExtensions.push_back("VK_EXT_debug_report");
#endif

	return 0;
}

//////////////////////////////////////////////////////////////////
// INSTANCE
inline int Renderer::Create_Instance()
{
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
inline int Renderer::Create_Surface_SDL()
{
	if (!SDL_Vulkan_CreateSurface(window, this->instance, &this->surface)) {
		printf("Could not create a Vulkan surface.\n");
		return 0;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE / QUEUE FAMILIES
inline int Renderer::Pick_PhysicalDevice()
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
// LOGICAL DEVICE / QUEUES / POOLS
inline int Renderer::Create_Device()
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

inline int Renderer::Create_CommandBuffers()
{
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = this->graphicsFamilyIndex;

	VK_CHECK(vkCreateCommandPool(this->device, &createInfo, NULL, &this->commandPool));

	this->commandBuffers.resize(2);
	//for (size_t i = 0; i < commandBuffers.size(); i++)
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.pNext = NULL;
		allocInfo.commandPool = this->commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 2;

		VK_CHECK(vkAllocateCommandBuffers(this->device, &allocInfo, this->commandBuffers.data()));

	//////////////////////////////////////////////////////
	// DESCRIPTOR POOL & SETS
	this->descriptorSetLayouts.resize(2);
	std::vector<VkDescriptorPoolSize> poolSizes(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;
	
#define NUM_DESC 2
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = NULL;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = NUM_DESC;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	VK_CHECK(vkCreateDescriptorPool(this->device, &descriptorPoolCreateInfo, NULL, &this->descriptorPool));

	std::vector< VkDescriptorSetLayoutCreateInfo> layoutCreateInfos(2);

	///////////////////////////////////////
	// UNIFORM BUFFER DESCRIPTOR LAYOUT
	/*VkDescriptorSetLayoutBinding uniformBufferSetLayoutBinding{};
	uniformBufferSetLayoutBinding.binding = 0;
	uniformBufferSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferSetLayoutBinding.descriptorCount = 1;
	uniformBufferSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uniformBufferSetLayoutBinding.pImmutableSamplers = NULL;

	layoutCreateInfos[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfos[0].pNext = NULL;
	layoutCreateInfos[0].flags = 0;
	layoutCreateInfos[0].bindingCount = 1;
	layoutCreateInfos[0].pBindings = &uniformBufferSetLayoutBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(this->device, &layoutCreateInfos[0], NULL, &this->descriptorSetLayouts[0]));*/

	///////////////////////////////////////
	// TEXTURE DESCRIPTOR LAYOUT & SAMPLER
	/*VkSamplerCreateInfo textureSamplerInfo{};
	textureSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	textureSamplerInfo.pNext = NULL;
	textureSamplerInfo.flags = 0;
	textureSamplerInfo.magFilter;
	textureSamplerInfo.minFilter;
	textureSamplerInfo.mipmapMode;
	textureSamplerInfo.addressModeU;
	textureSamplerInfo.addressModeV;
	textureSamplerInfo.addressModeW;
	textureSamplerInfo.mipLodBias;
	textureSamplerInfo.anisotropyEnable;
	textureSamplerInfo.maxAnisotropy;
	textureSamplerInfo.compareEnable;
	textureSamplerInfo.compareOp;
	textureSamplerInfo.minLod;
	textureSamplerInfo.maxLod;
	textureSamplerInfo.borderColor;
	textureSamplerInfo.unnormalizedCoordinates;

	VkSampler textureSampler = VK_NULL_HANDLE;
	VK_CHECK(vkCreateSampler(this->device, &textureSamplerInfo, NULL, &textureSampler));

	VkDescriptorSetLayoutBinding textureLayoutBinding{};
	textureLayoutBinding.binding = 1;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	textureLayoutBinding.pImmutableSamplers = &textureSampler;

	layoutCreateInfos[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfos[1].pNext = NULL;
	layoutCreateInfos[1].flags = 0;
	layoutCreateInfos[1].bindingCount = 1;
	layoutCreateInfos[1].pBindings = &textureLayoutBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(this->device, &layoutCreateInfos[1], NULL, &this->descriptorSetLayouts[1]));*/

	
	return 1;
}

//////////////////////////////////////////////////////////////////
// SWAPCHAIN
inline int Renderer::Create_Swapchain()
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
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //surfaceCapabilities.supportedUsageFlags;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = NULL;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; //surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = swapchainInfo.presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// One mistake that is not catched by the validationLayers is if you don't enable the device extension VK_KHR_SWAPCHAIN_EXTENSION_NAME, swapchain will be NULL
	VK_CHECK(vkCreateSwapchainKHR(this->device, &swapchainCreateInfo, nullptr, &this->swapchain));

	//return result;

	/////////////////////////////////:
	// GET SWAPCHAIN IMAGES & VIEWS
	VK_CHECK(vkGetSwapchainImagesKHR(this->device, this->swapchain, &swhapchainImageCount, NULL));
	this->swapchainInfo.Images.resize(swhapchainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(this->device, this->swapchain, &swhapchainImageCount, this->swapchainInfo.Images.data()));

	swapchainInfo.ImageViews.resize(swhapchainImageCount);
	VK_CHECK(Create_SwapchainImageViews(this->swapchainInfo.Images[0], &this->swapchainInfo.ImageViews[0]));
	VK_CHECK(Create_SwapchainImageViews(this->swapchainInfo.Images[1], &this->swapchainInfo.ImageViews[1]));

	return 0;
}

//////////////////////////////////////////////////////////////////
// RENDER PASS
inline int Renderer::Create_renderPass()
{
	VkAttachmentDescription color_attachment{};
	color_attachment.flags = 0;
	color_attachment.format = swapchainInfo.surfaceFormat.format;
	color_attachment.samples = NUM_SAMPLES;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount;
	subpass.pInputAttachments;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pResolveAttachments;
	subpass.pDepthStencilAttachment;
	subpass.preserveAttachmentCount;
	subpass.pPreserveAttachments;

	/*
	Subpass dependencies describe dependencies between different subpasses. 
	When an attachment is used in one specific way in a given subpass (for example, rendering into it),
	but in another way in another subpass (sampling from it), we can create a memory barrier or we can provide a subpass dependency
	that describes the intended usage of an attachment in these two subpasses. 
	Of course, the latter option is recommended, as the driver can (usually) prepare the barriers in a more optimal way.
	And the code itself is improved—everything required to understand the code is gathered in one place, one object.

	In our simple example, we have only one subpass, but we specify two dependencies.
	This is because we can (and should) specify dependencies between render passes (by providing the number of a given subpass)
	and operations outside of them (by providing a VK_SUBPASS_EXTERNAL value).
	Here we provide one dependency for color attachment between operations occurring before a render pass and its only subpass.
	The second dependency is defined for operations occurring inside a subpass and after the render pass.
	*/
	std::vector<VkSubpassDependency> dependencies = {
  {
	VK_SUBPASS_EXTERNAL,                            // uint32_t                       srcSubpass – Index of a first (previous) subpass or VK_SUBPASS_EXTERNAL if we want to indicate dependency between subpass and operations outside of a render pass.
	0,                                              // uint32_t                       dstSubpass – Index of a second (later) subpass (or VK_SUBPASS_EXTERNAL).
	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // VkPipelineStageFlags           srcStageMask – Pipeline stage during which a given attachment was used before (in a src subpass).
	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags           dstStageMask – Pipeline stage during which a given attachment will be used later (in a dst subpass).
	VK_ACCESS_MEMORY_READ_BIT,                      // VkAccessFlags                  srcAccessMask	– Types of memory operations that occurred in a src subpass or before a render pass.
	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // VkAccessFlags                  dstAccessMask	– Types of memory operations that occurred in a dst subpass or after a render pass.
	VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags – Flag describing the type (region) of dependency.
  },
  {
	0,                                              // uint32_t                       srcSubpass
	VK_SUBPASS_EXTERNAL,                            // uint32_t                       dstSubpass
	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags           srcStageMask
	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // VkPipelineStageFlags           dstStageMask
	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // VkAccessFlags                  srcAccessMask
	VK_ACCESS_MEMORY_READ_BIT,                      // VkAccessFlags                  dstAccessMask
	VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags
  }
	};

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &color_attachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = dependencies.size();
	renderPassCreateInfo.pDependencies = dependencies.data();
	VK_CHECK(vkCreateRenderPass(this->device, &renderPassCreateInfo, NULL, &this->renderPass));

	this->swapchainInfo.framebuffers.resize(2);
	Create_framebuffer(this->swapchainInfo.framebuffers);

	return 1;
}

//////////////////////////////////////////////////////////////////
// PIPELINE
inline int Renderer::Create_Pipeline()
{
	Load_Shader("shaders/vert.spv", this->vertShaderSPV);
	Load_Shader("shaders/frag.spv", this->fragShaderSPV);

	VK_CHECK(Create_ShaderModule(&vertModule, this->vertShaderSPV));
	VK_CHECK(Create_ShaderModule(&fragModule, this->fragShaderSPV));

	//////////////////////////
	// SHADER STAGES
	std::vector< VkPipelineShaderStageCreateInfo> shaderStages(2);
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].pNext = NULL;
	shaderStages[0].flags = 0;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertModule;
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].pNext = NULL;
	shaderStages[1].flags = 0;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragModule;
	shaderStages[1].pName = "main";

	///////////////////////
	// VERTEX INPUT STATE
	// TODO: fill to provide verticies through VertexBuffer as opposed in shader
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pNext = NULL;
	vertexInputStateCreateInfo.flags = 0;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexBindingDescriptions;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions;

	/////////////////////////
	// INPUT ASSEMBLY STATE
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.pNext = NULL;
	inputAssemblyState.flags = 0;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	/////////////////////////////
	// VIEWPORT & SCISSOR STATE
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = (float) WIDTH;
	viewport.height = (float) HEIGHT;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	scissor.extent.width = WIDTH;
	scissor.extent.height = HEIGHT;
	scissor.offset = VkOffset2D{ 0,0 };

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = NULL;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	///////////////////
	// RASTER STATE
	VkPipelineRasterizationStateCreateInfo rasterState{};
	rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterState.pNext = NULL;
	rasterState.flags = 0;
	rasterState.depthClampEnable;
	rasterState.rasterizerDiscardEnable;
	rasterState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterState.depthBiasEnable;
	rasterState.depthBiasConstantFactor;
	rasterState.depthBiasClamp;
	rasterState.depthBiasSlopeFactor;
	rasterState.lineWidth = 1.f;

	////////////////////////////////////////////////////////////
	// MULTISAMPLE STATE
	// rasterizationSamples must be greater than 0
	VkPipelineMultisampleStateCreateInfo multisampleState{};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pNext;
	multisampleState.flags;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable;
	multisampleState.minSampleShading;
	multisampleState.pSampleMask;
	multisampleState.alphaToCoverageEnable;
	multisampleState.alphaToOneEnable;

	////////////////////////////////////////////
	// DEPTH STENCIL
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.pNext = NULL;
	depthStencilState.flags = 0;
	depthStencilState.depthTestEnable;
	depthStencilState.depthWriteEnable;
	depthStencilState.depthCompareOp;
	depthStencilState.depthBoundsTestEnable;
	depthStencilState.stencilTestEnable;
	depthStencilState.front;
	depthStencilState.back;
	depthStencilState.minDepthBounds;
	depthStencilState.maxDepthBounds;

	//////////////////////////////////
	// COLOR BLEND STATE
	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.blendEnable;
	blendAttachmentState.srcColorBlendFactor;
	blendAttachmentState.dstColorBlendFactor;
	blendAttachmentState.colorBlendOp;
	blendAttachmentState.srcAlphaBlendFactor;
	blendAttachmentState.dstAlphaBlendFactor;
	blendAttachmentState.alphaBlendOp;
	blendAttachmentState.colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.pNext = NULL;
	colorBlendState.flags = 0;
	colorBlendState.logicOpEnable;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;
	colorBlendState.blendConstants[4];

	//////////////////
	// DYNAMIC STATES
	std::vector<VkDynamicState> dynamicStates(2);
	dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.pNext = NULL;
	dynamicStateInfo.flags = 0;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	//////////////////////////////:
	// PIPELINE LAYOUT
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = NULL;
	pipelineLayoutInfo.flags = 0;
	/*pipelineLayoutInfo.setLayoutCount = this->descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = this->descriptorSetLayouts.data();*/
	pipelineLayoutInfo.pushConstantRangeCount;
	pipelineLayoutInfo.pPushConstantRanges;

	VK_CHECK(vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, NULL, &pipelineLayout));

	VkGraphicsPipelineCreateInfo pipeCreateInfo{};
	pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeCreateInfo.pNext = NULL;
	pipeCreateInfo.flags = 0;
	pipeCreateInfo.stageCount = 2;
	pipeCreateInfo.pStages = shaderStages.data();
	pipeCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipeCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipeCreateInfo.pTessellationState;
	pipeCreateInfo.pViewportState = &viewportState;
	pipeCreateInfo.pRasterizationState = &rasterState;
	pipeCreateInfo.pMultisampleState = &multisampleState;
	pipeCreateInfo.pDepthStencilState = &depthStencilState;
	pipeCreateInfo.pColorBlendState = &colorBlendState;
	//pipeCreateInfo.pDynamicState = &dynamicStateInfo;
	pipeCreateInfo.layout = pipelineLayout;
	pipeCreateInfo.renderPass = this->renderPass;
	pipeCreateInfo.subpass;
	pipeCreateInfo.basePipelineHandle;
	pipeCreateInfo.basePipelineIndex = -1;

	vkCreateGraphicsPipelines(this->device, NULL, 1, &pipeCreateInfo, NULL, &this->pipeline);
	return 0;
}

/////////////////////////////////////////////////////
// RENDERING FUNCTIONS

inline VkResult Renderer::BeginDraw()
{
	

	return VkResult();
}

/////////////////////////////////////////////////////
// UTILITY FUNCTIONS

inline void Renderer::Create_Semaphores()
{
	/*VkSemaphoreTypeCreateInfo acquireSemaphoreType{};
	acquireSemaphoreType.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	acquireSemaphoreType.pNext = NULL;
	acquireSemaphoreType.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
	acquireSemaphoreType.initialValue = 0;
	
	VkSemaphoreTypeCreateInfo graphicsSemaphoreType{};
	graphicsSemaphoreType.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	graphicsSemaphoreType.pNext = NULL;
	graphicsSemaphoreType.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
	graphicsSemaphoreType.initialValue = 0;*/

	////--------------
	this->imageAcquireSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	this->queueSubmittedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	this->imagesInFlight.resize(this->swapchainInfo.ImageViews.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = NULL;
	semaphoreInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK(vkCreateSemaphore(this->device, &semaphoreInfo, NULL, &imageAcquireSemaphores[i]));
		VK_CHECK(vkCreateSemaphore(this->device, &semaphoreInfo, NULL, &queueSubmittedSemaphore[i]));
		VK_CHECK(vkCreateFence(this->device, &fenceInfo, NULL, &this->inFlightFences[i]));
	}

}

inline VkResult Renderer::Create_ShaderModule(VkShaderModule *shader_module, std::vector<char> &bytecode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.codeSize = bytecode.size();
	createInfo.pCode = reinterpret_cast<Uint32 *>(bytecode.data());

	return vkCreateShaderModule(this->device, &createInfo, NULL, shader_module);
}

inline int Renderer::Load_Shader(char *filepath, std::vector<char> &bytecode)
{
	long lsize = 0;
	size_t result = 0;

	FILE *fptr = NULL;
	fopen_s(&fptr, filepath, "rb");
	if (!fptr) {
		printf("ERROR: could not open: %s\n", filepath);
		exit(84);
	}

	fseek(fptr, 0, SEEK_END);
	lsize = ftell(fptr);
	rewind(fptr);

	bytecode.resize(lsize);

	result = fread(bytecode.data(), sizeof(char), lsize, fptr);
	if (result < 0) { printf("ERROR: during shader read: \n"); exit(84); }

	fclose(fptr);

	return (int)result;
}

inline void Renderer::Create_framebuffer(std::vector<VkFramebuffer> &framebuffers)
{
	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext = NULL;
		createInfo.renderPass = this->renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &this->swapchainInfo.ImageViews[i];
		createInfo.width = WIDTH;
		createInfo.height = HEIGHT;
		createInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(this->device, &createInfo, NULL, &framebuffers[i]));
	}
}

inline int Renderer::Create_SwapchainImageViews(VkImage image, VkImageView *imageView)
{
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = 1;
	range.layerCount = 1;

	VkImageViewCreateInfo IviewCreateInfo{};
	IviewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	IviewCreateInfo.pNext = NULL;
	IviewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	IviewCreateInfo.image = image;
	IviewCreateInfo.flags = 0;

	// VK_FORMAT_B8G8R8A8_SRGB with tiling VK_IMAGE_TILING_OPTIMAL does not support usage that includes VK_IMAGE_USAGE_STORAGE_BIT.
	// The Vulkan spec states: If usage contains VK_IMAGE_USAGE_STORAGE_BIT, then the image view's format features must contain VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT.
	IviewCreateInfo.format = this->swapchainInfo.surfaceFormat.format;

	IviewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	IviewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	IviewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	IviewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	IviewCreateInfo.subresourceRange = range;

	return (vkCreateImageView(this->device, &IviewCreateInfo, NULL, imageView));
}

inline VkSurfaceFormatKHR Renderer::GetSurfaceFormatIfAvailable(std::vector<VkSurfaceFormatKHR> &surfaceFormats) {
	for (const auto &format : surfaceFormats)
	{
		if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_UNORM)
			return format;
	}
	printf("VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && VK_FORMAT_B8G8R8A8_SRGB are not supported ! FIX THIS !!!\n");
	return surfaceFormats[0];
}

inline VkPresentModeKHR Renderer::GetPresentModeIfAvailable(std::vector<VkPresentModeKHR> &modes, VkPresentModeKHR desired) {
	for (const auto &mode : modes)
	{
		if (mode == desired)
			return mode;
	}

	printf("Desired Present Mode not found, fallback default mode returned\n");
	return VK_PRESENT_MODE_FIFO_KHR;
}