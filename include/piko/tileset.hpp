// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <string>
#include <vector>

#include "math.hpp"

namespace piko{
    struct Sprite;
    class SpriteSheet;

    struct TileFrames {
        std::vector<const Sprite*> frames; 
        float frameDuration = 0.1f; 
    };

    // System built-in bitwise attributes for tiles, used in physics and rendering.
    enum TILEATTRIB : uint8_t {
        NONE = 0,
        ANIMATED = 1 << 0, 
        SOLID = 1 << 1,
        TRIGGER = 1 << 2,
        FRICTION = 1 << 3,
        SLOPE = 1 << 4
    };

    // Tile data representing an unique tile with its own identifier, tag, attributess and sprite.
    struct Tile{
        uint16_t id = 0;
        uint8_t tag = 0;  
        uint8_t attrib = TILEATTRIB::NONE;
        const Sprite* spr = nullptr;
        std::vector<TileFrames> frames;
    };

    class TileSet{
        public:
            TileSet(const std::string& name, std::vector<Tile> tiles);
            TileSet(const std::string& name, const SpriteSheet& spr, int count, int startIdx = 0);
            ~TileSet();

            const Tile* getTile(uint16_t id) const;
            int getSize() const;

            void modifyTag(Tile& tile);
            void modifyAttrib(Tile& tile);
        private:
            std::string name = "";
            std::vector<Tile> tiles;

    };

}