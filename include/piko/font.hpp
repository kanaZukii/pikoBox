// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include "piko/math.hpp"

#include <array>
#include <vector>
#include <string>

namespace piko {
    class TextureIMG;

    struct FontGlyph {
        int value;        // Unicode char value
        Rect source;      // Source rectangle in the texture atlas
        int offsetX;      // Horizontal offset from cursor to glyph origin
        int offsetY;      // Vertical offset from cursor to glyph origin
        int advanceX;     // How many pixels to advance the cursor for next char
    };

    class FontAtlas {
        public:
            FontAtlas(const std::string& filepath, int baseSize);
            ~FontAtlas();

            FontAtlas(const FontAtlas&) = delete;
            FontAtlas& operator=(const FontAtlas&) = delete;

            FontAtlas(FontAtlas&& other) noexcept;
            FontAtlas& operator=(FontAtlas&& other) noexcept;

            const TextureIMG* getTexture() const { return texture; }
            inline int getBaseSize() const { return baseSize; } 
            const FontGlyph* getGlyph(int asciiValue) const;

            std::string getFilePath() const {return path;}

        private:
            const TextureIMG* texture = nullptr;
            std::array<FontGlyph, 256> glyphs;
            std::string path = "";
            int baseSize = 0;
            int glyphCount = 0;
    };
}