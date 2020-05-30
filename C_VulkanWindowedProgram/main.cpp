#include "Engine.h"

int main()
{
	Engine eng;
	
	eng.Init_SDL();
	eng.Create_Window_SDL("[Project Name] Window", 0, 0, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	eng.Set_LayersAndInstanceExtensions();
	eng.Create_Instance();
	eng.Create_Surface_SDL();
	eng.Pick_PhysicalDevice();
	eng.Create_Device();
	eng.Create_Pool();
	eng.Create_renderPass();
	eng.Create_Swapchain();
	
	VkSemaphore acquireSemaphore;
	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 1;
	VK_CHECK(vkCreateSemaphore(eng.device, &semaphoreCreateInfo, NULL, &acquireSemaphore));
	
	VkSemaphore graphicsSemaphore;
	VkSemaphoreCreateInfo semephoreCreateInfo;
	semephoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semephoreCreateInfo.pNext = NULL;
	semephoreCreateInfo.flags = 1;
	VK_CHECK(vkCreateSemaphore(eng.device, &semephoreCreateInfo, NULL, &graphicsSemaphore));



	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = VK_REMAINING_MIP_LEVELS;
	range.baseArrayLayer = 0;
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	

    bool stillRunning = true;
    while(stillRunning) {
		Uint32 imageIndex = 0;
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
			
			const Uint8 *keystate = SDL_GetKeyboardState(NULL);
			keystate[SDL_SCANCODE_ESCAPE] ? stillRunning = false : 0;

            switch(event.type) {

            case SDL_QUIT:
                stillRunning = false;
                break;

            default:
                break;
            }
        }

		VK_CHECK(vkAcquireNextImageKHR(eng.device, eng.swapchain, NULL, acquireSemaphore, NULL, &imageIndex));
	
		vkResetCommandPool(eng.device, eng.commandPool, 0);
		// begin cmd
		VkCommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = NULL;
		cmdBeginInfo.flags;
		cmdBeginInfo.pInheritanceInfo;
		VK_CHECK(vkBeginCommandBuffer(eng.commandBuffer, &cmdBeginInfo));
		
		// clear
		
		vkCmdClearColorImage(eng.commandBuffer, eng.swapchainImages[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &eng.clearColor, 1, &range);
		
		// end cmd
		VK_CHECK(vkEndCommandBuffer(eng.commandBuffer));

		// queue submit
		VkPipelineStageFlags pipeStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = NULL;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &acquireSemaphore;
		submitInfo.pWaitDstStageMask = &pipeStageFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &eng.commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &graphicsSemaphore;

		VK_CHECK(vkQueueSubmit(eng.queueGraphics, 1, &submitInfo, NULL));
		/*
		 presentInfo Validation ERROR: Images passed to present must be in layout VK_IMAGE_LAYOUT_PRESENT_SRC_KHR or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR but is in VK_IMAGE_LAYOUT_UNDEFINED.
		 The Vulkan spec states: Each element of pImageIndices must be the index of a presentable image acquired from the swapchain specified by the corresponding element of the pSwapchains array,
		 and the presented image subresource must be in the VK_IMAGE_LAYOUT_PRESENT_SRC_KHR layout at the time the operation is executed on a VkDevice
		 (https://github.com/KhronosGroup/Vulkan-Docs/search?q=VUID-VkPresentInfoKHR-pImageIndices-01296)

		 /!\ Meaning: "if you have an "empty" render loop that just presents images that have never been rendered to, you get error messages like this from the debug layers"
		 https://github.com/vulkano-rs/vulkano/issues/519
		*/
		// present queue
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = NULL;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &graphicsSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &eng.swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults;

		VK_CHECK(vkQueuePresentKHR(eng.queueGraphics, &presentInfo));

		VK_CHECK(vkDeviceWaitIdle(eng.device));
    }

    // Clean up.
	vkDestroyRenderPass(eng.device, eng.renderPass, NULL);
	vkDestroyCommandPool(eng.device, eng.commandPool, NULL);
	vkDestroySemaphore(eng.device, acquireSemaphore, NULL);
	vkDestroySemaphore(eng.device, graphicsSemaphore, NULL);
	vkDestroySwapchainKHR(eng.device, eng.swapchain, NULL);
    vkDestroySurfaceKHR(eng.instance, eng.surface, NULL);
	vkDestroyDevice(eng.device, NULL);
    SDL_DestroyWindow(eng.window);
    SDL_Quit();
    vkDestroyInstance(eng.instance, NULL);

    return 0;
}