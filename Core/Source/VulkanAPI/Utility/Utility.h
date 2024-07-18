#pragma once


#ifdef false
#define LOG() std::cout
#else
#define LOG() while(false) std::cout
#endif




#include <filesystem>
#include <map>

#include "VulkanAPI/Model/MyModel.h"
#include "VulkanAPI/VulkanObjects/Textures/VKTexture.h"
#include "ISerializable.h"


namespace VULKAN{


		struct SimpleVertex
		{
			glm::vec3 pos;
			glm::vec2 textCoord;
		};

		struct MaterialUniformData
		{
			float albedoIntensity;
			float normalIntensity;
			float specularIntensity;
            float roughnessIntensity = 0.5f;
			glm::vec3 diffuseColor;
			float reflectivityIntensity = 0.5f;
            //32
            glm::vec3 baseReflection;
            float metallicIntensity;
            //48
            float emissionIntensity;
            int roughnessOffset = -1;
            int metallicOffset = -1;
            int specularOffset = -1;
            //64
			int textureIndexStart = -1;
			int texturesSizes = 0;
            int diffuseOffset = -1;
            int normalOffset = -1;
            //80
		};

		struct ModelDataUniformBuffer 
		{
			uint32_t materialIndex;
			uint32_t geometryIndexStart;
            uint32_t indexOffset;
		};
		struct Material: ISerializable<Material>
		{
			MaterialUniformData materialUniform{};
			std::vector<std::string> paths;
			std::vector<VKTexture*> materialTextures;
            std::string materialReferencePath="";
            std::string name="";
            std::string targetPath="";
            bool generated = false;
            int id= 0;
			void CreateTextures(VulkanSwapChain& swap_chain, int& allTexturesOffset)
			{
                std::cout<<"MaterialTexturesAlready created for: "<<name<<"\n";
                if(generated)return;
				materialUniform.textureIndexStart = allTexturesOffset;
				for (int i = 0; i < paths.size(); ++i)
				{
					if (!std::filesystem::exists(paths[i].c_str()))continue;
					materialUniform.texturesSizes++;
					VKTexture* texture = new VKTexture(paths[i].c_str(), swap_chain);
					materialTextures.push_back(texture);
                    allTexturesOffset++;
				}
                generated = true;
                std::cout<<" New Texture sizes: "<<allTexturesOffset <<"\n"; 
			}
            nlohmann::json Serialize() override{
                nlohmann::json jsonData;
                std::vector<float> diffuse;
                diffuse.push_back(materialUniform.diffuseColor.x);
                diffuse.push_back(materialUniform.diffuseColor.y);
                diffuse.push_back(materialUniform.diffuseColor.z);
                std::vector<float> baseReflection;
                baseReflection.push_back(materialUniform.baseReflection.x);
                baseReflection.push_back(materialUniform.baseReflection.y);
                baseReflection.push_back(materialUniform.baseReflection.z);

                jsonData = {
                        {"ID",this->id},
                        {"Name",this->name},
                        {"AlbedoIntensity",this->materialUniform.albedoIntensity},
                        {"NormalIntensity",this->materialUniform.normalIntensity},
                        {"SpecularIntensity",this->materialUniform.specularIntensity},
                        {"RoughnessIntensity",this->materialUniform.roughnessIntensity},
                        //16
                        {"DiffuseColor",diffuse},
                        {"ReflectivityIntensity", this->materialUniform.reflectivityIntensity},
                        //32
                        {"BaseReflection",baseReflection},
                        {"MetallicIntensity",this->materialUniform.metallicIntensity},
                        //48
                        {"EmissionIntensity",this->materialUniform.emissionIntensity},
                        {"RoughnessOffset",this->materialUniform.roughnessOffset},
                        {"MetallicOffset",this->materialUniform.metallicOffset},
                        {"SpecularOffset",this->materialUniform.specularOffset},
                        //60
                        {"TextureIndexStart",this->materialUniform.textureIndexStart},
                        {"TextureSizes",this->materialUniform.texturesSizes},
                        {"DiffuseOffset",this->materialUniform.diffuseOffset},
                        {"NormalOffset",this->materialUniform.normalOffset},
                        //80
                        
                        
                        
                        {"Paths",this->paths},
                        {"MaterialReferencePath",this->materialReferencePath},
                        {"TargetPath",this->targetPath}
                };
                return jsonData;
//
            }
            Material Deserialize(nlohmann::json &jsonObj) override{

                this->id = jsonObj.at("ID");
                this->name = jsonObj.at("Name");
                std::vector<int> diffuse;
                std::vector<int> baseReflection;
                this->materialUniform.albedoIntensity = jsonObj.at("AlbedoIntensity");
                this->materialUniform.normalIntensity = jsonObj.at("NormalIntensity");
                this->materialUniform.specularIntensity = jsonObj.at("SpecularIntensity");
                this->materialUniform.roughnessIntensity = jsonObj.at("RoughnessIntensity");
                //16
                diffuse = jsonObj.at("DiffuseColor").get<std::vector<int>>();
                this->materialUniform.diffuseColor.x = diffuse[0];
                this->materialUniform.diffuseColor.y = diffuse[1];
                this->materialUniform.diffuseColor.z = diffuse[2];
                this->materialUniform.reflectivityIntensity = jsonObj.at("ReflectivityIntensity");
                //32

                //base reflection
                baseReflection = jsonObj.at("BaseReflection").get<std::vector<int>>();
                this->materialUniform.baseReflection.x = baseReflection[0];
                this->materialUniform.baseReflection.y = baseReflection[1];
                this->materialUniform.baseReflection.z = baseReflection[2];
                this->materialUniform.metallicIntensity=jsonObj.at("MetallicIntensity");
                //48
                this->materialUniform.emissionIntensity==jsonObj.at("EmissionIntensity");
                this->materialUniform.roughnessOffset = jsonObj.at("RoughnessOffset");
                this->materialUniform.metallicOffset = jsonObj.at("MetallicOffset");
                this->materialUniform.specularOffset= jsonObj.at("SpecularOffset");
                //60
                this->materialUniform.textureIndexStart=jsonObj.at("TextureIndexStart");
                this->materialUniform.texturesSizes = 0;
//                this->materialUniform.texturesSizes = jsonObj.at("TextureSizes");
                this->materialUniform.diffuseOffset = jsonObj.at("DiffuseOffset");
                this->materialUniform.normalOffset = jsonObj.at("NormalOffset");
                //80
                
                this->paths = jsonObj.at("Paths").get<std::vector<std::string>>();
                this->materialReferencePath = jsonObj.at("MaterialReferencePath");
                this->targetPath = jsonObj.at("TargetPath");
                return *this;
            }
            void SaveData() override{
                
            }
		};

		struct ModelData : ISerializable<ModelData>
		{
			std::vector <Vertex> vertices;
			std::vector<uint32_t> indices;

            std::vector<uint32_t> firstMeshIndex;
			std::vector<uint32_t> firstMeshVertex;
			std::vector<int> materialIds;
			std::vector<uint32_t> meshIndexCount;
			std::vector<uint32_t> meshVertexCount;
			std::map<int,Material>materialDataPerMesh;
			int meshCount;
			uint32_t indexBLASOffset = 0;
			uint32_t vertexBLASOffset = 0;
			uint32_t transformBLASOffset = 0;
            std::vector<ModelDataUniformBuffer> dataUniformBuffer;
            uint32_t id;
            bool generated= false;
            uint32_t materialOffset= 0;
            std::string pathToAssetReference="";
            void CalculateMaterialsIdsOffsets(){
                for (int i = 0; i < dataUniformBuffer.size(); ++i) {
                    dataUniformBuffer[i].materialIndex = materialIds[i];
                }
            }
			void CreateAllTextures(VulkanSwapChain& swap_chain, int& allTextureOffset)
			{
				for (int i = 0; i < materialDataPerMesh.size(); ++i)
				{
					materialDataPerMesh.at(i).CreateTextures(swap_chain,allTextureOffset);
				}
			}
            nlohmann::json Serialize() override{
                nlohmann::json jsonData;
                jsonData = {
                        {
                                "ModelID",this->id
                        },
                        {
                                "MeshCount",this->meshCount
                        },
                        {
                                "Generated",this->generated
                        },
                        {
                                "MaterialsIDs",this->materialIds
                        },
                        {
                                "PathToAssetReference",this->pathToAssetReference
                        },
                        {
                                "MaterialOffset",this->materialOffset
                        },
                };
                return jsonData;
            }
            ModelData Deserialize(nlohmann::json &jsonObj) override{
                this->id = jsonObj.at("ModelID");
                this->meshCount = jsonObj.at("MeshCount");
                this->generated=jsonObj.at("Generated");
                this->materialIds = jsonObj.at("MaterialsIDs").get<std::vector<int>>();
                this->pathToAssetReference = jsonObj.at("PathToAssetReference");
                this->materialOffset = jsonObj.at("MaterialOffset");
                return *this; 
            }
            void SaveData() override{
                
            }
		};
		

		uint32_t alignedSize(uint32_t value, uint32_t alignment);
		size_t alignedSize(size_t value, size_t alignment);
		VkDeviceSize alignedVkSize(VkDeviceSize value, VkDeviceSize alignment);

		struct DescriptorData
		{
			VkDescriptorSet descriptor;
			VkDescriptorSetLayout descriptorSetLayout;
			std::vector<VkDescriptorPoolSize>poolSizes;
		};

		//FROM SASCHA WILLEMS INITIALIZERS LIBRARY



		namespace INITIALIZERS {

			inline VkMemoryAllocateInfo memoryAllocateInfo()
			{
				VkMemoryAllocateInfo memAllocInfo{};
				memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				return memAllocInfo;
			}

			inline VkMappedMemoryRange mappedMemoryRange()
			{
				VkMappedMemoryRange mappedMemoryRange{};
				mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				return mappedMemoryRange;
			}

			inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
				VkCommandPool commandPool,
				VkCommandBufferLevel level,
				uint32_t bufferCount)
			{
				VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandPool = commandPool;
				commandBufferAllocateInfo.level = level;
				commandBufferAllocateInfo.commandBufferCount = bufferCount;
				return commandBufferAllocateInfo;
			}

			inline VkCommandPoolCreateInfo commandPoolCreateInfo()
			{
				VkCommandPoolCreateInfo cmdPoolCreateInfo{};
				cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				return cmdPoolCreateInfo;
			}

			inline VkCommandBufferBeginInfo commandBufferBeginInfo()
			{
				VkCommandBufferBeginInfo cmdBufferBeginInfo{};
				cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				return cmdBufferBeginInfo;
			}

			inline VkCommandBufferInheritanceInfo commandBufferInheritanceInfo()
			{
				VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
				cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				return cmdBufferInheritanceInfo;
			}

			inline VkRenderPassBeginInfo renderPassBeginInfo()
			{
				VkRenderPassBeginInfo renderPassBeginInfo{};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				return renderPassBeginInfo;
			}

			inline VkRenderPassCreateInfo renderPassCreateInfo()
			{
				VkRenderPassCreateInfo renderPassCreateInfo{};
				renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				return renderPassCreateInfo;
			}

			/** @brief Initialize an image memory barrier with no image transfer ownership */
			inline VkImageMemoryBarrier imageMemoryBarrier()
			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				return imageMemoryBarrier;
			}

			/** @brief Initialize a buffer memory barrier with no image transfer ownership */
			inline VkBufferMemoryBarrier bufferMemoryBarrier()
			{
				VkBufferMemoryBarrier bufferMemoryBarrier{};
				bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				return bufferMemoryBarrier;
			}

			inline VkMemoryBarrier memoryBarrier()
			{
				VkMemoryBarrier memoryBarrier{};
				memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				return memoryBarrier;
			}

			inline VkImageCreateInfo imageCreateInfo()
			{
				VkImageCreateInfo imageCreateInfo{};
				imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				return imageCreateInfo;
			}

			inline VkSamplerCreateInfo samplerCreateInfo()
			{
				VkSamplerCreateInfo samplerCreateInfo{};
				samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerCreateInfo.maxAnisotropy = 1.0f;
				return samplerCreateInfo;
			}

			inline VkImageViewCreateInfo imageViewCreateInfo()
			{
				VkImageViewCreateInfo imageViewCreateInfo{};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				return imageViewCreateInfo;
			}

			inline VkFramebufferCreateInfo framebufferCreateInfo()
			{
				VkFramebufferCreateInfo framebufferCreateInfo{};
				framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				return framebufferCreateInfo;
			}

			inline VkSemaphoreCreateInfo semaphoreCreateInfo()
			{
				VkSemaphoreCreateInfo semaphoreCreateInfo{};
				semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				return semaphoreCreateInfo;
			}

			inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)
			{
				VkFenceCreateInfo fenceCreateInfo{};
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.flags = flags;
				return fenceCreateInfo;
			}

			inline VkEventCreateInfo eventCreateInfo()
			{
				VkEventCreateInfo eventCreateInfo{};
				eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
				return eventCreateInfo;
			}

			inline VkSubmitInfo submitInfo()
			{
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				return submitInfo;
			}

			inline VkViewport viewport(
				float width,
				float height,
				float minDepth,
				float maxDepth)
			{
				VkViewport viewport{};
				viewport.width = width;
				viewport.height = height;
				viewport.minDepth = minDepth;
				viewport.maxDepth = maxDepth;
				return viewport;
			}

			inline VkRect2D rect2D(
				int32_t width,
				int32_t height,
				int32_t offsetX,
				int32_t offsetY)
			{
				VkRect2D rect2D{};
				rect2D.extent.width = width;
				rect2D.extent.height = height;
				rect2D.offset.x = offsetX;
				rect2D.offset.y = offsetY;
				return rect2D;
			}

			inline VkBufferCreateInfo bufferCreateInfo()
			{
				VkBufferCreateInfo bufCreateInfo{};
				bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				return bufCreateInfo;
			}

			inline VkBufferCreateInfo bufferCreateInfo(
				VkBufferUsageFlags usage,
				VkDeviceSize size)
			{
				VkBufferCreateInfo bufCreateInfo{};
				bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufCreateInfo.usage = usage;
				bufCreateInfo.size = size;
				return bufCreateInfo;
			}

			inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
				uint32_t poolSizeCount,
				VkDescriptorPoolSize* pPoolSizes,
				uint32_t maxSets)
			{
				VkDescriptorPoolCreateInfo descriptorPoolInfo{};
				descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descriptorPoolInfo.poolSizeCount = poolSizeCount;
				descriptorPoolInfo.pPoolSizes = pPoolSizes;
				descriptorPoolInfo.maxSets = maxSets;
				return descriptorPoolInfo;
			}

			inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
				const std::vector<VkDescriptorPoolSize>& poolSizes,
				uint32_t maxSets)
			{
				VkDescriptorPoolCreateInfo descriptorPoolInfo{};
				descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
				descriptorPoolInfo.pPoolSizes = poolSizes.data();
				descriptorPoolInfo.maxSets = maxSets;
				return descriptorPoolInfo;
			}

			inline VkDescriptorPoolSize descriptorPoolSize(
				VkDescriptorType type,
				uint32_t descriptorCount)
			{
				VkDescriptorPoolSize descriptorPoolSize{};
				descriptorPoolSize.type = type;
				descriptorPoolSize.descriptorCount = descriptorCount;
				return descriptorPoolSize;
			}

			inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
				VkDescriptorType type,
				VkShaderStageFlags stageFlags,
				uint32_t binding,
				uint32_t descriptorCount = 1)
			{
				VkDescriptorSetLayoutBinding setLayoutBinding{};
				setLayoutBinding.descriptorType = type;
				setLayoutBinding.stageFlags = stageFlags;
				setLayoutBinding.binding = binding;
				setLayoutBinding.descriptorCount = descriptorCount;
				return setLayoutBinding;
			}

			inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
				const VkDescriptorSetLayoutBinding* pBindings,
				uint32_t bindingCount)
			{
				VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
				descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorSetLayoutCreateInfo.pBindings = pBindings;
				descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
				return descriptorSetLayoutCreateInfo;
			}

			inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
				const std::vector<VkDescriptorSetLayoutBinding>& bindings)
			{
				VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
				descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorSetLayoutCreateInfo.pBindings = bindings.data();
				descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
				return descriptorSetLayoutCreateInfo;
			}

			inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
				const VkDescriptorSetLayout* pSetLayouts,
				uint32_t setLayoutCount = 1)
			{
				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
				pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
				pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
				return pipelineLayoutCreateInfo;
			}

			inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
				uint32_t setLayoutCount = 1)
			{
				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
				pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
				return pipelineLayoutCreateInfo;
			}

			inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
				VkDescriptorPool descriptorPool,
				const VkDescriptorSetLayout* pSetLayouts,
				uint32_t descriptorSetCount)
			{
				VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
				descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocateInfo.descriptorPool = descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
				descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
				return descriptorSetAllocateInfo;
			}

			inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
			{
				VkDescriptorImageInfo descriptorImageInfo{};
				descriptorImageInfo.sampler = sampler;
				descriptorImageInfo.imageView = imageView;
				descriptorImageInfo.imageLayout = imageLayout;
				return descriptorImageInfo;
			}

			inline VkWriteDescriptorSet writeDescriptorSet(
				VkDescriptorSet dstSet,
				VkDescriptorType type,
				uint32_t binding,
				VkDescriptorBufferInfo* bufferInfo,
				uint32_t descriptorCount = 1)
			{
				VkWriteDescriptorSet writeDescriptorSet{};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = dstSet;
				writeDescriptorSet.descriptorType = type;
				writeDescriptorSet.dstBinding = binding;
				writeDescriptorSet.pBufferInfo = bufferInfo;
				writeDescriptorSet.descriptorCount = descriptorCount;
				return writeDescriptorSet;
			}

			inline VkWriteDescriptorSet writeDescriptorSet(
				VkDescriptorSet dstSet,
				VkDescriptorType type,
				uint32_t binding,
				VkDescriptorImageInfo* imageInfo,
				uint32_t descriptorCount = 1)
			{
				VkWriteDescriptorSet writeDescriptorSet{};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = dstSet;
				writeDescriptorSet.descriptorType = type;
				writeDescriptorSet.dstBinding = binding;
				writeDescriptorSet.pImageInfo = imageInfo;
				writeDescriptorSet.descriptorCount = descriptorCount;
				return writeDescriptorSet;
			}

			inline VkVertexInputBindingDescription vertexInputBindingDescription(
				uint32_t binding,
				uint32_t stride,
				VkVertexInputRate inputRate)
			{
				VkVertexInputBindingDescription vInputBindDescription{};
				vInputBindDescription.binding = binding;
				vInputBindDescription.stride = stride;
				vInputBindDescription.inputRate = inputRate;
				return vInputBindDescription;
			}

			inline VkVertexInputAttributeDescription vertexInputAttributeDescription(
				uint32_t binding,
				uint32_t location,
				VkFormat format,
				uint32_t offset)
			{
				VkVertexInputAttributeDescription vInputAttribDescription{};
				vInputAttribDescription.location = location;
				vInputAttribDescription.binding = binding;
				vInputAttribDescription.format = format;
				vInputAttribDescription.offset = offset;
				return vInputAttribDescription;
			}

			inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo()
			{
				VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
				pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				return pipelineVertexInputStateCreateInfo;
			}

			inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
				const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
				const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions
			)
			{
				VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
				pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
				pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
				pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
				pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
				return pipelineVertexInputStateCreateInfo;
			}

			inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
				VkPrimitiveTopology topology,
				VkPipelineInputAssemblyStateCreateFlags flags,
				VkBool32 primitiveRestartEnable)
			{
				VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
				pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				pipelineInputAssemblyStateCreateInfo.topology = topology;
				pipelineInputAssemblyStateCreateInfo.flags = flags;
				pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
				return pipelineInputAssemblyStateCreateInfo;
			}

			inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
				VkPolygonMode polygonMode,
				VkCullModeFlags cullMode,
				VkFrontFace frontFace,
				VkPipelineRasterizationStateCreateFlags flags = 0)
			{
				VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
				pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
				pipelineRasterizationStateCreateInfo.cullMode = cullMode;
				pipelineRasterizationStateCreateInfo.frontFace = frontFace;
				pipelineRasterizationStateCreateInfo.flags = flags;
				pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
				pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
				return pipelineRasterizationStateCreateInfo;
			}

			inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
				VkColorComponentFlags colorWriteMask,
				VkBool32 blendEnable)
			{
				VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
				pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
				pipelineColorBlendAttachmentState.blendEnable = blendEnable;
				return pipelineColorBlendAttachmentState;
			}

			inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
				uint32_t attachmentCount,
				const VkPipelineColorBlendAttachmentState* pAttachments)
			{
				VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
				pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
				pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
				return pipelineColorBlendStateCreateInfo;
			}

			inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
				VkBool32 depthTestEnable,
				VkBool32 depthWriteEnable,
				VkCompareOp depthCompareOp)
			{
				VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
				pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
				pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
				pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
				pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
				return pipelineDepthStencilStateCreateInfo;
			}

			inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
				uint32_t viewportCount,
				uint32_t scissorCount,
				VkPipelineViewportStateCreateFlags flags = 0)
			{
				VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
				pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				pipelineViewportStateCreateInfo.viewportCount = viewportCount;
				pipelineViewportStateCreateInfo.scissorCount = scissorCount;
				pipelineViewportStateCreateInfo.flags = flags;
				return pipelineViewportStateCreateInfo;
			}

			inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
				VkSampleCountFlagBits rasterizationSamples,
				VkPipelineMultisampleStateCreateFlags flags = 0)
			{
				VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
				pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
				pipelineMultisampleStateCreateInfo.flags = flags;
				return pipelineMultisampleStateCreateInfo;
			}

			inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
				const VkDynamicState* pDynamicStates,
				uint32_t dynamicStateCount,
				VkPipelineDynamicStateCreateFlags flags = 0)
			{
				VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
				pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
				pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
				pipelineDynamicStateCreateInfo.flags = flags;
				return pipelineDynamicStateCreateInfo;
			}

			inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
				const std::vector<VkDynamicState>& pDynamicStates,
				VkPipelineDynamicStateCreateFlags flags = 0)
			{
				VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
				pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
				pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
				pipelineDynamicStateCreateInfo.flags = flags;
				return pipelineDynamicStateCreateInfo;
			}

			inline VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
			{
				VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
				pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
				return pipelineTessellationStateCreateInfo;
			}

			inline VkGraphicsPipelineCreateInfo pipelineCreateInfo(
				VkPipelineLayout layout,
				VkRenderPass renderPass,
				VkPipelineCreateFlags flags = 0)
			{
				VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
				pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineCreateInfo.layout = layout;
				pipelineCreateInfo.renderPass = renderPass;
				pipelineCreateInfo.flags = flags;
				pipelineCreateInfo.basePipelineIndex = -1;
				pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
				return pipelineCreateInfo;
			}

			inline VkGraphicsPipelineCreateInfo pipelineCreateInfo()
			{
				VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
				pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineCreateInfo.basePipelineIndex = -1;
				pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
				return pipelineCreateInfo;
			}

			inline VkComputePipelineCreateInfo computePipelineCreateInfo(
				VkPipelineLayout layout,
				VkPipelineCreateFlags flags = 0)
			{
				VkComputePipelineCreateInfo computePipelineCreateInfo{};
				computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
				computePipelineCreateInfo.layout = layout;
				computePipelineCreateInfo.flags = flags;
				return computePipelineCreateInfo;
			}

			inline VkPushConstantRange pushConstantRange(
				VkShaderStageFlags stageFlags,
				uint32_t size,
				uint32_t offset)
			{
				VkPushConstantRange pushConstantRange{};
				pushConstantRange.stageFlags = stageFlags;
				pushConstantRange.offset = offset;
				pushConstantRange.size = size;
				return pushConstantRange;
			}

			inline VkBindSparseInfo bindSparseInfo()
			{
				VkBindSparseInfo bindSparseInfo{};
				bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
				return bindSparseInfo;
			}

			/** @brief Initialize a map entry for a shader specialization constant */
			inline VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
			{
				VkSpecializationMapEntry specializationMapEntry{};
				specializationMapEntry.constantID = constantID;
				specializationMapEntry.offset = offset;
				specializationMapEntry.size = size;
				return specializationMapEntry;
			}

			/** @brief Initialize a specialization constant info structure to pass to a shader stage */
			inline VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
			{
				VkSpecializationInfo specializationInfo{};
				specializationInfo.mapEntryCount = mapEntryCount;
				specializationInfo.pMapEntries = mapEntries;
				specializationInfo.dataSize = dataSize;
				specializationInfo.pData = data;
				return specializationInfo;
			}

			/** @brief Initialize a specialization constant info structure to pass to a shader stage */
			inline VkSpecializationInfo specializationInfo(const std::vector<VkSpecializationMapEntry>& mapEntries, size_t dataSize, const void* data)
			{
				VkSpecializationInfo specializationInfo{};
				specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
				specializationInfo.pMapEntries = mapEntries.data();
				specializationInfo.dataSize = dataSize;
				specializationInfo.pData = data;
				return specializationInfo;
			}

			// Ray tracing related
			inline VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR()
			{
				VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR{};
				accelerationStructureGeometryKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
				return accelerationStructureGeometryKHR;
			}

			inline VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR()
			{
				VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR{};
				accelerationStructureBuildGeometryInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
				return accelerationStructureBuildGeometryInfoKHR;
			}

			inline VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR()
			{
				VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR{};
				accelerationStructureBuildSizesInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
				return accelerationStructureBuildSizesInfoKHR;
			}

			inline VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR()
			{
				VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR{};
				rayTracingShaderGroupCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
				return rayTracingShaderGroupCreateInfoKHR;
			}

			inline VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR()
			{
				VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR{};
				rayTracingPipelineCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
				return rayTracingPipelineCreateInfoKHR;
			}

			inline VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR()
			{
				VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR{};
				writeDescriptorSetAccelerationStructureKHR.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				return writeDescriptorSetAccelerationStructureKHR;
			}
		}
	


}




