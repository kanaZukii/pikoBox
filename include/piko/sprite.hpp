#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "piko/math.hpp"

struct Texture; 
struct Image;
typedef struct Texture Texture2D;

namespace piko {
    class TextureIMG{
        public:
            TextureIMG(const std::string& filepath);
            TextureIMG(Image rlImg, const std::string& filepath);
            ~TextureIMG();

            TextureIMG(const TextureIMG&) = delete;
            TextureIMG& operator=(const TextureIMG&) = delete;

            TextureIMG(TextureIMG&& other) noexcept;
            TextureIMG& operator=(TextureIMG&& other) noexcept;

            bool operator==(const TextureIMG& other) const;

            inline int getWidth() const noexcept {return width;}
            inline int getHeight() const noexcept {return height;}

            inline float getInvWidth() const noexcept {return invW;}
            inline float getInvHeight() const noexcept {return invH;}

            inline const std::string& getFilePath() const noexcept {return path;}
            inline const Texture2D& getData() const noexcept {return *data;}
            unsigned int getOpenGLID() const;


        private:
            Texture2D* data = nullptr;
            std::string path = "";
            int width = 0;
            int height = 0;
            float invW = 0.0f;
            float invH = 0.0f;
    };


    struct Sprite{
        const TextureIMG* tex = nullptr;
        Rect source = {0.0f};
        std::string sheet = "";
        int index = -1;
        Vect2 pivot = {0.0f, 0.0f};
    };

    class SpriteSheet{
        public:
            SpriteSheet(const std::string& name, const TextureIMG* tex, const std::vector<Rect>& sources);
            
            const Sprite* getSprite(uint16_t index) const;

            inline uint16_t getSize() const noexcept { return static_cast<uint16_t>(sprites.size()); }
            inline const std::vector<Rect>& getSources() const noexcept { return sources; }
            inline const TextureIMG* getTexAtlas () const noexcept { return texAtlas; }
            inline const std::string& getName() const noexcept { return name; }

            std::string serialize();

        private:
            const TextureIMG* texAtlas;
            const std::vector<Rect> sources;
            std::vector<Sprite> sprites;
            std::string name = "";
    };
}
