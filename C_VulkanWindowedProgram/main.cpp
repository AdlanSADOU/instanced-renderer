#include "Engine.h"

int main()
{
	Renderer ren;
	
	ren.Init_SDL();
	ren.Create_Window_SDL("[Project Name] Window", 0, 0, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	ren.Set_LayersAndInstanceExtensions();
	ren.Create_Instance();
	ren.Create_Surface_SDL();
	ren.Pick_PhysicalDevice();
	ren.Create_Device();
	ren.Create_CommandBuffers();
	ren.Create_Swapchain();
	ren.Create_renderPass();
	ren.Create_Semaphores();
	ren.Create_Pipeline();

    bool stillRunning = true;
    while(stillRunning) {
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

		vkWaitForFences(ren.device, 1, &ren.inFlightFences[ren.currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VK_CHECK(vkAcquireNextImageKHR(ren.device, ren.swapchain, UINT64_MAX, ren.imageAcquireSemaphores[ren.currentFrame], VK_NULL_HANDLE, &imageIndex));

		if (ren.imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(ren.device, 1, &ren.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		ren.imagesInFlight[imageIndex] = ren.inFlightFences[ren.currentFrame];

		{ /////////////////
			VkCommandBufferBeginInfo cmdBeginInfo{};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = NULL;
			cmdBeginInfo.flags;
			cmdBeginInfo.pInheritanceInfo;
			VK_CHECK(vkBeginCommandBuffer(ren.commandBuffers[ren.currentFrame], &cmdBeginInfo));

			VkRenderPassBeginInfo renderpassBeginInfo{};
			renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpassBeginInfo.pNext = NULL;
			renderpassBeginInfo.renderPass = ren.renderPass;
			renderpassBeginInfo.framebuffer = ren.swapchainInfo.framebuffers[ren.currentFrame];
			renderpassBeginInfo.renderArea.extent.width = WIDTH;
			renderpassBeginInfo.renderArea.extent.height = HEIGHT;
			renderpassBeginInfo.clearValueCount = 1;
			renderpassBeginInfo.pClearValues = &ren.clearValue;

			VkSubpassContents subpassContents{ VK_SUBPASS_CONTENTS_INLINE };

			vkCmdBeginRenderPass(ren.commandBuffers[ren.currentFrame], &renderpassBeginInfo, subpassContents);
			vkCmdBindPipeline(ren.commandBuffers[ren.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, ren.pipeline);
			vkCmdDraw(ren.commandBuffers[ren.currentFrame], 3, 1, 0, 0);
			vkCmdEndRenderPass(ren.commandBuffers[ren.currentFrame]);
			VK_CHECK(vkEndCommandBuffer(ren.commandBuffers[ren.currentFrame]));
		} //////////////////

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { ren.imageAcquireSemaphores[ren.currentFrame] };
		VkSemaphore signalSemaphores[] = { ren.queueSubmittedSemaphore[ren.currentFrame] };

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &ren.commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VK_CHECK(vkResetFences(ren.device, 1, &ren.inFlightFences[ren.currentFrame]));

		if (vkQueueSubmit(ren.queueGraphics, 1, &submitInfo, ren.inFlightFences[ren.currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { ren.swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		VK_CHECK(vkQueuePresentKHR(ren.queueGraphics, &presentInfo));

		ren.currentFrame = (ren.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		vkDeviceWaitIdle(ren.device);
    }

    // Clean up.
	vkDestroyImageView(ren.device, ren.swapchainInfo.ImageViews[0], NULL);
	vkDestroyImageView(ren.device, ren.swapchainInfo.ImageViews[1], NULL);
	vkDestroyShaderModule(ren.device, ren.vertModule, NULL);
	vkDestroyShaderModule(ren.device, ren.fragModule, NULL);
	vkDestroyPipelineLayout(ren.device, ren.pipelineLayout, NULL);
	vkDestroyPipeline(ren.device, ren.pipeline, NULL);
	vkDestroyFramebuffer(ren.device, ren.swapchainInfo.framebuffers[0], NULL);
	vkDestroyFramebuffer(ren.device, ren.swapchainInfo.framebuffers[1], NULL);
	vkDestroyRenderPass(ren.device, ren.renderPass, NULL);
	vkDestroyCommandPool(ren.device, ren.commandPool, NULL);
	vkDestroyDescriptorPool(ren.device, ren.descriptorPool, NULL);
	vkDestroyFence(ren.device, ren.inFlightFences[0], NULL);
	vkDestroyFence(ren.device, ren.inFlightFences[1], NULL);
	vkDestroySemaphore(ren.device, ren.imageAcquireSemaphores[0], NULL);
	vkDestroySemaphore(ren.device, ren.imageAcquireSemaphores[1], NULL);
	vkDestroySemaphore(ren.device, ren.queueSubmittedSemaphore[0], NULL);
	vkDestroySemaphore(ren.device, ren.queueSubmittedSemaphore[1], NULL);
	vkDestroySwapchainKHR(ren.device, ren.swapchain, NULL);
    vkDestroySurfaceKHR(ren.instance, ren.surface, NULL);
	vkDestroyDevice(ren.device, NULL);
    SDL_DestroyWindow(ren.window);
    SDL_Quit();
    vkDestroyInstance(ren.instance, NULL);
    return 0;
}