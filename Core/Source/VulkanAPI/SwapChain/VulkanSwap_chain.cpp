#include "VulkanSwap_chain.hpp"
#include <filesystem>


// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
namespace VULKAN {

	VulkanSwapChain::VulkanSwapChain(MyVulkanDevice& deviceRef, VkExtent2D extent)
		: device{ deviceRef }, windowExtent{ extent } {
		Init();
	}
	VulkanSwapChain::VulkanSwapChain(MyVulkanDevice& deviceRef, VkExtent2D extent, std::shared_ptr<VulkanSwapChain> previous)
		: device{ deviceRef }, windowExtent{ extent }, oldSwapChain{ previous } {
		Init();

		oldSwapChain = nullptr;
	}


	VulkanSwapChain::~VulkanSwapChain() {
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device.device(), imageView, nullptr);
		}
		for (int i = 0; i < depthImages.size(); i++) {
			vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
			vkDestroyImage(device.device(), depthImages[i], nullptr);
			vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
		}

		if (swapChain != nullptr) {
			vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
			swapChain = nullptr;
		}
		depthImageViews.clear();
		colorUIImageView.clear();
		depthImages.clear();
		colorUIImages.clear();
		depthImageMemorys.clear();

		vkDestroyImageView(device.device(), colorImageView, nullptr);
		vkDestroyImage(device.device(), colorImage, nullptr);
		vkFreeMemory(device.device(), colorImageMemory, nullptr);

		swapChainImageViews.clear();


		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
		}
		for (auto UIFramebuffer : UIframebuffers)
		{
			vkDestroyFramebuffer(device.device(), UIFramebuffer, nullptr);
		}

		vkDestroyRenderPass(device.device(), renderPass, nullptr);
		vkDestroyRenderPass(device.device(), UIRenderPass, nullptr);

		// cleanup synchronization objects
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device.device(), inFlightFences[i], nullptr);
			vkDestroySemaphore(device.device(), computeRenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device.device(), computeInFlightFences[i], nullptr);

		}
	}

	VkResult VulkanSwapChain::acquireNextImage(uint32_t* imageIndex) {
		vkWaitForFences(
			device.device(),
			1,
			&inFlightFences[currentFrame],
			VK_TRUE,
			std::numeric_limits<uint64_t>::max());

		VkResult result = vkAcquireNextImageKHR(
			device.device(),
			swapChain,
			std::numeric_limits<uint64_t>::max(),
			imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
			VK_NULL_HANDLE,
			imageIndex);

		return result;
	}

	VkResult VulkanSwapChain::submitCommandBuffers(
		const VkCommandBuffer* buffers, uint32_t* imageIndex) {
		if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo = {};

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {/* computeRenderFinishedSemaphores[currentFrame] ,*/ imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = {/* VK_PIPELINE_STAGE_VERTEX_INPUT_BIT ,*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;


		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;


		vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
		if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex;

		auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		return VK_SUCCESS;
	}

//	VkResult VulkanSwapChain::submitComputeCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
//	{
//		 Compute submission
//
//		VkSubmitInfo submitInfo = {};
//		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//		submitInfo.commandBufferCount = 1;
//		submitInfo.pCommandBuffers = buffers;
//		submitInfo.signalSemaphoreCount = 1;
//		submitInfo.pSignalSemaphores = &computeRenderFinishedSemaphores[currentFrame];
//		auto result = vkQueueSubmit(device.computeQueue_, 1, &submitInfo, computeInFlightFences[currentFrame]);
//		if (result != VK_SUCCESS) {
//			throw std::runtime_error("failed to submit compute command buffer!");
//		};
//		return result;
//	}
//
	void VulkanSwapChain::HandleColorImage(VkImage image, 
		VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer, 
		VkPipelineStageFlagBits srcStageFlags,VkPipelineStageFlagBits dstStageFlags,
		VkAccessFlagBits accessMask,VkAccessFlagBits dstAccessMask)
	{
		
		VkImageMemoryBarrier newBarrier = {};
		newBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		newBarrier.srcAccessMask = accessMask;
		newBarrier.dstAccessMask = dstAccessMask;
		newBarrier.oldLayout = oldLayout;
		newBarrier.newLayout = newLayout;
		newBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		newBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		newBarrier.image = image;
		newBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		newBarrier.subresourceRange.baseMipLevel = 0;
		newBarrier.subresourceRange.levelCount = 1;
		newBarrier.subresourceRange.baseArrayLayer = 0;
		newBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStageFlags,
			dstStageFlags,
			0,
			0, nullptr,
			0, nullptr,
			1, &newBarrier
		);
	}

	void VulkanSwapChain::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
	                                  VkFormat format, VkImageTiling tilling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};

		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tilling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = numSamples;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device.device(), &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image!");
		}
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device.device(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);
		if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate image memory!");
		}
		vkBindImageMemory(device.device(), image, imageMemory, 0);

	}

	void VulkanSwapChain::CreateImageSamples(VkSampler& sampler, float mipLevels)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		//Not using anistropy;
		//samplerInfo.anisotropyEnable = VK_TRUE;
		//samplerInfo.maxAnisotropy = mySwapChain.device.properties.limits.maxSamplerAnisotropy;

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;


		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = mipLevels;
		if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture sampler :/ ");
		}

	}


	void VulkanSwapChain::Init()
	{
		createSwapChain();
		createImageViews();
		createRenderPass();
		CreateColorResources();
		createDepthResources();
		createFramebuffers();
		createUIImageViews();
		CreateDynamicRenderPass(UIRenderPass);
        CreateDynamicRenderPass(PostProRenderPass);
        CreateDynamicRenderPass(UpSampleRenderPass);
        CreateDynamicRenderPass(DownSampleRenderPass);
        CreateDynamicRenderPass(FinalRenderPass);
		CreateUIFramebuffers();
		createSyncObjects();
	}

	void VulkanSwapChain::createSwapChain() {
		SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.surface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.graphicsAndComputeFamily, indices.presentFamily };

		if (indices.graphicsAndComputeFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

		if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// we only specified a minimum number of images in the swap chain, so the implementation is
		// allowed to create a swap chain with more. That's why we'll first query the final number of
		// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
		// retrieve the handles.
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		colorUIImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, colorUIImages.data());



		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void VulkanSwapChain::createImageViews() {

		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = swapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) !=
				VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}

		}
	}

	void VulkanSwapChain::createRenderPass() {



		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = getSwapChainImageFormat();
		colorAttachment.samples = device.msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = device.msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = getSwapChainImageFormat();
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		if (activateMsaa)
		{
			subpass.pResolveAttachments = &colorAttachmentResolveRef;
		}

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		dependency.dstSubpass = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;


		std::vector<VkAttachmentDescription> attachments = { colorAttachment ,depthAttachment };
		if (activateMsaa)
		{
			attachments = { colorAttachment,depthAttachment,colorAttachmentResolve };
		}
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

	}

	void VulkanSwapChain::createFramebuffers() {
		swapChainFramebuffers.resize(imageCount());
		for (size_t i = 0; i < imageCount(); i++) {

			std::vector<VkImageView> attachments = { colorImageView, depthImageViews[i] };
			if (activateMsaa)
			{
				attachments = { colorImageView , depthImageViews[i], swapChainImageViews[i] };
			}


			VkExtent2D swapChainExtent = getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(
				device.device(),
				&framebufferInfo,
				nullptr,
				&swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}

	}

	void VulkanSwapChain::createUIImageViews()
	{
	}

	void VulkanSwapChain::CreateUIFramebuffers()
	{
		VkExtent2D swapChainExtent = getSwapChainExtent();
		UIframebuffers.resize(imageCount());
		for (size_t i = 0; i < imageCount(); i++)
		{
			std::array<VkImageView, 1>attachments = { colorUIImageView[i] };
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = UIRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;
			if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &UIframebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Error at UI framebuffer creation");
			}
		}

	}

	void VulkanSwapChain::createDepthResources() {
		VkFormat depthFormat = findDepthFormat();
		VkExtent2D swapChainExtent = getSwapChainExtent();

		depthImages.resize(imageCount());
		depthImageMemorys.resize(imageCount());
		depthImageViews.resize(imageCount());

		for (int i = 0; i < depthImages.size(); i++) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = device.msaaSamples;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			device.createImageWithInfo(
				imageInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImages[i],
				depthImageMemorys[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}
		}
	}

	void VulkanSwapChain::createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

		computeRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &computeRenderFinishedSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(device.device(), &fenceInfo, nullptr, &computeInFlightFences[i]) !=
				VK_SUCCESS)
			{
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}

		}
	}

	void VulkanSwapChain::CreateColorResources()
	{
		VkFormat colorFormat = swapChainImageFormat;

		CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device.msaaSamples, colorFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			colorImage, colorImageMemory);
		colorImageView = CreateImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VkFormat UIFormat = VK_FORMAT_R8G8B8A8_UNORM;
		colorUIImageView.resize(colorUIImages.size());
		for (int i = 0; i < colorUIImages.size(); i++)
		{
			colorUIImageView[i] = CreateImageView(colorUIImages[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

	}

	VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "Present mode: Mailbox" << std::endl;
				return availablePresentMode;
			}
		}

		// for (const auto &availablePresentMode : availablePresentModes) {
		//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
		//     std::cout << "Present mode: Immediate" << std::endl;
		//     return availablePresentMode;
		//   }
		// }

		std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = windowExtent;
			actualExtent.width = std::max(
				capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(
				capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	VkImageView VulkanSwapChain::CreateImageView(VkImage& image, VkFormat& format, VkImageAspectFlagBits aspectFlags, uint32_t mipLevels)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture image view! KEKW");
		}


		return imageView;
	}


	//this is a weird abstraction off the swapchain images views creation
	void VulkanSwapChain::CreateTextureImageView(VkImageView& view, VkImage& image, uint32_t mipLevels, VkFormat format)
	{
		view = CreateImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{

			swapChainImageViews[i] = CreateImageView(swapChainImages[i], format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

		}

	}

	void VulkanSwapChain::WaitForComputeFence()
	{
		vkWaitForFences(device.device(), 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(device.device(), 1, &computeInFlightFences[currentFrame]);
	}

	VkFormat VulkanSwapChain::findDepthFormat() {
		return device.findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

    void VulkanSwapChain::CreateDynamicRenderPass(VkRenderPass &renderPass) {
        VkAttachmentDescription attachment = {};
        attachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        if (vkCreateRenderPass(device.device(), &info, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Error at UI renderpass creation");
        }
    }


}  // namespace lve
