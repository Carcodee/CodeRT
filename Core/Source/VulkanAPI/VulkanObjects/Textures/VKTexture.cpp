#include "VKTexture.h"
#include <stb_image.h>

namespace VULKAN {

	VKTexture::VKTexture(const char* path, VulkanSwapChain& swapchain) : mySwapChain{swapchain} , device{swapchain.device}
	{
		this->path = path;
		CreateTextureImage();
		textureImageView = mySwapChain.CreateImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT,mipLevels);
		CreateTextureSample();
		mySwapChain.device.deletionQueue.push_function([this]() {vkDestroySampler(device.device(), textureSampler, nullptr);});
		mySwapChain.device.deletionQueue.push_function([this]() {vkDestroyImageView(device.device(), textureImageView, nullptr);});
		mySwapChain.device.deletionQueue.push_function([this]() {vkDestroyImage(device.device(), textureImage, nullptr);});
		mySwapChain.device.deletionQueue.push_function([this]() {vkFreeMemory(device.device(), textureImageMemory, nullptr);});

		//myDevice.ResourceToDestroy(this);


	}

	VKTexture::VKTexture(const char* path, VulkanSwapChain& swapchain, uint32_t mipLevels) : mySwapChain{ swapchain }, device{ swapchain.device }
	{
		this->mipLevels = mipLevels;




	}

	VKTexture::VKTexture(VulkanSwapChain& swapchain) : mySwapChain{ swapchain }, device{ swapchain.device }
	{

	}



	void VKTexture::CreateTextureImage()
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
		mipLevels = (static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1);


		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		mySwapChain.device.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(mySwapChain.device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(mySwapChain.device.device(), stagingBufferMemory);

		stbi_image_free(pixels);

		mySwapChain.CreateImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			textureImage, textureImageMemory);
		mySwapChain.device.TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		mySwapChain.device.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
		vkDestroyBuffer(mySwapChain.device.device(), stagingBuffer, nullptr);
		vkFreeMemory(mySwapChain.device.device(), stagingBufferMemory, nullptr);
		mySwapChain.device.GenerateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

	}

	void VKTexture::CreateTextureSample()
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
		samplerInfo.maxLod = static_cast<float>(mipLevels);
		if (vkCreateSampler(mySwapChain.device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture sampler :/ ");
		}
	}


	void VKTexture::CreateImageViews()
	{

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = textureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;


		if (vkCreateImageView(mySwapChain.device.device(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture image view! KEKW");
		}


		
	}


}
