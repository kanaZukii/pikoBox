#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>

#include "piko/math.hpp"
#include "piko/sprite.hpp"
#include "piko/shader.hpp"
#include "piko/audioClip.hpp"
#include "piko/font.hpp"
#include "piko/logger.hpp"

namespace piko {
    static std::string ReadFileToString(const std::string& path);

    enum class AssetType {Texture, Audio, Font, Shader, SpriteSheet};
    template<typename T> struct AssetMapType;
    template<> struct AssetMapType<TextureIMG> { static constexpr AssetType type = AssetType::Texture; };
    template<> struct AssetMapType<AudioClip> { static constexpr AssetType type = AssetType::Audio; };
    template<> struct AssetMapType<FontAtlas> { static constexpr AssetType type = AssetType::Font; };
    template<> struct AssetMapType<RenderShader> { static constexpr AssetType type = AssetType::Shader; };
    template<> struct AssetMapType<SpriteSheet> { static constexpr AssetType type = AssetType::SpriteSheet; };


    class AssetManager{
        public:
            struct AssetHandle { std::string key; AssetType type;};

            ~AssetManager() = default;
            AssetManager(const AssetManager&) = delete;
            AssetManager& operator=(const AssetManager&) = delete;

            const TextureIMG* addTexture(std::string key, std::string path);
            const AudioClip* addAudioClip(std::string key, std::string path, AudioClip::AudioType type);
            const FontAtlas* addFontAtlas(std::string key, std::string path, int baseSize);

            const RenderShader* addShader(std::string key, std::string verPath, std::string fragPath);
            const RenderShader* addShaderFromMemory(std::string key, std::string verPath, std::string fragPath);

            const SpriteSheet* addSpriteSheet(std::string key, const TextureIMG* tex, Vect2 sprSize, int numSprs, int spacing=0, Vect2 startPos={0.0f}, int wrapX=0);
            const SpriteSheet* addSpriteSheet(std::string key, const TextureIMG* tex, const std::vector<Rect>& sources);

            const Sprite* getTexSprite(std::string key);
            const Sprite* getTexSpriteByPath(std::string path);
            const Sprite* getSpriteFromSheet(std::string sheet, uint16_t index);

            const std::vector<std::string> getTextureNames() const;
            const std::vector<std::string> getSpriteSheetNames() const;
            const std::vector<std::string> getAudioNames() const;
            const std::vector<std::string> getFontNames() const;
            const std::vector<std::string> getSfxNames() const;
            const std::vector<std::string> getMusicNames() const;

            bool saveAssetsRefToFile(std::string path);
            bool loadAssetsFromRefFile(std::string path);

        private:
            AssetManager(){}
            void init();
            void terminate();

            std::string serialize();
            void deserialize(const std::string& rawJson);

            void flushDeletionQueue();

            std::unordered_map<std::string, Sprite> texSprites;
            std::unordered_map<std::string, TextureIMG> textures;
            std::unordered_map<std::string, std::string> texPathToKey;

            std::unordered_map<std::string, FontAtlas> fonts;
            std::unordered_map<std::string, std::string> fontPathToKey;

            std::unordered_map<std::string, AudioClip> audios;
            std::unordered_map<std::string, std::string> audioPathToKey;
            
            std::vector<std::string> sfx;
            std::vector<std::string> music;
            
            std::unordered_map<std::string, SpriteSheet> spriteSheets;
            std::unordered_map<std::string, RenderShader> shaders;

            std::vector<AssetHandle> deletionQueue;

            friend class Engine;
        
        public:
            template<typename T>
            inline const T* get(const std::string& key) {
                constexpr AssetType type = AssetMapType<T>::type;

                int typeIdx = static_cast<int>(type);

                if constexpr (type == AssetType::Texture) {
                    auto tex_exist = textures.find(key);
                    if (tex_exist != textures.end()) return &(tex_exist->second); 
                } 
                else if constexpr (type == AssetType::Font) {
                    auto font_exist = fonts.find(key);
                    if (font_exist != fonts.end()) return &(font_exist->second); 
                }
                else if constexpr (type == AssetType::Audio) {
                    auto aud_exist = audios.find(key);
                    if (aud_exist != audios.end()) return &(aud_exist->second); 
                }
                else if constexpr (type == AssetType::SpriteSheet) {
                    auto sheet_exist = spriteSheets.find(key);
                    if (sheet_exist != spriteSheets.end()) return &(sheet_exist->second);
                }
                else if constexpr (type == AssetType::Shader){
                    auto shader_exist = shaders.find(key);
                    if (shader_exist != shaders.end()) return &(shader_exist->second);
                }
                
                static std::vector<std::string> types = {"Texture", "Audio", "Font", "Shader", "SpriteSheet"};
                PBOX_ERROR("ASSET_MAN: Cannot find %s '%s'", types[typeIdx].c_str(), key.c_str());
                return nullptr;
            }

            template<typename T>
            inline const T* getByPath(const std::string& path) {
                constexpr AssetType type = AssetMapType<T>::type;

                if constexpr (type == AssetType::Texture) {
                    auto key_exist = texPathToKey.find(path);
                    if (key_exist != texPathToKey.end()) return get<T>(key_exist->second);
                } 
                else if constexpr (type == AssetType::Font) {
                    auto font_exist = fontPathToKey.find(path);
                    if (font_exist != fontPathToKey.end()) return get<T>(font_exist->second);
                }
                else if constexpr (type == AssetType::Audio) {
                    auto audio_exist = audioPathToKey.find(path);
                    if (audio_exist != audioPathToKey.end()) return get<T>(audio_exist->second);
                } 
                else {
                    PBOX_ERROR("ASSET_MAN: Path-based lookup not supported for this asset type.");
                    return nullptr;
                }
                
                PBOX_ERROR("ASSET_MAN: Cannot find asset with path '%s'", path.c_str());
                return nullptr;
            }

            template<typename T>
            inline bool doExist(const std::string& key) {
                constexpr AssetType type = AssetMapType<T>::type;

                if constexpr (type == AssetType::Texture)     return textures.count(key) > 0;
                if constexpr (type == AssetType::Font)        return fonts.count(key) > 0;
                if constexpr (type == AssetType::Audio)       return audios.count(key) > 0;
                if constexpr (type == AssetType::SpriteSheet) return spriteSheets.count(key) > 0;
                if constexpr (type == AssetType::Shader)      return shaders.count(key) > 0;
                
                return false;
            }

            template<typename T>
            inline void enqueueRemoval(const std::string& key){
                if(!doExist<T>(key)) return;
                deletionQueue.push_back({key, AssetMapType<T>::type});
            }
    };
}

