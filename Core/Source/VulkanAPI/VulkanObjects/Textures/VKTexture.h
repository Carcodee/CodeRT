#pragma once
#include "VulkanAPI/VulkanInit/VulkanInit.h"
#include "VulkanAPI/SwapChain/VulkanSwap_chain.hpp"

namespace VULKAN {


	class VKTexture : public IResource
	{
	public:

		VKTexture(const char* path, VulkanSwapChain& swapchain, bool addShaderId = false);
        ~VKTexture() override;
		//VKTexture(VKTexture& other);
		//Resources handled by the swapchain;
		VKTexture(VulkanSwapChain& swapchain, uint32_t width, uint32_t height, VkImageLayout newLayout,VkAccessFlags dstAccessMask,VkPipelineStageFlags stageFlags, VkFormat format, bool addShaderId = false);
		VKTexture(VulkanSwapChain& swapchain, bool addShaderId = false);

		VKTexture& operator=(const VKTexture& other) = delete;

		void DestroyResource() const override{
			
			vkDestroySampler(mySwapChain.device.device(), textureSampler, nullptr);
			vkDestroyImageView(mySwapChain.device.device(), textureImageView, nullptr);
			vkDestroyImage(mySwapChain.device.device(), textureImage, nullptr);
			vkFreeMemory(mySwapChain.device.device(), textureImageMemory, nullptr);
		
		}
		void CreateStorageImage(uint32_t width, uint32_t height, VkImageLayout newLayout,VkAccessFlags dstAccessMask,VkPipelineStageFlags stageFlags,VkFormat format);
		void CreateImageFromSize(VkDeviceSize size,unsigned char* fontsData ,uint32_t width, uint32_t height, VkFormat format);
		void CreateTextureImage();

		void CreateTextureSample();

		void CreateImageViews(VkFormat format);
		void CreateImageViews();
        void TransitionTexture(VkImageLayout newLayout, VkAccessFlags dstAccessFlags, VkPipelineStageFlags dstStage, VkCommandBuffer& commandBuffer);
        void TransitionTexture(VkImageLayout newLayout, VkAccessFlags dstAccessFlags, VkPipelineStageFlags dstStage);
        
        int id= -1;
        VkImage textureImage=nullptr;
		VkSampler textureSampler= nullptr;
		VkImageView textureImageView= nullptr;
		VkDeviceMemory textureImageMemory=nullptr;
        VkDescriptorSet textureDescriptor = nullptr;
        
        //this means is not in the pool

	private:
        
		VulkanSwapChain& mySwapChain;
		MyVulkanDevice& device;

        VkFormat format= VK_FORMAT_R8G8B8A8_SRGB;
        VkPipelineStageFlags currentStage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkAccessFlags currentAccessFlags=0;
        VkImageLayout currentLayout= VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t mipLevels=0;
		const char* path=nullptr;

	};

}