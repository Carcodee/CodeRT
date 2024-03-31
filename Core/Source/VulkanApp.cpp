
#include "VulkanApp.h"
#include <stdexcept>
#include <array>
#include "VulkanAPI/VulkanObjects/Textures/VKTexture.h"
#include <string>

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


			if (auto commandBuffer = renderer.BeginComputeFrame())
			{


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
				renderer.BeginSwapChainRenderPass(commandBuffer);
				forward_RS.pipelineReader->bind(commandBuffer);
				forward_RS.renderSystemDescriptorSetHandler->UpdateUniformBuffer<UniformBufferObjectData>(renderer.GetCurrentFrame(), 1);

				//vkCmdDraw(commandBuffer[imageIndex], 3, 1, 0, 0);
				myModel->BindVertexBufferIndexed(commandBuffer);
				myModel->BindDescriptorSet(commandBuffer, forward_RS.pipelineLayout,forward_RS.renderSystemDescriptorSetHandler->descriptorData[0].descriptorSets[renderer.GetCurrentFrame()]);
				myModel->DrawIndexed(commandBuffer);
				//TEST----


				ImGui_ImplVulkan_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
				bool showDemoWindow = true;
				if (true)
				    ImGui::ShowDemoWindow(&showDemoWindow);

				ImGui::Render();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, 0);

				//TEST---------------------

				renderer.EndSwapChainRenderPass(commandBuffer);
				renderer.EndFrame();
			}
			deltaTime = (currentTime - lastDeltaTime) * 100.0;
			lastDeltaTime = currentTime;
			cicles++;
		}

		//TEST
		ImGui_ImplVulkan_Shutdown();

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vkDeviceWaitIdle(myDevice.device());
	}
//#ifdef IS_EDITOR

	void VulkanApp::RunEngine_EDITOR(std::function<void(bool& showDemoWindow,VkCommandBuffer currentCommandBuffer)>&& editorContext)
	{


		while (!initWindow.ShouldClose())
		{
			glfwPollEvents();
			int currentTime = glfwGetTime();

			static auto newTime = std::chrono::high_resolution_clock::now();
			auto d = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(d - newTime).count();


			if (auto commandBuffer = renderer.BeginComputeFrame())
			{


				forward_RS.TransitionBeforeComputeRender(renderer.GetCurrentFrame());

				if (cicles==3)
				{
					//VkImageSubresourceRange imageSubresourceRange = {};
					//imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					//imageSubresourceRange.baseMipLevel = 0;
					//imageSubresourceRange.levelCount = 1; // Clear only one mip level
					//imageSubresourceRange.baseArrayLayer = 0;
					//imageSubresourceRange.layerCount = 1; // Assuming the image is not an array
					//const VkClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
					//vkCmdClearColorImage(commandBuffer, forward_RS.outputStorageImage->textureImage, VK_IMAGE_LAYOUT_GENERAL,&clearValue, 1,&imageSubresourceRange);
					//cicles = 0;
				}
				forward_RS.CreateComputeWorkGroups(renderer.GetCurrentFrame(), commandBuffer);
				forward_RS.UpdateUBO(renderer.GetCurrentFrame(), time);
				renderer.EndComputeFrame();
			}
			if (auto commandBuffer = renderer.BeginFrame())
			{
				forward_RS.TransitionBeforeForwardRender(renderer.GetCurrentFrame());	
				renderer.BeginSwapChainRenderPass(commandBuffer);
				forward_RS.pipelineReader->bind(commandBuffer);
				forward_RS.renderSystemDescriptorSetHandler->UpdateUniformBuffer<UniformBufferObjectData>(renderer.GetCurrentFrame(), 1);

				//vkCmdDraw(commandBuffer[imageIndex], 3, 1, 0, 0);
				myModel->BindVertexBufferIndexed(commandBuffer);
				myModel->BindDescriptorSet(commandBuffer, forward_RS.pipelineLayout,forward_RS.renderSystemDescriptorSetHandler->descriptorData[0].descriptorSets[renderer.GetCurrentFrame()]);
				myModel->DrawIndexed(commandBuffer);

				bool showDemoWindow = true;	
				editorContext(showDemoWindow,commandBuffer);
				renderer.EndSwapChainRenderPass(commandBuffer);

				renderer.EndFrame();
			}
			deltaTime = (currentTime - lastDeltaTime) * 100.0;
			lastDeltaTime = currentTime;
			cicles++;

		}
		vkDeviceWaitIdle(myDevice.device());

	}

//#endif

	VulkanApp::VulkanApp()
	{
		LoadModels();
		InitConfigsCache();

		SetUpImgui();
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
		std::string path = "C:/Users/carlo/Downloads/VikingRoom.fbx";
		VKBufferHandler* myBuffer= modelLoader->LoadModelTinyObject(path);
		
		myModel = std::make_unique<MyModel>(myDevice, *myBuffer);
		


	}

	 void VulkanApp::SetUpImgui()
        {

            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000;
            pool_info.poolSizeCount = std::size(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;

            VkDescriptorPool imguiPool;
            (vkCreateDescriptorPool(myDevice.device(), &pool_info, nullptr, &imguiPool));

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
            //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

            //io.ConfigViewportsNoAutoMerge = true;
            //io.ConfigViewportsNoTaskBarIcon = true;
            ImGuiStyle& style = ImGui::GetStyle();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }
            io.Fonts->AddFontDefault();
            // Our state
            bool show_demo_window = true;
            bool show_another_window = false;
            ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            // Setup Dear ImGui style

            io.DisplaySize = ImVec2(800, 600); // Set to actual window size
            ImGui::StyleColorsDark();

            // Setup Platform/Renderer backends
            bool result = ImGui_ImplGlfw_InitForVulkan(initWindow.window, true);
            if (result)
            {
                std::cout << "Imgui window init success" << "\n";
            }
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = VULKAN::MyVulkanDevice::g_Instance;
            init_info.PhysicalDevice = VULKAN::MyVulkanDevice::g_PhysicalDevice;
            init_info.Device = VULKAN::MyVulkanDevice::g_Device;
            init_info.QueueFamily = VULKAN::MyVulkanDevice::g_QueueFamily;
            init_info.Queue = VULKAN::MyVulkanDevice::g_Queue;
            init_info.PipelineCache = VULKAN::MyVulkanDevice::g_PipelineCache;
            init_info.DescriptorPool = imguiPool;
            init_info.RenderPass = renderer.GetSwapchainRenderPass();
            init_info.Subpass = 0;
            init_info.MinImageCount = VULKAN::MyVulkanDevice::g_MinImageCount;
            init_info.ImageCount = renderer.GetImageCount();
            init_info.MSAASamples = myDevice.msaaSamples;
            init_info.Allocator = VULKAN::MyVulkanDevice::g_Allocator;
            init_info.CheckVkResultFn = check_vk_result;
            ImGui_ImplVulkan_Init(&init_info);
            std::cout << "Hello Editor" << std::endl;
            ImGui_ImplVulkan_CreateFontsTexture();

        }



}

