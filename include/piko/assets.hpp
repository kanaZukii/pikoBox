// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include "piko/math.hpp"
#include "piko/sprite.hpp"
#include "piko/shader.hpp"
#include "piko/audioClip.hpp"
#include "piko/font.hpp"
#include "piko/animation.hpp"
#include "piko/logger.hpp"

namespace piko {
    // Useful for shaders and other text file
    static std::string ReadFileToString(const std::string& path);

    // Asset categorization for template-based lookup
    enum class AssetType {Texture, Audio, Font, Shader, SpriteSheet, AnimationClip};
    
    // Trait system to map C++ classes to the AssetType enum
    template<typename T> struct AssetMapType;
    template<> struct AssetMapType<TextureIMG> { static constexpr AssetType type = AssetType::Texture; };
    template<> struct AssetMapType<AudioClip> { static constexpr AssetType type = AssetType::Audio; };
    template<> struct AssetMapType<FontAtlas> { static constexpr AssetType type = AssetType::Font; };
    template<> struct AssetMapType<RenderShader> { static constexpr AssetType type = AssetType::Shader; };
    template<> struct AssetMapType<SpriteSheet> { static constexpr AssetType type = AssetType::SpriteSheet; }; 
    template<> struct AssetMapType<AnimationClip> { static constexpr AssetType type = AssetType::AnimationClip; };

    /*
        AssetManager, centralize repository for all type of assets.
        Handles asset lifecycle via unique_ptr.
        Does not perform garbage collection automatically. 
        Dangling pointers retrieved from get<T> must be handled manually.
    */
    class AssetManager{
        public:
            struct AssetHandle { std::string key; AssetType type;};

            ~AssetManager() = default;

            // Prevent copying to ensure exclusive ownership
            AssetManager(const AssetManager&) = delete;
            AssetManager& operator=(const AssetManager&) = delete;

            // For creating new TextureIMG asset
            const TextureIMG* addTexture(std::string key, std::string path);
            // For creating new AudioClip asset
            const AudioClip* addAudioClip(std::string key, std::string path, AudioClip::AudioType type);
            // For creating new FontAtlas asset
            const FontAtlas* addFontAtlas(std::string key, std::string path, int baseSize);
            // For creating new RenderShader asset from disk
            const RenderShader* addShader(std::string key, std::string verPath, std::string fragPath);
            
            // For creating new RenderShader assets for hardcoded shaders.
            const RenderShader* addShaderFromMemory(std::string key, std::string verPath, std::string fragPath);

            // For creating new SpriteSheet asset
            const SpriteSheet* addSpriteSheet(
                std::string key, 
                const TextureIMG* tex, 
                Vect2 sprSize, 
                int numSprs, 
                int spacing=0, 
                Vect2 startPos={0.0f}, 
                int wrapX=0
            );

            // For creating new SpriteSheet asset with predefined sources
            const SpriteSheet* addSpriteSheet(std::string key, const TextureIMG* tex, const std::vector<Rect>& sources);

            // For creating new AnimationClip asset
            const AnimationClip* addAnimationClip(
                std::string key, 
                const std::vector<SpriteKey>& sprKey, 
                const std::vector<TransformKey>& tranKey, 
                const std::vector<ColorKey>& colKey
            );

            // Returns a TextureIMG in Sprite asset type
            const Sprite* getTexSprite(std::string key);
            // Returns a TextureIMG in Sprite asset type
            const Sprite* getTexSpriteByPath(std::string path);

            // Returns a Sprite from a specified sheet and index
            const Sprite* getSpriteFromSheet(std::string sheet, uint16_t index);

            // For list inspections
            const std::vector<std::string> getTextureNames() const;
            const std::vector<std::string> getSpriteSheetNames() const;
            const std::vector<std::string> getAudioNames() const;
            const std::vector<std::string> getFontNames() const;
            const std::vector<std::string> getSfxNames() const;
            const std::vector<std::string> getMusicNames() const;

            // Serialization
            bool saveAssetsRefToFile(std::string path);
            bool loadAssetsFromRefFile(std::string path);

        private:
            AssetManager(){}
            void init();
            void terminate();

            std::string serialize();
            void deserialize(const std::string& rawJson);

            // Processes flagged removals
            void flushDeletionQueue();

            std::unordered_map<std::string, std::unique_ptr<Sprite>> texSprites;
            std::unordered_map<std::string,  std::unique_ptr<TextureIMG>> textures;
            std::unordered_map<std::string, std::string> texPathToKey;

            std::unordered_map<std::string, std::unique_ptr<FontAtlas>> fonts;
            std::unordered_map<std::string, std::string> fontPathToKey;

            std::unordered_map<std::string, std::unique_ptr<AudioClip>> audios;
            std::unordered_map<std::string, std::string> audioPathToKey;

            std::unordered_set<std::string> sfx;
            std::unordered_set<std::string> music;
            
            std::unordered_map<std::string, std::unique_ptr<RenderShader>> shaders;
            std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> spriteSheets;
            std::unordered_map<std::string, std::unique_ptr<AnimationClip>> animationClips;

            std::vector<AssetHandle> deletionQueue;

            friend class Engine;
        
        public:
            // Generic getter for any asset type T
            template<typename T>
            inline const T* get(const std::string& key) {
                constexpr AssetType type = AssetMapType<T>::type;

                int typeIdx = static_cast<int>(type);

                if constexpr (type == AssetType::Texture) {
                    auto tex_exist = textures.find(key);
                    if (tex_exist != textures.end()) return tex_exist->second.get(); 
                } 
                else if constexpr (type == AssetType::Font) {
                    auto font_exist = fonts.find(key);
                    if (font_exist != fonts.end()) return font_exist->second.get(); 
                }
                else if constexpr (type == AssetType::Audio) {
                    auto aud_exist = audios.find(key);
                    if (aud_exist != audios.end()) return aud_exist->second.get(); 
                }
                else if constexpr (type == AssetType::Shader){
                    auto shader_exist = shaders.find(key);
                    if (shader_exist != shaders.end()) return shader_exist->second.get();
                }
                else if constexpr (type == AssetType::SpriteSheet) {
                    auto sheet_exist = spriteSheets.find(key);
                    if (sheet_exist != spriteSheets.end()) return sheet_exist->second.get();
                }
                else if constexpr (type == AssetType::AnimationClip) {
                    auto anim_exist = animationClips.find(key);
                    if (anim_exist != animationClips.end()) return anim_exist->second.get();
                }
                
                static std::vector<std::string> types = {"Texture", "Audio", "Font", "Shader", "SpriteSheet", "AnimationClip"};
                PBOX_ERROR("ASSET_MAN: Cannot find %s '%s'", types[typeIdx].c_str(), key.c_str());
                return nullptr;
            }

            /*
                Getter for any asset type T using its unique filepath.
                For TextureIMG, FontAtlas and AudioClip only.
            */
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

            // Checks for asset existence without triggering error logs
            template<typename T>
            inline bool doExist(const std::string& key) {
                constexpr AssetType type = AssetMapType<T>::type;

                if constexpr (type == AssetType::Texture)     return textures.count(key) > 0;
                if constexpr (type == AssetType::Font)        return fonts.count(key) > 0;
                if constexpr (type == AssetType::Audio)       return audios.count(key) > 0;
                if constexpr (type == AssetType::Shader)      return shaders.count(key) > 0;
                if constexpr (type == AssetType::SpriteSheet) return spriteSheets.count(key) > 0;
                if constexpr (type == AssetType::AnimationClip) return animationClips.count(key) > 0;
                
                return false;
            }

            // Schedules an asset for deferred removal
            template<typename T>
            inline void enqueueRemoval(const std::string& key){
                if(!doExist<T>(key)) return;
                deletionQueue.push_back({key, AssetMapType<T>::type});
            }
    };
}
