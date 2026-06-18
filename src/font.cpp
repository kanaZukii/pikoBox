#include "piko/font.hpp"
#include "piko/sprite.hpp"
#include "piko/logger.hpp"

#include "raylib.h"

using namespace piko;

FontAtlas::FontAtlas(const std::string& filepath, int baseSize) : baseSize(baseSize) {
    // 1. Zero out / initialize the entire glyph array safely
    for (size_t i = 0; i < glyphs.size(); ++i) {
        glyphs[i] = FontGlyph{ 0, Rect{0, 0, 0, 0}, 0, 0, 0 };
    }

    // 2. Load the raw font file bytes using Raylib
    int dataSize = 0;
    unsigned char* fileData = LoadFileData(filepath.c_str(), &dataSize);
    if (!fileData) {
        PBOX_ERROR("FONT_ATLAS: Failed to open font asset at '%s'.", filepath.c_str());
        return;
    }
    
    path = filepath;

    // We target standard printable ASCII (95 characters, from space '32' to '~' 126)
    int targetGlyphCount = 95;
    this->glyphCount = targetGlyphCount;

    std::vector<int> fontChars(targetGlyphCount);
    for (int i = 0; i < targetGlyphCount; i++) fontChars[i] = 32 + i;

    // 3. Extract glyph metrics via Raylib
    GlyphInfo* raylibGlyphs = LoadFontData(fileData, dataSize, baseSize, fontChars.data(), targetGlyphCount, FONT_DEFAULT, &targetGlyphCount);

    // 4. Bake character shapes into a single image atlas sheet
    Rectangle* paddingRecs = nullptr;
    Image atlasImage = GenImageFontAtlas(raylibGlyphs, &paddingRecs, targetGlyphCount, baseSize, 0, 1);

    // 5. Instantiating via the raw Image constructor
    // We append a pseudo suffix like ":atlas" so the texture path remains unique inside VRAM tracking
    this->texture = new TextureIMG(atlasImage, filepath + ":atlas"); 

    // Free CPU side pixels now that VRAM allocation is finished
    UnloadImage(atlasImage);

    // 6. Map parsed layout metrics straight into their exact ASCII indices in our flat array
    for (int i = 0; i < targetGlyphCount; i++) {
        int codepoint = raylibGlyphs[i].value;

        if (codepoint < 0 || codepoint >= 256) continue;

        FontGlyph glyph;
        glyph.value = codepoint;
        glyph.source = {
            paddingRecs[i].x,
            paddingRecs[i].y,
            paddingRecs[i].width,
            paddingRecs[i].height
        };
        glyph.offsetX = raylibGlyphs[i].offsetX;
        glyph.offsetY = raylibGlyphs[i].offsetY;
        glyph.advanceX = raylibGlyphs[i].advanceX;

        glyphs[codepoint] = glyph;
    }

    UnloadFontData(raylibGlyphs, targetGlyphCount);
    if (paddingRecs != nullptr) {
        MemFree(paddingRecs); 
    }
    UnloadFileData(fileData);
}

 FontAtlas::~FontAtlas(){
    if(texture) delete texture;
 }

 // Move Constructor
FontAtlas::FontAtlas(FontAtlas&& other) noexcept 
    : texture(other.texture),
      glyphs(std::move(other.glyphs)),
      baseSize(other.baseSize),
      glyphCount(other.glyphCount)
{
    other.texture = nullptr;
    other.baseSize = 0;
    other.glyphCount = 0;
}

// Move Assignment Operator
FontAtlas& FontAtlas::operator=(FontAtlas&& other) noexcept {
    if (this != &other) {
        if (texture) delete texture; 

        texture = other.texture;
        glyphs = std::move(other.glyphs);
        baseSize = other.baseSize;
        glyphCount = other.glyphCount;

        other.texture = nullptr;
        other.baseSize = 0;
        other.glyphCount = 0;
    }
    return *this;
}

const FontGlyph* FontAtlas::getGlyph(int asciiValue) const {
    if (asciiValue < 0 || asciiValue >= 256 || glyphs[asciiValue].value == 0) {
        // Safe check: If '?' is valid, return it. Otherwise, return the first valid asset we can find (like Space at 32)
        if (glyphs['?'].value != 0) return &glyphs['?'];
        if (glyphs[32].value != 0)  return &glyphs[32];
        
        // Absolute emergency fallback to avoid uninitialized memory reads or recursion
        return &glyphs[0]; 
    }
    return &glyphs[asciiValue];
}