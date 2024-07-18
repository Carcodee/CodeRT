
#include "VulkanApp.h"
#include <stdexcept>
#include <array>
#include "VulkanAPI/VulkanObjects/Textures/VKTexture.h"
#include <string>

#include "VulkanAPI/Model/ModelHandler.h"
#include "VulkanAPI/ResourcesManagers/Assets/AssetsHandler.h"
#include "VulkanAPI/Utility/InputSystem/InputHandler.h"

int cicles=0;
namespace VULKAN{
	
	void VulkanApp::Run()
	{

		while (!initWindow.ShouldClose())
		{
			glfwPollEvents();
			int currentTime = glfwGetTime();

			static auto newTime = std::chrono::high_resolution_clock::now();
			auto d = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(d - newTime).count();
			rayTracing_RS.cam.position.x = ImguiRenderSystem::GetInstance()->camPos[0];
			rayTracing_RS.cam.position.y = ImguiRenderSystem::GetInstance()->camPos[1];
			rayTracing_RS.cam.position.z = ImguiRenderSystem::GetInstance()->camPos[2];
			rayTracing_RS.cam.UpdateCamera();


			forward_RS.renderSystemDescriptorSetHandler->cam.position.x = ImguiRenderSystem::GetInstance()->modelCamPos[0];
			forward_RS.renderSystemDescriptorSetHandler->cam.position.y = ImguiRenderSystem::GetInstance()->modelCamPos[1];
			forward_RS.renderSystemDescriptorSetHandler->cam.position.z = ImguiRenderSystem::GetInstance()->modelCamPos[2];

			if (auto commandBuffer = renderer.BeginComputeFrame())
			{

				rayTracing_RS.DrawRT(commandBuffer);

				forward_RS.TransitionBeforeComputeRender(renderer.GetCurrentFrame());

				if (cicles == 3)
				{
					VkImageSubresourceRange imageSubresourceRange = {};
					imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageSubresourceRange.baseMipLevel = 0;
					imageSubresourceRange.levelCount = 1; // Clear only one mip level
					imageSubresourceRange.baseArrayLayer = 0;
					imageSubresourceRange.layerCount = 1; // Assuming the image is not an array
					const VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
					vkCmdClearColorImage(commandBuffer, forward_RS.outputStorageImage->textureImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &imageSubresourceRange);
					cicles = 0;
				}
				forward_RS.CreateComputeWorkGroups(renderer.GetCurrentFrame(), commandBuffer);
				forward_RS.UpdateUBO(renderer.GetCurrentFrame(), time);
				renderer.EndComputeFrame();
			}
			if (auto commandBuffer = renderer.BeginFrame())
			{
				forward_RS.TransitionBeforeForwardRender(renderer.GetCurrentFrame());

				rayTracing_RS.TransitionStorageImage();

				renderer.BeginSwapChainRenderPass(commandBuffer);
				forward_RS.pipelineReader->bind(commandBuffer);
				forward_RS.renderSystemDescriptorSetHandler->UpdateUniformBuffer<UniformBufferObjectData>(renderer.GetCurrentFrame(), 1, ImguiRenderSystem::GetInstance()->RotationSpeed);
				//vkCmdDraw(commandBuffer[imageIndex], 3, 1, 0, 0);
				renderer.EndSwapChainRenderPass(commandBuffer);

				renderer.EndFrame();
			}
			deltaTime = (currentTime - lastDeltaTime) * 100.0;
			lastDeltaTime = currentTime;
			cicles++;

		}
		vkDeviceWaitIdle(myDevice.device());
	}
//#ifdef IS_EDITOR

	void VulkanApp::RunEngine_EDITOR(std::function<void()>&& editorContext)
	{


		while (!initWindow.ShouldClose())
		{
			glfwPollEvents();
			int currentTime = glfwGetTime();

			static auto newTime = std::chrono::high_resolution_clock::now();
			auto d = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(d - newTime).count();
			rayTracing_RS.cam.position.x = ImguiRenderSystem::GetInstance()->camPos[0];
			rayTracing_RS.cam.position.y = ImguiRenderSystem::GetInstance()->camPos[1];
			rayTracing_RS.cam.position.z = ImguiRenderSystem::GetInstance()->camPos[2];
			rayTracing_RS.cam.UpdateCamera();


			forward_RS.renderSystemDescriptorSetHandler->cam.position.x = ImguiRenderSystem::GetInstance()->modelCamPos[0];
			forward_RS.renderSystemDescriptorSetHandler->cam.position.y = ImguiRenderSystem::GetInstance()->modelCamPos[1];
			forward_RS.renderSystemDescriptorSetHandler->cam.position.z = ImguiRenderSystem::GetInstance()->modelCamPos[2];

			if (auto commandBuffer = renderer.BeginComputeFrame())
			{

				rayTracing_RS.DrawRT(commandBuffer);
				
				forward_RS.TransitionBeforeComputeRender(renderer.GetCurrentFrame());

				if (cicles==3)
				{
					VkImageSubresourceRange imageSubresourceRange = {};
					imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageSubresourceRange.baseMipLevel = 0;
					imageSubresourceRange.levelCount = 1; // Clear only one mip level
					imageSubresourceRange.baseArrayLayer = 0;
					imageSubresourceRange.layerCount = 1; // Assuming the image is not an array
					const VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
					vkCmdClearColorImage(commandBuffer, forward_RS.outputStorageImage->textureImage, VK_IMAGE_LAYOUT_GENERAL,&clearValue, 1,&imageSubresourceRange);
					cicles = 0;
				}
				forward_RS.CreateComputeWorkGroups(renderer.GetCurrentFrame(), commandBuffer);
				forward_RS.UpdateUBO(renderer.GetCurrentFrame(), time);
				renderer.EndComputeFrame();
			}
			if (auto commandBuffer = renderer.BeginFrame())
			{
				forward_RS.TransitionBeforeForwardRender(renderer.GetCurrentFrame());	

				rayTracing_RS.TransitionStorageImage();

				renderer.BeginSwapChainRenderPass(commandBuffer);
				forward_RS.pipelineReader->bind(commandBuffer);
				forward_RS.renderSystemDescriptorSetHandler->UpdateUniformBuffer<UniformBufferObjectData>(renderer.GetCurrentFrame(), 1, ImguiRenderSystem::GetInstance()->RotationSpeed);

				//vkCmdDraw(commandBuffer[imageIndex], 3, 1, 0, 0);
				renderer.EndSwapChainRenderPass(commandBuffer);


				VkMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // Adjust based on your needs
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;  // Adjust based on your needs

				vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // Source stage
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,         // Destination stage
					0,                                             // No flags
					1, &barrier,                                   // Memory barriers
					0, nullptr,                                    // Buffer barriers
					0, nullptr                                     // Image barriers
				);

				ImguiRenderSystem::GetInstance()->WasWindowResized();
				renderer.BeginUIRenderPass(commandBuffer);
				ImguiRenderSystem::GetInstance()->BeginFrame();
				editorContext();
				ImguiRenderSystem::GetInstance()->EndFrame();
				ImguiRenderSystem::GetInstance()->UpdateBuffers();
				ImguiRenderSystem::GetInstance()->DrawFrame(commandBuffer);
				renderer.EndUIRenderPass(commandBuffer);

				renderer.EndFrame();
			}
			deltaTime = (currentTime - lastDeltaTime) * 100.0;
			lastDeltaTime = currentTime;
			cicles++;

		}
		vkDeviceWaitIdle(myDevice.device());

	}

	void VulkanApp::RunDynamicRendering(std::function<void()>&& editorContext)
	{
		while (!initWindow.ShouldClose())
		{
			glfwPollEvents();
			float currentTime = glfwGetTime();

			static auto newTime = std::chrono::high_resolution_clock::now();
			auto d = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(d - newTime).count();
			//rayTracing_RS.cam.position.x = ImguiRenderSystem::GetInstance()->camPos[0];
			//rayTracing_RS.cam.position.y = ImguiRenderSystem::GetInstance()->camPos[1];
			//rayTracing_RS.cam.position.z = ImguiRenderSystem::GetInstance()->camPos[2];
			rayTracing_RS.light.color = glm::make_vec3(ImguiRenderSystem::GetInstance()->lightCol);
			rayTracing_RS.light.pos = glm::make_vec3(ImguiRenderSystem::GetInstance()->lightPos);
			rayTracing_RS.light.intensity = ImguiRenderSystem::GetInstance()->lightIntensity;

			rayTracing_RS.cam.Move(deltaTime);
			rayTracing_RS.cam.UpdateCamera();

            if (ModelHandler::GetInstance()->updateMaterialData){
                rayTracing_RS.UpdateMaterialInfo();
                ModelHandler::GetInstance()->updateMaterialData = false;
            }
            if (ModelHandler::GetInstance()->updateMeshData){
                rayTracing_RS.UpdateMeshInfo();
                ModelHandler::GetInstance()->updateMeshData = false;
            }

            LoadQueryModels();
            
			if (auto commandBuffer = renderer.BeginComputeFrame())
			{

				rayTracing_RS.DrawRT(commandBuffer);
				
				forward_RS.TransitionBeforeComputeRender(renderer.GetCurrentFrame());

				if (cicles==3)
				{
					VkImageSubresourceRange imageSubresourceRange = {};
					imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageSubresourceRange.baseMipLevel = 0;
					imageSubresourceRange.levelCount = 1; // Clear only one mip level
					imageSubresourceRange.baseArrayLayer = 0;
					imageSubresourceRange.layerCount = 1; // Assuming the image is not an array
					const VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
					vkCmdClearColorImage(commandBuffer, forward_RS.outputStorageImage->textureImage, VK_IMAGE_LAYOUT_GENERAL,&clearValue, 1,&imageSubresourceRange);
					cicles = 0;
				}
				forward_RS.CreateComputeWorkGroups(renderer.GetCurrentFrame(), commandBuffer);
				forward_RS.UpdateUBO(renderer.GetCurrentFrame(), time);
				renderer.EndComputeFrame();
			}
			if (auto commandBuffer = renderer.BeginFrame())
			{
				forward_RS.TransitionBeforeForwardRender(renderer.GetCurrentFrame());

				rayTracing_RS.TransitionStorageImage();

				renderer.BeginSwapChainRenderPass(commandBuffer);
				forward_RS.pipelineReader->bind(commandBuffer);
				forward_RS.renderSystemDescriptorSetHandler->UpdateUniformBuffer<UniformBufferObjectData>(renderer.GetCurrentFrame(), 1, ImguiRenderSystem::GetInstance()->RotationSpeed);

				renderer.EndSwapChainRenderPass(commandBuffer);

				VkMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // Adjust based on your needs
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;  // Adjust based on your needs

				vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // Source stage
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,         // Destination stage
					0,                                             // No flags
					1, &barrier,                                   // Memory barriers
					0, nullptr,                                    // Buffer barriers
					0, nullptr                                     // Image barriers
				);


			    VkClearValue clearValue{};
				clearValue= { 0.1f, 0.1f, 0.1f, 0.1f };
			
				VkRenderingInfo renderingInfo{};
				VkRenderingAttachmentInfo colorAttachmentInfo={};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				colorAttachmentInfo.imageView = renderer.GetSwapchain().colorUIImageView[renderer.currentImageIndex];
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearValue;



				renderingInfo.colorAttachmentCount = 1;
				renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
				renderingInfo.layerCount = 1;
				renderingInfo.colorAttachmentCount = 1;
				renderingInfo.pColorAttachments = &colorAttachmentInfo;
				renderingInfo.renderArea.offset = { 0,0 };
				renderingInfo.renderArea.extent = renderer.GetSwapchain().getSwapChainExtent();


				ImguiRenderSystem::GetInstance()->WasWindowResized();
				renderer.BeginDynamicRenderPass(commandBuffer, renderingInfo);
				ImguiRenderSystem::GetInstance()->BeginFrame();

				
				editorContext();
				ImguiRenderSystem::GetInstance()->EndFrame();
				ImguiRenderSystem::GetInstance()->UpdateBuffers();
				ImguiRenderSystem::GetInstance()->DrawFrame(commandBuffer);
				renderer.EndDynamicRenderPass(commandBuffer);

				//renderer.GetSwapchain().HandleColorImage(renderer.GetSwapchain().colorImage,
				//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, commandBuffer,
				//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				//	 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
				renderer.GetSwapchain().HandleColorImage(renderer.GetSwapchain().colorUIImages[renderer.currentImageIndex],
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, commandBuffer,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

				renderer.EndFrame();


			}
			deltaTime = (currentTime - lastDeltaTime);
			lastDeltaTime = currentTime;
			cicles++;
			AssetsHandler::GetInstance()->RegisterSaves();


			InputHandler::GetInstance()->UpdateInputStates();


		}
		vkDeviceWaitIdle(myDevice.device());

	}

	//#endif

	VulkanApp::VulkanApp(bool DynamicRendering, bool editor)
	{

        std::cout << "Present mode: currentWorkingDir: "<< std::filesystem::current_path()  << std::endl;
		rayTracing_RS.Create_RT_RenderSystem();
		forward_RS.raytracingImage = rayTracing_RS.storageImage;
		forward_RS.InitForwardSystem();
		LoadModels();
		InitConfigsCache();
        ImguiRenderSystem::GetInstance(&renderer, &myDevice);
		if (editor)
		{
			SetUpImgui();
			ImguiRenderSystem::GetInstance()->UseDynamicRendering = DynamicRendering;
//			ImguiRenderSystem::GetInstance()->AddSamplerAndViewForImage(rayTracing_RS.storageImage->textureSampler, rayTracing_RS.storageImage->textureImageView);
			ImguiRenderSystem::GetInstance()->SetUpSystem(initWindow.window);
            
            ImguiRenderSystem::GetInstance()->AddTexture(rayTracing_RS.storageImage);
            ImguiRenderSystem::GetInstance()->viewportTexture = rayTracing_RS.storageImage;
            
		}

		
	}

	VulkanApp::~VulkanApp()
	{

	}

	void VulkanApp::InitConfigsCache()
	{
    
		MyVulkanDevice::g_Instance=myDevice.instance;
		MyVulkanDevice::g_PhysicalDevice=myDevice.physicalDevice;
		MyVulkanDevice::g_Device=myDevice.device();
		MyVulkanDevice::g_QueueFamily=myDevice.findPhysicalQueueFamilies().graphicsAndComputeFamily;
		MyVulkanDevice::g_Queue=myDevice.graphicsQueue_;
		MyVulkanDevice::g_DescriptorPool=forward_RS.renderSystemDescriptorSetHandler->descriptorPool;
//		MyVulkanDevice::g_DescriptorPool=imguiDescriptorPool;
		MyVulkanDevice::g_MinImageCount=2;
		MyVulkanDevice::g_SwapChainRebuild = false;

	}


	void VulkanApp::LoadModels()
	{

		//const std::vector<Vertex> vertices= {
		//	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		//	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		//	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		//	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		//	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		//	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		//	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		//	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
		//};
		//const std::vector<uint16_t> indices = {
		//		0, 1, 2, 2, 3, 0,
		//		4, 5, 6, 6, 7, 4
		//};
		

	}

	 void VulkanApp::SetUpImgui()
        {

                // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
            //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

            // Setup Platform/Renderer backends
            bool result = ImGui_ImplGlfw_InitForVulkan(initWindow.window, true);
            if (result)
            {
                std::cout << "Imgui window init success" << "\n";
            }

        }

    void VulkanApp::LoadQueryModels() {

        if (!ModelHandler::GetInstance()->queryModelIdsToHandle.empty())
        {

            ModelHandler::GetInstance()->LoadAllModelsFromDisc();
            for (auto element :ModelHandler::GetInstance()->modelsReady)
            {
                //std::shared_ptr<ModelHandler::ModelToLoadState> current = element;
                if (element.get()->state== ModelHandler::UNLOADED || element.get()->state== ModelHandler::DISPACHED)
                {
                    continue;
                }

                rayTracing_RS.AddModelToPipeline(*element.get()->model);
                element.get()->state = ModelHandler::DISPACHED;
            }
            if (ModelHandler::GetInstance()->modelsReady.size() == ModelHandler::GetInstance()->queryModelIdsToHandle.size())
            {
                ModelHandler::GetInstance()->queryModelIdsToHandle.clear();
                ModelHandler::GetInstance()->modelsReady.clear();
                ModelHandler::GetInstance()->Loading = false;
            }
            if (rayTracing_RS.updateDescriptorData)
            {
                rayTracing_RS.UpdateRaytracingData();
                rayTracing_RS.updateDescriptorData = false;
            }

        }
        if (!ModelHandler::GetInstance()->queryModelPathsToHandle.empty())
        {
            ModelHandler::GetInstance()->LoadAllModels();
            for (auto element :ModelHandler::GetInstance()->modelsReady)
            {
                //std::shared_ptr<ModelHandler::ModelToLoadState> current = element;
                if (element.get()->state== ModelHandler::UNLOADED || element.get()->state== ModelHandler::DISPACHED)
                {
                    continue;
                }

                rayTracing_RS.AddModelToPipeline(*element.get()->model);
                element.get()->state = ModelHandler::DISPACHED;
            }
            if (ModelHandler::GetInstance()->modelsReady.size() == ModelHandler::GetInstance()->queryModelPathsToHandle.size())
            {
                ModelHandler::GetInstance()->queryModelPathsToHandle.clear();
                ModelHandler::GetInstance()->modelsReady.clear();
                ModelHandler::GetInstance()->Loading = false;
            }
            if (rayTracing_RS.updateDescriptorData)
            {
                rayTracing_RS.UpdateRaytracingData();
                rayTracing_RS.updateDescriptorData = false;
            }
        }       
    }


}

