#include "piko/assets.hpp"
#include "piko/logger.hpp"

#include "raylib.h"
#include "json.hpp"

#include <stdexcept>
#include <fstream>

using json = nlohmann::json;
using namespace piko;

std::string AssetManager::ReadFileToString(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File not found at path: " + path);
    }
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

AssetManager::AssetManager(){
    PBOX_INFO("ASSET_MAN: Asset Manager Initialized.");
}

AssetManager::~AssetManager() {
    PBOX_INFO("ASSET_MAN: Cleaning up resources....");
    spriteSheets.clear();
    textures.clear();
    shaders.clear();
    texSprites.clear();
    audios.clear();
    fonts.clear();
    PBOX_INFO("ASSET_MAN: Cleanup complete.");
}

const TextureIMG* AssetManager::addTexture(std::string key, std::string path){
    auto tex_exist = textures.find(key);
    if (tex_exist != textures.end()) {
        PBOX_WARN("ASSET_MAN: Cannot create texture '%s'. It already exist.", key.c_str());
        return &(tex_exist->second);
    }

    try {
        // Use emplace to construct directly in the map node. No temporaries, no leaks!
        auto [it, success] = textures.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(path)
        );

        if (!success) {
            throw std::runtime_error("Map insertion failed for texture");
        }

        texPathToKey[path] = key;
        return &(it->second);

    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Cannot create texture '%s': %s", key.c_str(), e.what());
        return nullptr; 
    }
}

const TextureIMG* AssetManager::getTexture(std::string key){
    auto tex_exist = textures.find(key);
    if (tex_exist != textures.end()) {
        return &(tex_exist->second);
    }
    PBOX_ERROR("ASSET_MAN: Cannot find texture '%s'", key.c_str());
    return nullptr;
}

const TextureIMG* AssetManager::getTextureByPath(std::string path){
    auto key_exist = texPathToKey.find(path);
    if (key_exist != texPathToKey.end()) {
        return getTexture(key_exist->second);
    }
   
    PBOX_ERROR("ASSET_MAN: Cannot find texture with path: %s", path.c_str());
    return nullptr;
}

const Sprite* AssetManager::getTexSprite(std::string key){
    // 1. Ensure the texture exists first
    auto tex_exist = textures.find(key);
    if (tex_exist == textures.end()) {
        PBOX_ERROR("ASSET_MAN: Cannot find texture sprite '%s'", key.c_str());
        return nullptr;
    }

    // 2. Return if the 1:1 sprite wrapper is already cached
    auto tsprite_exist = texSprites.find(key); 
    if (tsprite_exist != texSprites.end()) {
        return &(tsprite_exist->second);
    }

    // 3. Construct the sprite wrapper safely inside the node map space
    const TextureIMG* texPtr = &(tex_exist->second);
    Rect srcRect = { 0.0f, 0.0f, static_cast<float>(texPtr->getWidth()), static_cast<float>(texPtr->getHeight()) };
    
    auto [it, success] = texSprites.emplace(key, Sprite{ texPtr, srcRect, key, -1, { 0.0f, 0.0f } });
    return &(it->second);
}

const Sprite* AssetManager::getTexSpriteByPath(std::string path){
    auto key_exist = texPathToKey.find(path);
    if (key_exist != texPathToKey.end()) {
        return getTexSprite(key_exist->second);
    }
    PBOX_ERROR("ASSET_MAN: Cannot find texture sprite with path: %s", path.c_str());
    return nullptr;
}

const AudioClip* AssetManager::addAudioClip(std::string key, std::string path, AudioClip::AudioType type, int targetChannel) {
    auto aud_exist = audios.find(key);
    if (aud_exist != audios.end()) {
        PBOX_WARN("ASSET_MAN: Cannot create audio clip '%s'. It already exist.", key.c_str());
        return &(aud_exist->second);
    }

    try {
        auto [it, success] = audios.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(path, type, targetChannel)
        );
        
        audioPathToKey[path] = key;
        return &(it->second);
    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Cannot create audio clip '%s': %s", key.c_str(), e.what());
        return nullptr;
    }
}

const FontAtlas* AssetManager::addFontAtlas(std::string key, std::string path, int baseSize){
    auto font_exist = fonts.find(key);
    if (font_exist != fonts.end()) {
        PBOX_WARN("ASSET_MAN: Cannot create font atlas '%s'. It already exist.", key.c_str());
        return &(font_exist->second);
    }

    try {
        // Use emplace to construct directly in the map node. No temporaries, no leaks!
        auto [it, success] = fonts.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(path, baseSize)
        );

        if (!success) {
            throw std::runtime_error("Map insertion failed for font.");
        }

        fontPathToKey[path] = key;
        return &(it->second);

    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Cannot create font atlas '%s': %s", e.what());
        return nullptr; 
    }
}

const FontAtlas* AssetManager::getFontAtlas(std::string key){
    auto font_exist = fonts.find(key);
    if (font_exist != fonts.end()) {
        return &(font_exist->second);
    }

    PBOX_ERROR("ASSET_MAN: Cannot find font atlas '%s'", key.c_str());
    return nullptr;
}

const FontAtlas* AssetManager::getFontAtlasByPath(std::string path){
    auto font_exist = fontPathToKey.find(path);
    if (font_exist != fontPathToKey.end()) {
        return getFontAtlas(font_exist->second);
    }
    PBOX_ERROR("ASSET_MAN: Cannot find font atlas with path: %s", path.c_str());
    return nullptr;
}

const AudioClip* AssetManager::getAudioClip(std::string key){
    auto aud_exist = audios.find(key);
    if (aud_exist != audios.end()) {
        return &(aud_exist->second);
    }
    PBOX_ERROR("ASSET_MAN: Cannot find audio clip '%s'", key.c_str());
    return nullptr;
}

const AudioClip* AssetManager::getAudioClipByPath(std::string path){
    auto audio_exist = audioPathToKey.find(path);
    if (audio_exist != audioPathToKey.end()) {
        return getAudioClip(audio_exist->second);
    }
    PBOX_ERROR("ASSET_MAN: Cannot find audio with path: %s", path.c_str());
    return nullptr;
}

const SpriteSheet* AssetManager::addSpriteSheet(std::string key, const TextureIMG* tex, Vect2 sprSize, int numSprs, int spacing, Vect2 startPos, int wrapX) {
    auto sheet_exist = spriteSheets.find(key);
    if (sheet_exist != spriteSheets.end()) {
        PBOX_WARN("ASSET_MAN: Cannot create sprite sheet '%s'. It already exist.", key.c_str());
        return &sheet_exist->second;
    }

    try {
        if (tex == nullptr || sprSize.x <= 0 || sprSize.y <= 0 || numSprs <= 0 || wrapX < 0) {
            throw std::runtime_error("Invalid spritesheet parameters");
        }

        int sprWidth = static_cast<int>(sprSize.x);
        int sprHeight = static_cast<int>(sprSize.y);
        int currentX = static_cast<int>(startPos.x);
        int currentY = static_cast<int>(startPos.y);

        if (wrapX + sprWidth > tex->getWidth()) {
            throw std::runtime_error("wrapX position would cause immediate texture overflow");
        }

        std::vector<Rect> sources;
        sources.reserve(numSprs);

        for (int i = 0; i < numSprs; i++) {
            if (currentY + sprHeight > tex->getHeight()) {
                PBOX_WARN("ASSET_MAN: Reached end of texture for spirte sheet '%s'. Loaded sprite %d/%d", key.c_str(), i, numSprs);
                break;
            }

            sources.push_back(Rect{ (float)currentX, (float)currentY, (float)sprWidth, (float)sprHeight });
            currentX += sprWidth + spacing;

            if (currentX + sprWidth > tex->getWidth()) {
                currentX = wrapX;
                currentY += sprHeight + spacing;
            }
        }

        // Emplace your sheet directly to keep the vector allocations completely secure inside the heap node
        auto [it, success] = spriteSheets.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(tex, sources, key)
        );

        return &(it->second);
    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Cannot create sprite sheet '%s': %s", key.c_str(), e.what());
        return nullptr;
    }
}

const SpriteSheet* AssetManager::addSpriteSheet(std::string key, const TextureIMG* tex, const std::vector<Rect>& sources){
    auto sheet_exist = spriteSheets.find(key);
    if (sheet_exist != spriteSheets.end()) {
        PBOX_WARN("ASSET_MAN: Cannot create sprite sheet '%s'. It already exist.", key.c_str());
        return &sheet_exist->second;
    }

     try {
        // Emplace your sheet directly to keep the vector allocations completely secure inside the heap node
        auto [it, success] = spriteSheets.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(tex, sources, key)
        );

        return &(it->second);
    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Cannot create sprite sheet '%s': %s", key.c_str(), e.what());
        return nullptr;
    }
}

const SpriteSheet* AssetManager::getSpriteSheet(std::string key){
    auto sheet_exist = spriteSheets.find(key);
    if (sheet_exist != spriteSheets.end()) {
        return &(sheet_exist->second);
    }
    PBOX_ERROR("ASSET_MAN: Cannot find sprite sheet '%s'", key.c_str());
    return nullptr;
}

const Sprite* AssetManager::getSpriteFromSheet(std::string sheet, uint16_t index){
    const SpriteSheet* sprSheet = getSpriteSheet(sheet);
    return sprSheet ? sprSheet->getSprite(index) : nullptr;
}

RenderShader* AssetManager::addShader(std::string key, std::string verPath, std::string fragPath) {
    auto shader_exist = shaders.find(key);
    if (shader_exist != shaders.end()) {
        PBOX_WARN("ASSET_MAN: Cannot create shader '%s'. It already exist.", key.c_str());
        return &(shader_exist->second);
    }

    try {
        // Read file contents to raw strings right here in the asset system
        std::string vCode = ReadFileToString(verPath);
        std::string fCode = ReadFileToString(fragPath);

        auto [it, success] = shaders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(vCode, fCode, key) // Passes the code strings down!
        );
        it->second.verPath = verPath;
        it->second.fragPath = fragPath;
        return &(it->second);
    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Failed to create shader '%s': %s", key.c_str(), e.what());
        return nullptr; 
    }
}

RenderShader* AssetManager::addShaderFromMemory(std::string key, std::string verCode, std::string fragCode) {
    auto shader_exist = shaders.find(key);
    if (shader_exist != shaders.end()) {
        return &(shader_exist->second);
    }

    try {
        auto [it, success] = shaders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(verCode, fragCode, key)
        );
        it->second.verPath = "[MEMORY]";
        it->second.fragPath = "[MEMORY]";
        return &(it->second);
    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: failed to create shader '%s' from memory: %s", key.c_str(), e.what());
        return nullptr;
    }
}

RenderShader* AssetManager::getShader(std::string key){
    auto shader_exist = shaders.find(key);
    return (shader_exist != shaders.end()) ? &(shader_exist->second) : nullptr;
}

std::string AssetManager::serialize() {
    json data = json::object();

    // 1. Core Textures Matrix
    json textureMap = json::object();
    for (const auto& [key, tex] : textures) {
        textureMap[key] = tex.getFilePath(); // Assuming getFilePath() or path property exists
    }
    data["textures"] = textureMap;

    // 2. Audio Files Registry
    json audioMap = json::object();
    for (const auto& [key, audio] : audios) {
        json audioData = {
            {"path", audio.getFilePath()},
            {"type", static_cast<int>(audio.getType())}, // Serialize enum to int
            {"channel", audio.getDefaultChannel()}
        };
        audioMap[key] = audioData;
    }
    data["audio"] = audioMap;

    // 3. Typography Font Atlases
    json fontMap = json::object();
    for (const auto& [key, font] : fonts) {
        json fontData = {
            {"path", font.getFilePath()},
            {"baseSize", font.getBaseSize()}
        };
        fontMap[key] = fontData;
    }
    data["fonts"] = fontMap;

    // 4. Render Shaders Program Files
    json shaderMap = json::object();
    for (const auto& [key, shader] : shaders) {
        std::string vPath = shader.verPath;
        std::string fPath = shader.fragPath;
        if(vPath.empty() || vPath == "[MEMORY]" || fPath.empty() || fPath == "[MEMORY]"){
            continue;
        }
        json shaderData = {
            {"vertPath", vPath},
            {"fragPath", fPath}
        };
        shaderMap[key] = shaderData;
    }
    data["shaders"] = shaderMap;

    // 5. SpriteSheet Packing Layouts (Depends on Texture Keys)
    json sheetMap = json::object();
    for (const auto& [key, sheet] : spriteSheets) {
        json sprSrcs = json::array();
        const std::vector<Rect>& sources = sheet.getSources();
        for(const Rect& r : sources){
            sprSrcs.push_back(
                {{"x", r.x},{"y", r.y},{"w", r.w},{"h", r.h}}
            );
        }

        json sheetData = {
            {"texturePath", sheet.getTexAtlas()->getFilePath()},
            {"sprSources", sprSrcs}
        };
        sheetMap[key] = sheetData;
    }
    data["spriteSheets"] = sheetMap;

    return data.dump(4);
}

void AssetManager::deserialize(const std::string& rawJson) {
    json data = json::parse(rawJson);

    // Hard reset all internal containers to guarantee a completely clean sandbox state
    this->texSprites.clear();
    this->textures.clear();
    this->texPathToKey.clear();
    this->audios.clear();
    this->audioPathToKey.clear();
    this->spriteSheets.clear();

    if (data.contains("textures") && data["textures"].is_object()) {
        for (const auto& [key, pathJson] : data["textures"].items()) {
            if (pathJson.is_string()) {
                this->addTexture(key, pathJson.get<std::string>());
            }
        }
        PBOX_INFO("ASSET_MAN: Successfully deserialized texture assets to runtime." );
    }

    if (data.contains("audio") && data["audio"].is_object()) {
        for (const auto& [key, audioData] : data["audio"].items()) {
            std::string path = audioData.value("path", "");
            int typeInt = audioData.value("type", 0);
            int channel = audioData.value("channel", 0);
            
            AudioClip::AudioType type = static_cast<AudioClip::AudioType>(typeInt);
            this->addAudioClip(key, path, type, channel);
        }
        PBOX_INFO("ASSET_MAN: Successfully audio assets to runtime." );
    }

    if (data.contains("fonts") && data["fonts"].is_object()) {
        for (const auto& [key, fontData] : data["fonts"].items()) {
            std::string path = fontData.value("path", "");
            int baseSize = fontData.value("baseSize", 16);
            this->addFontAtlas(key, path, baseSize);
        }
        PBOX_INFO("ASSET_MAN: Successfully deserialized font assets to runtime." );
    }

    if (data.contains("shaders") && data["shaders"].is_object()) {
        for (const auto& [key, shaderData] : data["shaders"].items()) {
            std::string vert = shaderData.value("vertPath", "");
            std::string frag = shaderData.value("fragPath", "");
            this->addShader(key, vert, frag);
        }
        PBOX_INFO("ASSET_MAN: Successfully deserialized shader assets to runtime." );
    }

    if (data.contains("spriteSheets") && data["spriteSheets"].is_object()) {
        for (const auto& [key, sheetData] : data["spriteSheets"].items()) {
            std::string texPath = sheetData.value("texturePath", "");
            
            // Look up the matching texture address via its disk path token
            const TextureIMG* tex = this->getTextureByPath(texPath);

            if (tex != nullptr && sheetData.contains("sprSources") && sheetData["sprSources"].is_array()) {
                std::vector<Rect> parsedSources;
                
                // Parse out every individual variable coordinate bounding box
                for (const auto& rectJson : sheetData["sprSources"]) {
                    Rect r;
                    r.x = rectJson.value("x", 0.0f);
                    r.y = rectJson.value("y", 0.0f);
                    r.w = rectJson.value("w", 0.0f);
                    r.h = rectJson.value("h", 0.0f);
                    parsedSources.push_back(r);
                }

                // Call your new vector-overloaded custom placement method
                this->addSpriteSheet(key, tex, parsedSources);
            } else {
                PBOX_ERROR("ASSET_MAN: Deserialization... Could not resolve source TextureIMG or frame array data for SpriteSheet '%s' at path '%s'", 
                        key.c_str(), 
                        texPath.c_str()
                    );
            }
        }
        PBOX_INFO("ASSET_MAN: Successfully deserialized spritesheet assets to runtime." );
    }

    PBOX_INFO("ASSET_MAN: Successfully deserialized all assets to runtime." );
}


bool AssetManager::saveAssetsRefToFile(std::string path){
    try {
        std::ofstream output_file(path);
        
        if (!output_file.is_open()) {
            PBOX_ERROR("ASSET_MAN: Serialization failed. Could not open or create file at : '%s'", path.c_str());
            return false;
        }

        output_file << this->serialize();

        output_file.close();
        PBOX_INFO("ASSET_MAN: Successfully Serialized assets to '%s'", path.c_str());
        return true;

    } catch (const json::parse_error& e) {
        PBOX_ERROR("ASSET_MAN: Serialization failed. JSON Parsing Failed '%s'", e.what());
        return false;
    }
}

bool AssetManager::loadAssetsFromRefFile(std::string path) {
    std::ifstream input_file(path);
    
    if (!input_file.is_open()) {
        PBOX_ERROR("ASSET_MAN: Loading serialized assets failed. Could not open or load file at : '%s'", path.c_str());
        return false;
    }

    try {
        json assetData;
        input_file >> assetData; // ◄— Parses the entire file stream automatically, whitespaces and all!
        input_file.close();

        // Pass the dump directly to your deserializer
        this->deserialize(assetData.dump());

        PBOX_INFO("ASSET_MAN: Successfully loaded assets from reference file: '%s'", path.c_str());
        return true;

    } catch (const json::parse_error& e) {
        PBOX_ERROR("ASSET_MAN: Loading serialized assets failed from '%s' : %s", path.c_str(), e.what());
        return false;
    } catch (const std::exception& e) {
        PBOX_ERROR("ASSET_MAN: Unexpected structural execution drop during load phase: %s", e.what());
        return false;
    }
}