#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

#include "piko/math.hpp"
#include "piko/sprite.hpp"
#include "piko/shader.hpp"
#include "piko/audioClip.hpp"
#include "piko/font.hpp"

namespace piko {
    class AssetManager{
        public:
            ~AssetManager() = default;
            AssetManager(const AssetManager&) = delete;
            AssetManager& operator=(const AssetManager&) = delete;

            static std::string ReadFileToString(const std::string& path);

            const RenderShader* addShader(std::string key, std::string verPath, std::string fragPath);
            const RenderShader* addShaderFromMemory(std::string key, std::string verPath, std::string fragPath);
            const RenderShader* getShader(std::string key);

            const AudioClip* addAudioClip(std::string key, std::string path, AudioClip::AudioType type);
            const AudioClip* getAudioClip(std::string key);
            const AudioClip* getAudioClipByPath(std::string path);

            const FontAtlas* addFontAtlas(std::string key, std::string path, int baseSize);
            const FontAtlas* getFontAtlas(std::string key);
            const FontAtlas* getFontAtlasByPath(std::string path);

            const TextureIMG* addTexture(std::string key, std::string path);
            const TextureIMG* getTexture(std::string key);
            const TextureIMG* getTextureByPath(std::string path);

            const Sprite* getTexSprite(std::string key);
            const Sprite* getTexSpriteByPath(std::string path);

            const SpriteSheet* addSpriteSheet(std::string key, const TextureIMG* tex, Vect2 sprSize, int numSprs, int spacing=0, Vect2 startPos={0.0f}, int wrapX=0);
            const SpriteSheet* addSpriteSheet(std::string key, const TextureIMG* tex, const std::vector<Rect>& sources);
            const SpriteSheet* getSpriteSheet(std::string key);
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

            friend class Engine;
    };
}

