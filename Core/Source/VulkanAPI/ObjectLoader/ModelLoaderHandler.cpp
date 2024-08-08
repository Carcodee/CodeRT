#include "ModelLoaderHandler.h"
#include "VulkanAPI/Model/ModelHandler.h"
#include <filesystem>
#include <fstream>
#include <unordered_set>

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

namespace VULKAN {

	ModelLoaderHandler *ModelLoaderHandler::instance = nullptr;



	ModelLoaderHandler* ModelLoaderHandler::GetInstance()
	{
		if (instance==nullptr)
		{
			instance = new ModelLoaderHandler();
		}
		return instance;
	}

	ModelLoaderHandler::ModelLoaderHandler()
	{

	}

    void ModelLoaderHandler::FindReader(tinyobj::ObjReader& reader, std::string path) {
		tinyobj::ObjReaderConfig objConfig;

		if (!reader.ParseFromFile(path, objConfig))
		{
			if (!reader.Error().empty())
			{
				PRINTLVK("Error from reader":)
					PRINTLVK(reader.Error())
			}

		}
		if (!reader.Warning().empty()) {
			std::cout << "TinyObjReader: " << reader.Warning();
		}

    }
	ModelData ModelLoaderHandler::GetModelVertexAndIndicesTinyObject(std::string path)
	{
		tinyobj::ObjReader reader;
		tinyobj::ObjReaderConfig objConfig;

		if (!reader.ParseFromFile(path, objConfig))
		{
			if (!reader.Error().empty())
			{
				PRINTLVK("Error from reader":)
					PRINTLVK(reader.Error())
			}

		}
		if (!reader.Warning().empty()) {
			std::cout << "TinyObjReader: " << reader.Warning();
		}


		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<uint32_t> firstIndices;
		std::vector<uint32_t> firstMeshVertex;
		std::vector<uint32_t> meshIndexCount;
		std::vector<uint32_t> meshVertexCount;
		std::map<int, Material> materialsDatas;
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		std::vector<int> materialIdsOnObject;

		attrib = reader.GetAttrib();
		shapes = reader.GetShapes();
		materials = reader.GetMaterials();
		int meshCount = shapes.size();
		int indexStartCounter = 0;
		int vertexStartCouner = 0;
		for (const auto& shape : shapes)
		{
			if (shape.mesh.material_ids.size() <= 0 || shape.mesh.material_ids[0] < 0)
			{
				materialIdsOnObject.push_back(0);
			}
			else
			{
				materialIdsOnObject.push_back(static_cast<uint32_t>(shape.mesh.material_ids[0]+ ModelHandler::GetInstance()->currentMaterialsOffset));
			}
			bool firstIndexAddedPerMesh = false;
			bool firstVertexFinded = false;
			int indexCount = 0;
			int vertexCount = 0;
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}
				else {
					vertex.normal = { 0.0f, 0.0f, 0.0f }; // Default normal if not present
				}

				if (index.texcoord_index == -1)
				{
					vertex.texCoord = {
						0,
						0
					};
				}
				else
				{
					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);

					if (!firstVertexFinded)
					{
						firstMeshVertex.push_back(vertexStartCouner);
						firstVertexFinded = true;
					}
					vertexStartCouner++;
					vertexCount++;
				}
				if (!firstIndexAddedPerMesh)
				{
					firstIndices.push_back(indexStartCounter);
					firstIndexAddedPerMesh = true;
				}
				indices.push_back(uniqueVertices[vertex]);
				indexStartCounter++;
				indexCount++;

			}
			meshIndexCount.push_back(indexCount);
			meshVertexCount.push_back(vertexCount);

		}
        for (int i = 0; i <indices.size() ; i+=3) {

            int index1 =indices[i];
            int index2 =indices[i + 1];
            int index3 =indices[i + 2];

            glm::vec3 tangent = CalculateTangent(vertices[index1].position,vertices[index2].position,vertices[index3].position,
                                                 vertices[index1].texCoord,vertices[index2].texCoord,vertices[index3].texCoord);
            vertices[index1].tangent = tangent;
            vertices[index2].tangent = tangent;
            vertices[index3].tangent = tangent;
        }
        int textureTotalSize = 0;

        ModelData modelData = {};
        modelData.materialOffset= ModelHandler::GetInstance()->currentMaterialsOffset;
		materialsDatas = LoadMaterialsFromReader(reader, path);
		std::vector<VKTexture>allTextures;

        modelData.vertices=vertices;
        modelData.indices=indices;
        modelData.firstMeshIndex=firstIndices;
        modelData.firstMeshVertex=firstMeshVertex;
        modelData.materialIds=materialIdsOnObject;
        modelData.meshIndexCount=meshIndexCount;
        modelData.meshVertexCount=meshVertexCount;
        modelData.materialDataPerMesh=materialsDatas;
        modelData.meshCount = meshCount;
        modelData.indexBLASOffset = 0;
        modelData.vertexBLASOffset = 0;
        modelData.transformBLASOffset = 0;
        modelData.generated = true;
		return modelData;
	}

	std::vector<VKTexture> ModelLoaderHandler::LoadTexturesFromPath(std::string path, VulkanSwapChain& swapChain)
	{
		std::vector<VKTexture> textures;
		std::filesystem::path texturePaths= std::filesystem::path(path);
		if (!std::filesystem::exists(texturePaths)&& !std::filesystem::is_directory(texturePaths))
		{
			PRINTLVK("Not a valid directory or path")	
			return textures;
		}
		std::vector<std::string> paths;
		for (const auto& entry: std::filesystem::directory_iterator(texturePaths))
		{
			paths.push_back(entry.path().string());
		}
		if (paths.size()<=0)
		{
			PRINTLVK("No paths inside the file")	
			return textures;
		}
		for (int i = 0; i < paths.size(); ++i)
		{
			VKTexture texture(paths[i].c_str(), swapChain);
			textures.push_back(texture);
		}
		return textures;
	}

	std::map<int, Material> ModelLoaderHandler::LoadMaterialsFromReader(tinyobj::ObjReader& reader ,std::string path)
	{
		auto& materials = reader.GetMaterials();

		int matCount = 0;
		std::filesystem::path currentModelPath(path);
		currentModelPath = currentModelPath.parent_path();

		std::string texturesPath = currentModelPath.string() + "\\textures";
		if (!std::filesystem::exists(texturesPath))
		{
			std::cout << "The current model does not have textures relative to the folder, Creating materials at model path: " << path << "\n";
		}

		std::map<int,Material> materialsDatas;

		for (const auto& material : materials)
		{
			Material materialData{};
			materialData.materialUniform.diffuseColor = glm::make_vec3(material.diffuse);
            materialData.materialUniform.roughnessIntensity = material.roughness;
            materialData.materialUniform.metallicIntensity = material.metallic;
            bool texturesFinded= false;
			if (!material.diffuse_texname.empty()) {
				std::string texturePathFinded = material.diffuse_texname;
				FixMaterialPaths(texturePathFinded, texturesPath, currentModelPath.string());
                materialData.paths.try_emplace(TEXTURE_TYPE::DIFFUSE,texturePathFinded);
                materialData.materialUniform.diffuseColor = glm::vec3(1.0f);
                texturesFinded = true;
			}
            if (!material.roughness_texname.empty() || !material.specular_texname.empty()|| !material.specular_highlight_texname.empty()) {
                std::string texName;
                if (!material.roughness_texname.empty()){
                    texName=material.roughness_texname;
                }else if (!material.specular_texname.empty()){
                    texName=material.specular_texname;
                }else if (!material.specular_highlight_texname.empty()){
                    texName=material.specular_highlight_texname;
                }
                std::string texturePathFinded= texName;
                FixMaterialPaths(texturePathFinded, texturesPath, currentModelPath.string());
                materialData.paths.try_emplace(TEXTURE_TYPE::ROUGHNESS,texturePathFinded);
                materialData.materialUniform.roughnessIntensity = 0.5f;
                texturesFinded = true;
            }
            
            if (!material.metallic_texname.empty()) {
                std::string texturePathFinded= material.metallic_texname;
                FixMaterialPaths(texturePathFinded, texturesPath, currentModelPath.string());
                materialData.paths.try_emplace(TEXTURE_TYPE::METALLIC,texturePathFinded);
                materialData.materialUniform.metallicIntensity = 1.0f;
                texturesFinded = true;
            }

			if (!material.bump_texname.empty()) {
				std::string texturePathFinded= material.bump_texname;
				FixMaterialPaths(texturePathFinded, texturesPath, currentModelPath.string());
                materialData.paths.try_emplace(TEXTURE_TYPE::NORMAL,texturePathFinded);
                materialData.materialUniform.normalIntensity = 1.0f;
                texturesFinded = true;
			}

            materialData.textureReferencePath= texturesPath;
            materialData.name = "Material_"+std::to_string(matCount);
            materialData.id = ModelHandler::GetInstance()->currentMaterialsOffset;
            if(texturesFinded){
                materialData.targetPath = texturesPath +"\\"+ materialData.name + ".MATCODE";
            } else{
                materialData.targetPath = currentModelPath.string() +"\\"+ materialData.name + ".MATCODE";
            }
			materialsDatas.try_emplace(matCount, materialData);
            ModelHandler::GetInstance()->allMaterialsOnApp.try_emplace(materialData.id,std::make_shared<Material>(materialData));
			matCount++;
            ModelHandler::GetInstance()->currentMaterialsOffset++;
		}

		return materialsDatas;
	}

	void ModelLoaderHandler::FixMaterialPaths(std::string& path, std::string texturesPath, std::string modelPath)
	{
		if (!std::filesystem::exists(texturesPath))
		{
			path =modelPath;
			return;
		}
		if (!std::filesystem::exists(path)&&path.size()>0)
		{
			size_t notValidPathFinishPos = path.find_last_of("//");
			path.erase(0, notValidPathFinishPos + 1);
			size_t extensionPart = path.find_last_of('.');
			path.erase(extensionPart, path.size());
			for (auto element :std::filesystem::directory_iterator(texturesPath))
			{
				if (element.is_regular_file())
				{
					std::string fileInPath=element.path().string();
					std::string bufferFileInPath=element.path().string();

					size_t filePathFinishPos = fileInPath.find_last_of("\\");

					fileInPath.erase(0, filePathFinishPos + 1);
					bufferFileInPath.erase(0, filePathFinishPos + 1);

					size_t extensionPartFileInPath = bufferFileInPath.find_last_of('.');

					//extension part
					bufferFileInPath.erase(extensionPartFileInPath, path.size());

					if (bufferFileInPath == path)
					{
						path = fileInPath;
						std::cout << "Filepath matches with: " << path <<"\n";
						break;
					}
					
				}
				
			}
			path = texturesPath + "\\" + path;
			if (!std::filesystem::exists(path))
			{
				std::cout << "Filepath does not exist: " << path <<"\n";
				path = modelPath;
			}

		}
		else
		{
			std::cout << "Path is absolute and exist: " << path <<"\n";
		}
	}

    void ModelLoaderHandler::GetModelFromReader(tinyobj::ObjReader& reader, ModelData& modelData) {

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<uint32_t> firstIndices;
        std::vector<uint32_t> firstMeshVertex;
        std::vector<uint32_t> meshIndexCount;
        std::vector<uint32_t> meshVertexCount;
        std::map<int, Material> materialsDatas;
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        std::vector<int> materialIdsOnObject;

        attrib = reader.GetAttrib();
        shapes = reader.GetShapes();
        materials = reader.GetMaterials();
        int meshCount = shapes.size();
        int indexStartCounter = 0;
        int vertexStartCouner = 0;
        for (const auto& shape : shapes)
        {
            if (shape.mesh.material_ids.size() <= 0 || shape.mesh.material_ids[0] < 0)
            {
                materialIdsOnObject.push_back(0);
            }
            else
            {
                //TODO: this is wrong, the offset must be the one that was when was loaded
                materialIdsOnObject.push_back(static_cast<uint32_t>(shape.mesh.material_ids[0] + modelData.materialOffset));
            }
            bool firstIndexAddedPerMesh = false;
            bool firstVertexFinded = false;
            int indexCount = 0;
            int vertexCount = 0;
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};
                vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                };
                vertex.color = { 1.0f, 1.0f, 1.0f };

                if (index.normal_index >= 0) {
                    vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                    };
                }
                else {
                    vertex.normal = { 0.0f, 0.0f, 0.0f }; // Default normal if not present
                }

                if (index.texcoord_index == -1)
                {
                    vertex.texCoord = {
                            0,
                            0
                    };
                }
                else
                {
                    vertex.texCoord = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }
                
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);

                    if (!firstVertexFinded)
                    {
                        firstMeshVertex.push_back(vertexStartCouner);
                        firstVertexFinded = true;
                    }
                    vertexStartCouner++;
                    vertexCount++;
                }
                if (!firstIndexAddedPerMesh)
                {
                    firstIndices.push_back(indexStartCounter);
                    firstIndexAddedPerMesh = true;
                }
                indices.push_back(uniqueVertices[vertex]);
                indexStartCounter++;
                indexCount++;
                

            }
            meshIndexCount.push_back(indexCount);
            meshVertexCount.push_back(vertexCount);

        }
        for (int i = 0; i <indices.size() ; i+=3) {

            int index1 =indices[i];
            int index2 =indices[i + 1];
            int index3 =indices[i + 2];
            
            glm::vec3 tangent = CalculateTangent(vertices[index1].position,vertices[index2].position,vertices[index3].position,
                                                 vertices[index1].texCoord,vertices[index2].texCoord,vertices[index3].texCoord);
            vertices[index1].tangent = tangent;
            vertices[index2].tangent = tangent;
            vertices[index3].tangent = tangent;
        } 
        int textureTotalSize = 0;
        std::vector<VKTexture>allTextures;
        modelData.vertices=vertices;
        modelData.indices=indices;
        modelData.firstMeshIndex=firstIndices;
        modelData.firstMeshVertex=firstMeshVertex;
        modelData.materialIds=materialIdsOnObject;
        modelData.meshIndexCount=meshIndexCount;
        modelData.meshVertexCount=meshVertexCount;
        modelData.materialDataPerMesh=materialsDatas;
        modelData.meshCount = meshCount;
        modelData.indexBLASOffset = 0;
        modelData.vertexBLASOffset = 0;
        modelData.transformBLASOffset = 0;
    }


    glm::vec3 ModelLoaderHandler::CalculateTangent(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 pos3,
                                                   glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3) {
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;
        glm::vec3 tangent;
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        return glm::normalize(tangent);
    }
}

