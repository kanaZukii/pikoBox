// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include "piko/math.hpp"

#include <array>
#include <vector>
#include <string>

namespace piko {
    class TextureIMG;

    // FontGlyph stores the layout metrics for a single character in a font.
    struct FontGlyph {
        int value;        // Unicode char value
        Rect source;      // Source rectangle in the texture atlas
        int offsetX;      // Horizontal offset from cursor to glyph origin
        int offsetY;      // Vertical offset from cursor to glyph origin
        int advanceX;     // How many pixels to advance the cursor for next char
    };

    // FontAtlas asset. Holds a bitmap font data and its metrics.
    class FontAtlas {
        public:
            // Loads a font from a filepath with its specified base size
            FontAtlas(const std::string& filepath, int baseSize);
            ~FontAtlas();

            // 
            FontAtlas(const FontAtlas&) = delete;
            FontAtlas& operator=(const FontAtlas&) = delete;

            FontAtlas(FontAtlas&& other) noexcept;
            FontAtlas& operator=(FontAtlas&& other) noexcept;

            // Returns the underlying TextureIMG atlas for rendering.
            const TextureIMG* getTexture() const { return texture; }

            // Returns the font's intended rendering size in pixels.
            inline int getBaseSize() const { return baseSize; } 

            // Retrieves metrics for a specific character. Returns nullptr if unsupported.
            const FontGlyph* getGlyph(int asciiValue) const;

            std::string getFilePath() const {return path;}

        private:
            const TextureIMG* texture = nullptr;

            // Cache for standard ASCII range (0-255)
            std::array<FontGlyph, 256> glyphs;
            std::string path = "";
            int baseSize = 0;
            int glyphCount = 0;
    };
}