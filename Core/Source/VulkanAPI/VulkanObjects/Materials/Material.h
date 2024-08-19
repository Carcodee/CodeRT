//
// Created by carlo on 7/20/2024.
//
#include <iostream>
#include <glm/glm.hpp>
#include "VulkanAPI/Utility/ISerializable.h"
#include "VulkanAPI/VulkanObjects/Textures/VKTexture.h"

#ifndef EDITOR_MATERIAL_H
#define EDITOR_MATERIAL_H

namespace VULKAN{

    enum TEXTURE_TYPE {
        DIFFUSE = 0,
        ROUGHNESS = 1,
        METALLIC = 2,
        NORMAL = 3,
        EMISSIVE = 4 ,
        METALLICROUGHNESS = 5
    };
    enum CONFIG_TYPE{
        ALPHA_AS_DIFFUSE,
        ALPHA_AS_A_CHANNEL
    };



    struct MaterialUniformData
    {
        float albedoIntensity = 1;
        float normalIntensity = 1;
        float specularIntensity = 1;
        float roughnessIntensity = 0.5f;
        glm::vec3 diffuseColor = glm::vec3 (1.0f);
        float reflectivityIntensity = 0.5f;
        //32
        glm::vec3 baseReflection;
        float metallicIntensity = 0.0f;
        //48
        float emissionIntensity = 0.0f;
        int roughnessOffset = -1;
        int metallicOffset = -1;
        int emissionOffset = -1;
        //64
        int metallicRoughnessOffset  = -1;
        float alphaCutoff = 1.0f;
        int diffuseOffset = -1;
        int normalOffset = -1;
        //80
        uint32_t configurations = 0;
        
    };

    struct Material: ISerializable<Material>
    {

        void RemoveTexture(TEXTURE_TYPE textureType);
        void CreateTextures(VulkanSwapChain& swap_chain, int& allTexturesOffset);
        void SetTexture(TEXTURE_TYPE textureType,VKTexture* texture);
        ~Material(){
            for (auto& pair : materialTextures) {
                delete pair.second;
            }
        };
        nlohmann::json Serialize() override;
        Material Deserialize(nlohmann::json &jsonObj) override;
        void SetConfiguration(int configData);
        bool GetConfigVal(CONFIG_TYPE configType);
        void SetConfigVal(CONFIG_TYPE configType, bool value);
        void SaveData() override{


        }
        MaterialUniformData materialUniform{};
        //the key is the texture offset in order correlate them 
        std::map<TEXTURE_TYPE,std::string> paths;
        std::map<TEXTURE_TYPE,VKTexture*> materialTextures;
        std::string textureReferencePath="";
        std::string name="";
        std::string targetPath="";
        bool generated = false;
        int id = 0;
    };

}
#endif //EDITOR_MATERIAL_H
