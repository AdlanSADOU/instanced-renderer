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

	//----------------------------------------------------
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = sizeof(vertex_Data);
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // indicate that till buffer will contain vertex data
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0; // only when concurrent sharing mode is specified - presentation and graphics is done on the same queue so this buffer does not need to be shared with another queue
	bufferCreateInfo.pQueueFamilyIndices = NULL; // only when concurrent sharing mode is specified

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferDeviceMemory = VK_NULL_HANDLE;

	VK_CHECK(vkCreateBuffer(ren.device, &bufferCreateInfo, NULL, &vertexBuffer));

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(ren.device, vertexBuffer, &memoryRequirements);

	//  we must find a memoryType that support our memoryRequirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : our additional requirement is that memory needs to be host visible
	for (uint32_t i = 0; i < ren.memoryProperties.memoryTypeCount; ++i) {
		if ((memoryRequirements.memoryTypeBits & (1 << i)) && (ren.memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		{
			VkMemoryAllocateInfo memoryAllocateInfo = {
			  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	  // VkStructureType                        sType
			  NULL,										  // const void                            *pNext
			  memoryRequirements.size,					  // VkDeviceSize                           allocationSize
			  i                                           // uint32_t                               memoryTypeIndex
			};
			VK_CHECK(vkAllocateMemory(ren.device, &memoryAllocateInfo, NULL, &vertexBufferDeviceMemory));
		}
	}
	
	VK_CHECK(vkBindBufferMemory(ren.device, vertexBuffer, vertexBufferDeviceMemory, 0));
	
	void *vertexBufferMemoryPointer;
	VK_CHECK(vkMapMemory(ren.device, vertexBufferDeviceMemory, 0, sizeof(vertex_Data), 0, &vertexBufferMemoryPointer));
	memcpy(vertexBufferMemoryPointer, vertex_Data, sizeof(vertex_Data));

	VkMappedMemoryRange flushRange{};
	flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	flushRange.pNext = NULL;
	flushRange.memory = vertexBufferDeviceMemory;
	flushRange.offset = 0;
	flushRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(ren.device, 1, &flushRange);

	vkUnmapMemory(ren.device, vertexBufferDeviceMemory);
	//____________________________________________________

    bool stillRunning = true;
    while(stillRunning) {
		VkResult result{};
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

		//system("cls");

		if (vkWaitForFences(ren.device, 1, &ren.inFlightFences[ren.currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
			printf("Fence: Waiting takes too long...!\n");
		}

		uint32_t imageIndex;
		VK_CHECK((result = vkAcquireNextImageKHR(ren.device, ren.swapchain, UINT64_MAX, ren.imageAcquireSemaphores[ren.currentFrame], VK_NULL_HANDLE, &imageIndex)));
		switch (result)
		{
		case VK_SUCCESS:
			break;
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			break;
		default:
			break;
		}

		if (ren.imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(ren.device, 1, &ren.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		ren.imagesInFlight[imageIndex] = ren.inFlightFences[ren.currentFrame];

		{ /////////////////
			VkCommandBufferBeginInfo cmdBeginInfo{};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = NULL;
			cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
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

			VkViewport viewport{};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = WIDTH;
			viewport.height = HEIGHT;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(ren.commandBuffers[ren.currentFrame], 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.extent = { WIDTH, HEIGHT };
			scissor.offset = { 0,0 };
			vkCmdSetScissor(ren.commandBuffers[ren.currentFrame], 0, 1, &scissor);

			VkSubpassContents subpassContents{ VK_SUBPASS_CONTENTS_INLINE };

			vkCmdBeginRenderPass(ren.commandBuffers[ren.currentFrame], &renderpassBeginInfo, subpassContents);
			vkCmdBindPipeline(ren.commandBuffers[ren.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, ren.pipeline);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(ren.commandBuffers[ren.currentFrame], 0, 1, &vertexBuffer, &offset);
			vkCmdDraw(ren.commandBuffers[ren.currentFrame], 4, 1, 0, 0);
			
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

		VK_CHECK((result = vkQueuePresentKHR(ren.queueGraphics, &presentInfo)));
		switch (result)
		{
		case VK_SUCCESS:
			break;
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			break;
		default:
			break;
		}
		ren.currentFrame = (ren.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		vkDeviceWaitIdle(ren.device);
    }

    // Clean up.
	vkDestroyBuffer(ren.device, vertexBuffer, NULL);
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