// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "piko/math.hpp"

// Forward declare raylib's structs that are important for Images and Textures.
// Allow us to avoid including the entirity of raylib in our headers.
struct Texture; 
struct Image;
typedef struct Texture Texture2D;

namespace piko {
    // TextureIMG asset. Holds GPU texture data from a loaded image.
    class TextureIMG{
        public:
            // Loads an image from the specified path and creates a GPU texture.
            TextureIMG(const std::string& filepath);

            // Wraps an existing raylib Image into a GPU texture.
            TextureIMG(Image rlImg, const std::string& filepath);

            // Ensures GPU texture resources are released upon destruction.
            ~TextureIMG();

            // TextureIMG is non-copyable to prevent accidental GPU resource duplication.
            TextureIMG(const TextureIMG&) = delete;
            TextureIMG& operator=(const TextureIMG&) = delete;

            // TextureIMG proper move semantics.
            TextureIMG(TextureIMG&& other) noexcept;
            TextureIMG& operator=(TextureIMG&& other) noexcept;

            // Compares textures based on their internal file path/data identity.
            bool operator==(const TextureIMG& other) const;

            // Compares textures based on their internal file path/data identity.
            inline int getWidth() const noexcept {return width;}
            inline int getHeight() const noexcept {return height;}

            // Precomputed inverse dimensions (1/width, 1/height) for optimized shader/math calculations.
            inline float getInvWidth() const noexcept {return invW;}
            inline float getInvHeight() const noexcept {return invH;}

            // Returns the source file path used to load this asset.
            inline const std::string& getFilePath() const noexcept {return path;}

            // Returns the underlying raylib texture data.
            inline const Texture2D& getData() const noexcept {return *data;}

            // Returns the raw OpenGL texture ID for custom binding.
            unsigned int getOpenGLID() const;


        private:
            Texture2D* data = nullptr;
            std::string path = "";
            int width = 0;
            int height = 0;
            float invW = 0.0f;
            float invH = 0.0f;
    };

    /*
        Sprite data struct used for rendering. 
        Holds a reference to its texture atlas, source region and SpriteSheet info.
        Must be owned by a SpriteSheet asset.
    */ 
    struct Sprite{
        const TextureIMG* tex = nullptr;    // TextureIMG atlas reference, ownership remains with AssetManager.
        Rect source = {0.0f};               // Pixel region in the texture atlas.
        std::string sheet = "";             // Identifier for the associated SpriteSheet.
        int index = -1;                     // Sprite's frame/cell index from a SpriteSheet.
        Vect2 pivot = {0.0f, 0.0f};         // Normalized pivot point
    };

    /*
        SpriteSheet asset. Contains a collection of sprites derived from a single texture atlas.
    */
    class SpriteSheet{
        public:
            // Constructs a sheet from a name, a TextureIMG atlas pointer, and a list of source rectangles.
            SpriteSheet(const std::string& name, const TextureIMG* tex, const std::vector<Rect>& sources);
            
            /*
                Returns a pointer to the sprite at the given index. 
                Returns nullptr if index is out of bounds.
            */
            const Sprite* getSprite(uint16_t index) const;

            // Returns the total number of sprites currently defined in this sheet.
            inline uint16_t getSize() const noexcept { return static_cast<uint16_t>(sprites.size()); }

            // Returns the list of source rectangles used to define the frames of this sheet.
            inline const std::vector<Rect>& getSources() const noexcept { return sources; }

            // Returns the reference to the underlying TextureIMG atlas.
            inline const TextureIMG* getTexAtlas () const noexcept { return texAtlas; }

            // Returns the unique identifier/name for this sheet.
            inline const std::string& getName() const noexcept { return name; }

            // Serializes the sheet and its sprites to a JSON string
            std::string serialize();

        private:
            const TextureIMG* texAtlas;         // Non-owning reference to the atlas TextureIMG.
            const std::vector<Rect> sources;    // Spatial data for each sprite frame.
            std::vector<Sprite> sprites;        // List of Sprite structs that belong to this sheet.
            std::string name = "";
    };
}
