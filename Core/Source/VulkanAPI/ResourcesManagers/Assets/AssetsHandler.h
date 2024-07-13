#pragma once
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "VulkanAPI/Utility/ISerializable.h"
#include "VulkanAPI/Model/ModelHandler.h"
#include "AssetThreat.h"

namespace VULKAN
{
	enum ASSET_TYPE
	{
		NONE,
		FILE,
		IMAGE,
		TEXTURE,
		MATERIAL,
		MODEL
	};
	struct AssetData: ISerializable<AssetData>
	{
		int assetId;
		std::string name;
        std::string codeFilePath="";
		std::string extensionType;
		uintmax_t sizeInBytes;
		ASSET_TYPE assetType;
		AssetData Deserialize(nlohmann::json& jsonObj) override;
		nlohmann::json Serialize() override;
		void SaveData() override;

	};

	class AssetsHandler
	{

	protected:


		static AssetsHandler* instance;
		std::unordered_map<std::string, AssetData> assets;
        std::vector<std::string>modelsPaths;
        std::vector<std::string>materialPaths;
		AssetsHandler();
        //only searching for assets extensions(ONLY, not mat of models)
		void SearchAllAssets(std::filesystem::path path);
        
        std::unordered_map<std::string, int> assetsLoaded;
		void SaveMetadata();
		int assetCounter;

        int counter = 0;
        void CreateSingleAssetMetadata(std::filesystem::path path, std::string data);
        void LoadSingleAssetMetadata(std::filesystem::path path);

        friend ModelHandler;
        
		std::string assetFilepath;
        AssetThreat assetThreat;
        

	public:
		
		~AssetsHandler();
		void RegisterSaves();

        std::string assetFileExtension = ".cAST";
        std::string codeModelFileExtension = ".CODE";
        std::string matFileExtension = ".MATCODE";

       
		void AddAssetData(std::string path, std::string codeAssetPath= "");
		AssetData& GetAssetData(std::string path);
		static AssetsHandler* GetInstance();
        
  
        template<typename T>
        std::string HandleAssetLoad(ISerializable<T>& iSerializableObj, std::string path , std::string fileType);
        template<typename T>
        void DeserializeCodeFile(std::unordered_map<std::string,std::shared_ptr<T>>& vectorToModify);
        template<typename T>
        void CreateCodeAsset(ISerializable<T>& iSerializableAsset,std::string path, std::string fileType);
    
	};

    template<typename T>
    std::string  AssetsHandler::HandleAssetLoad(ISerializable<T>& iSerializableObj, std::string path, std::string fileType) {
        nlohmann::json fileData = iSerializableObj.Serialize();
        std::filesystem::path filePath(path);
        std::string name = filePath.filename().string();
        filePath = filePath.parent_path();
        size_t extensionPos= name.find_last_of('.');
        name = name.substr(0, extensionPos);
        std::string metaFileName=filePath.string()+"\\"+ name + fileType;
        if (metaFileName==""){
            std::cout<<"Metafile name is empty: "<<metaFileName<<"\n";
            return "";
        }
        if (std::filesystem::exists(metaFileName) && fileType== assetFileExtension){
            std::cout<<"Loading asset from file: "<<metaFileName<<"\n";
            LoadSingleAssetMetadata(metaFileName);
        }else if(!std::filesystem::exists(metaFileName)) {
            std::cout<<"Creating asset from file: "<<metaFileName<<"\n";
            CreateSingleAssetMetadata(metaFileName, fileData.dump(4));
        }
        return metaFileName;
    }
    template<typename T>
    void AssetsHandler::DeserializeCodeFile(std::unordered_map<std::string,std::shared_ptr<T>>& vectorToModify) {
        
        std::vector<std::string>*pathsToLookInto = nullptr;
        int flagsCount=0;
        if (std::is_same<T, Material>::value){
            flagsCount = 1;
            pathsToLookInto = &materialPaths;
            
        } else if (std::is_same<T, ModelData>::value){
            flagsCount = 2;
            pathsToLookInto = &modelsPaths;
        } else{
            std::cout<<"It was tried to deserialize a non valid type \n";
            return;
        }
        for (const auto& path: *pathsToLookInto) {
            std::ifstream inFile(path);
            std::string line;
            std::string currentJsonString = "";
            while (std::getline(inFile, line))
            {
                currentJsonString += line;
                if (line[0]=='}')
                {
                    try
                    {
                        nlohmann::json currentJson = nlohmann::json::parse(currentJsonString);
                        T objToDeserialize{};
                        
                        objToDeserialize.Deserialize(currentJson);
                        vectorToModify.try_emplace(path,std::make_shared<T>(objToDeserialize));
                        
                        currentJsonString = "";
                    }
                    catch (nlohmann::json::parse_error& e)
                    {
                        std::cout << "Parse error at line " << line << ": " << e.what() << "\n";
                    }
                }

            }


        }           
    }


    template<typename T>
    void AssetsHandler::CreateCodeAsset(ISerializable<T>& iSerializableAsset,std::string path ,std::string fileType) {

        assetThreat.AddTask([this, &iSerializableAsset, path, fileType](){
            HandleAssetLoad<T>(iSerializableAsset, path, fileType);
        });
    }
    
}
