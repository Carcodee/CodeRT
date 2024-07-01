#include "ImguiRenderSystem.h"
#include "VulkanAPI/Utility/Utility.h"
#include <algorithm>
#include <iostream>

#include "VulkanAPI/Model/ModelHandler.h"
#include "VulkanAPI/ResourcesManagers/Assets/AssetsHandler.h"
#include "VulkanAPI/ResourcesManagers/UI/ResourcesUIHandler.h"
#include "VulkanAPI/Utility/InputSystem/InputHandler.h"


namespace VULKAN
{
	ImguiRenderSystem::~ImguiRenderSystem()
	{
		AssetsHandler::GetInstance()->~AssetsHandler();

	}

	ImguiRenderSystem::ImguiRenderSystem(VulkanRenderer& renderer, MyVulkanDevice& device ) : myRenderer(renderer) ,myDevice(device) 
	{

	}


	void ImguiRenderSystem::SetUpSystem(GLFWwindow* window)
	{
		this->myWindow= window;
		
		vertexBuffer.device = myDevice.device();
		indexBuffer.device = myDevice.device();

		InitImgui();
		CreateFonts();
		SetImgui(window);

		myRenderer.GetSwapchain().CreateImageSamples(viewportSampler, 1.0f);
		vpImageView= myRenderer.GetSwapchain().colorImageView;

		//AddSamplerAndViewForImage(viewportSampler,vpImageView);
		CreatePipelineLayout();

		//CreateImguiImage(viewportSampler, vpImageView, vpDescriptorSet);

		for (auto& image : imagesToCreate)
		{
			AddImage(image.sampler, image.imageView, image.descriptor);
		}
		CreatePipeline();

		//myDevice.deletionQueue.push_function([this]() {vertexBuffer.destroy();});
		//myDevice.deletionQueue.push_function([this]() {indexBuffer.destroy();});
        
		//for (size_t i = 0; i < myRenderer.GetMaxRenderInFlight(); i++)
		//{
		//	if (uniformBuffers.size()>0)
		//	{
		//		myDevice.deletionQueue.push_function([this, i]()
		//			{
		//				vkDestroyBuffer(myDevice.device(), uniformBuffers[i], nullptr);
		//			});
		//		myDevice.deletionQueue.push_function([this, i]()
		//			{
		//				vkFreeMemory(myDevice.device(), uniformBuffersMemory[i], nullptr);
		//			});
		//	}
		//}
		//myDevice.deletionQueue.push_function([this]()
		//	{
		//		vkDestroyDescriptorSetLayout(myDevice.device(), descriptorSetLayout, nullptr);
		//	});
		//myDevice.deletionQueue.push_function([this]()
		//	{
		//		vkDestroySampler(myDevice.device(), viewportSampler, nullptr);
		//	});
	}



	void ImguiRenderSystem::CreatePipelineLayout()
	{

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = 0;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding.pImmutableSamplers = nullptr;
		descriptorSetLayoutBindings.clear();
		descriptorSetLayoutBindings.push_back(layoutBinding);

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
		layoutInfo.pBindings = descriptorSetLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(myDevice.device(), &layoutInfo, nullptr, &descriptorSetLayout)!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		VkDeviceSize bufferSize = sizeof(UIVertex);

		uniformBuffers.resize(myRenderer.GetMaxRenderInFlight());
		uniformBuffersMemory.resize(myRenderer.GetMaxRenderInFlight());
		uniformBuffersMapped.resize(myRenderer.GetMaxRenderInFlight());
		for (size_t j = 0; j < myRenderer.GetMaxRenderInFlight(); j++)
		{
			myDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBuffers[j], uniformBuffersMemory[j]);

			vkMapMemory(myDevice.device(), uniformBuffersMemory[j], 0, bufferSize, 0, &uniformBuffersMapped[j]);

		}


		std::array<VkDescriptorPoolSize, 1> poolSize{};
		for (size_t i = 0; i < descriptorSetLayoutBindings.size(); i++)
		{
			poolSize[i].type = descriptorSetLayoutBindings[i].descriptorType;
			if (poolSize[i].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				poolSize[i].descriptorCount = static_cast<uint32_t>(myRenderer.GetMaxRenderInFlight());
				continue;
			}
			poolSize[i].descriptorCount = static_cast<uint32_t>(myRenderer.GetMaxRenderInFlight());

		}
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.maxSets = static_cast<uint32_t>(myRenderer.GetMaxRenderInFlight()*(imagesToCreate.size()+2));
		vkCreateDescriptorPool(myDevice.device(), &poolInfo, nullptr, &imguiPool);

		std::vector<VkDescriptorSetLayout> layouts(myRenderer.GetMaxRenderInFlight(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = imguiPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(myRenderer.GetMaxRenderInFlight());
		allocInfo.pSetLayouts = layouts.data();
		if (vkAllocateDescriptorSets(myDevice.device(), &allocInfo, &descriptorSets) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		//normalRenderingVP
		//if (vkAllocateDescriptorSets(myDevice.device(), &allocInfo, &vpDescriptorSet) != VK_SUCCESS)
		//{
		//	throw std::runtime_error("failed to allocate descriptor sets!");
		//}
		for (auto& descriptor : imagesToCreate)
		{
			if (vkAllocateDescriptorSets(myDevice.device(), &allocInfo, &descriptor.descriptor) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor sets!");
			}
		}
		for (size_t i = 0; i < myRenderer.GetMaxRenderInFlight(); i++)
		{
			
			std::array<VkWriteDescriptorSet, 1> descriptorWrite{};
			

			VkDescriptorImageInfo imageInfo{};
			
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = fontTexture->textureImageView;
			imageInfo.sampler = fontTexture->textureSampler;

			descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[0].dstSet = descriptorSets;
			descriptorWrite[0].dstBinding = 0;
			descriptorWrite[0].dstArrayElement = 0;
			descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite[0].descriptorCount =1;
			descriptorWrite[0].pImageInfo = &imageInfo;
			descriptorWrite[0].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(myDevice.device(), descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
		}
		


	}
	void ImguiRenderSystem::CreatePipeline()
	{
		 VkFormat format = myRenderer.GetSwapchain().getSwapChainImageFormat();
		const VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats =&format,
		};
		VkPushConstantRange pushConstantRange = {};

		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // The pipeline shader stages that will access the push constant range.
		pushConstantRange.offset = 0; // The start offset, in bytes, used to access the push constants in the specified stages.
		pushConstantRange.size = sizeof(MyPushConstBlock);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout");
		}

		assert(pipelineLayout != nullptr && "Cannot create pipeline before swapchain");
		PipelineConfigInfo pipelineConfig{};

		PipelineReader::UIPipelineDefaultConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = myRenderer.GetSwapchain().UIRenderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineReader = std::make_unique<PipelineReader>(myDevice);
		pipelineReader->CreateFlexibleGraphicPipeline<UIVertex>(
			"../Core/Source/Shaders/Imgui/imgui_shader.vert.spv",
			"../Core/Source/Shaders/Imgui/imgui_shader.frag.spv",
			pipelineConfig, UseDynamicRendering, pipeline_rendering_create_info);
		std::cout << "You are using dynamic rendering" << "\n";
	}
	void ImguiRenderSystem::CreateImguiImage(VkSampler imageSampler, VkImageView myImageView, VkDescriptorSet& descriptor)
	{

		for (size_t i = 0; i < myRenderer.GetMaxRenderInFlight(); i++)
		{

			std::array<VkWriteDescriptorSet, 1> descriptorWrite{};

			VkDescriptorImageInfo imageInfo{};

			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = myImageView;
			imageInfo.sampler = imageSampler;

			descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[0].dstSet = descriptor;
			descriptorWrite[0].dstBinding = 0;
			descriptorWrite[0].dstArrayElement = 0;
			descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite[0].descriptorCount = 1;
			descriptorWrite[0].pImageInfo = &imageInfo;
			descriptorWrite[0].pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(myDevice.device(), descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
		}

	}

	void ImguiRenderSystem::InitImgui()
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = 1.0f;
		ImGuiStyle& style = ImGui::GetStyle();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		style.ScaleAllSizes(1.0f);



	}

	void ImguiRenderSystem::SetStyle(uint32_t index)
	{

		switch (index)
		{
		case 0:
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style= vulkanStyle;
			break;
		}
		case 1:
			ImGui::StyleColorsClassic();
			break;
		case 2:
			ImGui::StyleColorsDark();
			break;
		case 3:
			ImGui::StyleColorsLight();
			break;
		case 4:

			ImGuiStyle& style = ImGui::GetStyle();
			style = minimalistStyle;
			break;
		}

	}

	void ImguiRenderSystem::CreateFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* fontData;
		int texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);
		fontTexture=new VKTexture(myRenderer.GetSwapchain());
		fontTexture->CreateImageFromSize(uploadSize, fontData, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
		fontTexture->CreateImageViews(VK_FORMAT_R8G8B8A8_UNORM);
		fontTexture->CreateTextureSample();
	}

	void ImguiRenderSystem::AddImage(VkSampler sampler, VkImageView image, VkDescriptorSet& descriptor)
	{
		CreateImguiImage(sampler, image, descriptor);
	}

	void ImguiRenderSystem::AddSamplerAndViewForImage(VkSampler sampler, VkImageView view)
	{
		VkDescriptorSet newSet{};
		ImguiImageInfo image{sampler,view, newSet};
		
		imagesToCreate.push_back(image);	
	}

	void ImguiRenderSystem::CreateStyles()
	{
		vulkanStyle = ImGui::GetStyle();
		vulkanStyle.FrameRounding = 3.0f;
		vulkanStyle.WindowPadding = ImVec2(8.00f, 8.00f);
		vulkanStyle.FramePadding = ImVec2(5.00f, 2.00f);
		vulkanStyle.CellPadding = ImVec2(6.00f, 6.00f);
		vulkanStyle.ItemSpacing = ImVec2(6.00f, 6.00f);
		vulkanStyle.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		vulkanStyle.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		vulkanStyle.IndentSpacing = 25;
		vulkanStyle.ScrollbarSize = 15;
		vulkanStyle.GrabMinSize = 10;
		vulkanStyle.WindowBorderSize = 1;
		vulkanStyle.ChildBorderSize = 1;
		vulkanStyle.PopupBorderSize = 1;
		vulkanStyle.FrameBorderSize = 1;
		vulkanStyle.TabBorderSize = 1;
		vulkanStyle.WindowRounding = 7;
		vulkanStyle.ChildRounding = 4;
		vulkanStyle.FrameRounding = 3;
		vulkanStyle.PopupRounding = 4;
		vulkanStyle.ScrollbarRounding = 9;
		vulkanStyle.GrabRounding = 3;
		vulkanStyle.LogSliderDeadzone = 4;
		vulkanStyle.TabRounding = 4;

		vulkanStyle.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		vulkanStyle.Colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
		vulkanStyle.Colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		vulkanStyle.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
		vulkanStyle.Colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		vulkanStyle.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		vulkanStyle.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
		vulkanStyle.Colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		vulkanStyle.Colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		vulkanStyle.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		vulkanStyle.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		vulkanStyle.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		vulkanStyle.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		vulkanStyle.Colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
		vulkanStyle.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		vulkanStyle.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		vulkanStyle.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		vulkanStyle.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		vulkanStyle.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		vulkanStyle.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		vulkanStyle.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		vulkanStyle.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		vulkanStyle.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		vulkanStyle.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);



		minimalistStyle = ImGui::GetStyle();
		minimalistStyle.Alpha = 1.0f;
		minimalistStyle.FrameRounding = 3.0f;
		minimalistStyle.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
		minimalistStyle.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
		minimalistStyle.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		minimalistStyle.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		minimalistStyle.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
		minimalistStyle.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		minimalistStyle.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		minimalistStyle.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		minimalistStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		minimalistStyle.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		minimalistStyle.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		minimalistStyle.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		minimalistStyle.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
		minimalistStyle.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		minimalistStyle.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		minimalistStyle.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		minimalistStyle.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

		for (int i = 0; i <= ImGuiCol_COUNT; i++)
		{
			ImVec4& col = minimalistStyle.Colors[i];
			float H, S, V;
			ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

			if (S < 0.1f)
			{
				V = 1.0f - V;
			}
			ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
			if (col.w < 1.00f)
			{
				col.w *= 0.3;
			}
		}


	}

	void ImguiRenderSystem::DrawFrame(VkCommandBuffer commandBuffer)
	{

		pipelineReader->bind(commandBuffer);
		ImGuiIO& io = ImGui::GetIO();
		VkViewport viewport = INITIALIZERS::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		myPushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		myPushConstBlock.translate = glm::vec2( - 1.0f);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MyPushConstBlock), &myPushConstBlock);
		ImDrawData* imDrawData = ImGui::GetDrawData();

		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if (imDrawData->CmdListsCount > 0) {

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x =std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y =std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
	
					//myDevice.TransitionImageLayout(myRenderer.GetSwapchain().colorImage, myRenderer.GetSwapchain().getSwapChainImageFormat(), 1,
					//	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

					VkDescriptorSet currentSet= static_cast<VkDescriptorSet>(pcmd->TextureId);
					
					if (currentSet != nullptr)
					{
						
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &currentSet, 0, nullptr);
					}
					else
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
					}
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;

				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}
	}

	void ImguiRenderSystem::UpdateBuffers()
	{
		ImDrawData* imDrawData = ImGui::GetDrawData();

		// Note: Alignment is done inside buffer creation
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return;
		}
		if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
			vertexBuffer.unmap();
			vertexBuffer.destroy();
			myDevice.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer.buffer, vertexBuffer.memory);

			vertexCount = imDrawData->TotalVtxCount;
			vertexBuffer.map();
		}
		if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
			indexBuffer.unmap();
			indexBuffer.destroy();
			myDevice.createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuffer.buffer, indexBuffer.memory);
			indexCount = imDrawData->TotalIdxCount;
			indexBuffer.map();
		}
		// Upload data
		ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		vertexBuffer.flush();
		indexBuffer.flush();
	}

	void ImguiRenderSystem::BeginFrame()
	{
		ImGui::NewFrame();

		// Start the Dear ImGui frame
		int width = 0;
		int height = 0;
		glfwGetWindowSize(myWindow,&width,&height);
		ImGui::SetWindowSize(ImVec2(width, height));
		ImGui::Begin("DockSpace Demo", nullptr, ImGuiWindowFlags_MenuBar  | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
		ImVec2 viewportSize=ImGui::GetContentRegionAvail();
		//ImGui::Image((ImTextureID)vpDescriptorSet, ImVec2(viewportSize.x, viewportSize.y));
		for (auto& imageDescriptorSet : imagesToCreate)
		{
			ImGui::Image((ImTextureID)imageDescriptorSet.descriptor, ImVec2(viewportSize.x, viewportSize.y));
		}

		ResourcesUIHandler::GetInstance()->DisplayDirInfo();

		ImGui::End(); 
		// Make the window full-screen and set the dock space

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::SliderFloat("Speed", &RotationSpeed, 0.0f, 10.0f, "%.3f");
		ImGui::SliderFloat3("ModelCam Pos", modelCamPos, -10.0f, 10.0f, "%.3f");
		ImGui::LabelText("Raytracing", "");
		ImGui::SliderFloat3("Rt Cam Pos", camPos, -10.0f, 10.0f, "%.3f");
		ImGui::LabelText("Light", "");
		ImGui::SliderFloat3("light Pos", lightPos, -50.0f, 50.0f, "%.3f");
		ImGui::ColorEdit3("light Col", lightCol, 0.0f);
		ImGui::SliderFloat("light Intensity", &lightIntensity, 0.0f,20.0f,"%.3f");
		ImGui::InputText("Import a model from path:", modelImporterText,IM_ARRAYSIZE(modelImporterText));

		if (ImGui::Button("Confirm"))
		{
			ModelHandler::GetInstance()->queryModelPathsToHandle.push_back(modelImporterText);
		}

		ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background



		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		//if (ImGui::Begin("DockSpace Demo", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking))
		//{
		//	ImGui::DockSpace(ImGui::GetID("MyDockSpace"));
		//}
		//	ImGui::End(); 


	}

	void ImguiRenderSystem::WasWindowResized()
	{
		if (myRenderer.GetSwapchain().colorImageView!=vpImageView)
		{
			SetUpSystem(myWindow);
			
		}
	}

	void ImguiRenderSystem::EndFrame()
	{
		ImGui::Render();

	}


	void ImguiRenderSystem::SetImgui(GLFWwindow* window)
	{
		// Color scheme
		CreateStyles();
		SetStyle(0);
		// Setup display size (every frame to accommodate for window resizing)
		int w, h;
		int display_w, display_h;
		glfwGetWindowSize(window, &w, &h);
		glfwGetFramebufferSize(window, &display_w, &display_h);

		// Dimensions
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)w, (float)h);
		io.DisplayFramebufferScale = ImVec2((float)display_w / (float)w, (float)display_h / (float)h);

		// If we directly work with os specific key codes, we need to map special key types like tab
		//io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		//io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		//io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		//io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		//io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		//io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		//io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		//io.KeyMap[ImGuiKey_Space] = VK_SPACE;
		//io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	}

}
