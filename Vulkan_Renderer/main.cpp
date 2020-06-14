#include "vk_context.h"
#include "vk_buffer.h"
#include "vk_texture.h"

int main()
{
	Context ren;

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

	// -----texture----
	Texture image;
	image.load_ImageData("resources/me.png");
	image.create_Image(ren.device);
	image.allocate_ImageMemory(ren.device, ren.deviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	image.create_ImageView(ren.device);
	image.create_Sampler(ren.device);
	//image.copy_TextureData(ren.device);

	//----------------------------------------------------
#define NUM_DESCSETS 1
	std::vector<VkDescriptorPoolSize> poolSizes(NUM_DESCSETS);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = NULL;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = NUM_DESCSETS;
	descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

	VkDescriptorPool descriptorPool{};
	vkCreateDescriptorPool(ren.device, &descriptorPoolCreateInfo, NULL, &descriptorPool);

#define	NUM_LAYOUTSETS 1
	// LAYOUTS ARRAY
	std::vector<VkDescriptorSetLayout> setLayouts(NUM_LAYOUTSETS);

	// BINDINGS
	VkDescriptorSetLayoutBinding setLayoutBinding{};
	setLayoutBinding.binding = 0;
	setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBinding.descriptorCount = 1;
	setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	setLayoutBinding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.pNext = NULL;
	setLayoutCreateInfo.flags = 0;
	setLayoutCreateInfo.bindingCount = 1;
	setLayoutCreateInfo.pBindings = &setLayoutBinding;
	vkCreateDescriptorSetLayout(ren.device, &setLayoutCreateInfo, NULL, &setLayouts[0]);

	// SETS
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.pNext = NULL;
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = NUM_DESCSETS;
	descriptorSetAllocInfo.pSetLayouts = setLayouts.data();

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	vkAllocateDescriptorSets(ren.device, &descriptorSetAllocInfo, &descriptorSet);

	VkCopyDescriptorSet copySet{};
	copySet.sType;
	copySet.pNext;
	copySet.srcSet;
	copySet.srcBinding;
	copySet.srcArrayElement;
	copySet.dstSet;
	copySet.dstBinding;
	copySet.dstArrayElement;
	copySet.descriptorCount;
	
	VkWriteDescriptorSet writeSet{};
	writeSet.sType;
	writeSet.pNext;
	writeSet.dstSet;
	writeSet.dstBinding;
	writeSet.dstArrayElement;
	writeSet.descriptorCount;
	writeSet.descriptorType;
	writeSet.pImageInfo;
	writeSet.pBufferInfo;
	writeSet.pTexelBufferView;

	//vkUpdateDescriptorSets(ren.device, 1, &writeSet, 1, &copySet);

	// ---------------
	Buffer vertexBuffer;
	vertexBuffer.create_Buffer(ren.device, ren.deviceMemoryProperties, sizeof(vertex_Data), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Buffer stagingBuffer; 
	stagingBuffer.create_Buffer(ren.device, ren.deviceMemoryProperties, sizeof(vertex_Data), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	stagingBuffer.mapCopyData(vertex_Data, sizeof(vertex_Data));
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

			VkBufferCopy bufferCopyInfo{};
			bufferCopyInfo.dstOffset = 0;
			bufferCopyInfo.srcOffset = 0;
			bufferCopyInfo.size = vertexBuffer.getSize();
			vkCmdCopyBuffer(ren.commandBuffers[ren.currentFrame], stagingBuffer.handle, vertexBuffer.handle, 1, &bufferCopyInfo);

			VkBufferMemoryBarrier BufferBarrier{};
			BufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			BufferBarrier.pNext = NULL;
			BufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			BufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			BufferBarrier.buffer = vertexBuffer.handle;
			BufferBarrier.offset = 0;
			BufferBarrier.size = VK_WHOLE_SIZE;

			vkCmdBeginRenderPass(ren.commandBuffers[ren.currentFrame], &renderpassBeginInfo, subpassContents);
			vkCmdBindPipeline(ren.commandBuffers[ren.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, ren.pipeline);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(ren.commandBuffers[ren.currentFrame], 0, 1, &vertexBuffer.handle, &offset);
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
			throw std::runtime_error("failed to submit draw command vertexBuffer!");
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
	//vkDestroyBuffer(ren.device, Buffer, NULL);
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