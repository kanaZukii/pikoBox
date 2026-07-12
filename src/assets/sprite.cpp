#include "piko/sprite.hpp"

#include "raylib.h"
#include "json.hpp"

#include <filesystem>

using json = nlohmann::json;
using namespace piko;

TextureIMG::TextureIMG(const std::string& filepath){
    // Check if file exists on disk
    if (!FileExists(filepath.c_str())) {
        throw std::runtime_error("TEXTUREIMG: File does not exist: " + filepath);
    }

    Image img = LoadImage(filepath.c_str());
    
    // Raylib check if image actually load pixels
    if (img.data == nullptr) {
        throw std::runtime_error("TEXTUREIMG: Failed to load image data: " + filepath);
    }
    
    data = new Texture2D;

    *data = LoadTextureFromImage(img);
    UnloadImage(img);

    width = data->width;
    height = data->height;
    path = filepath;

    invW = 1.0f / (float)width;
    invH = 1.0f / (float)height;
}

TextureIMG::TextureIMG(Image rlImg, const std::string& filepath){
    // Raylib check if image actually load pixels
    if (rlImg.data == nullptr) {
        throw std::runtime_error("TEXTUREIMG: Failed to load image data: " + filepath);
    }
    
    data = new Texture2D;

    *data = LoadTextureFromImage(rlImg);

    width = data->width;
    height = data->height;
    path = filepath;

    invW = 1.0f / (float)width;
    invH = 1.0f / (float)height;
}

TextureIMG::~TextureIMG(){
    if(data){
         if(data->id > 0){
            UnloadTexture(*data);
        }
        delete data;
    }
}

// Move Constructor
TextureIMG::TextureIMG(TextureIMG&& other) noexcept 
    : data(other.data), path(std::move(other.path)), width(other.width), height(other.height) {
    
    // Nullify the source so it doesn't unload our texture
    other.data = nullptr; 
    other.width = 0;
    other.height = 0;
}

// Move Assignment Operator
TextureIMG& TextureIMG::operator=(TextureIMG&& other) noexcept {
    if (this != &other) {
        // Swap our current (potentially loaded) data with the new one
        std::swap(data, other.data);
        std::swap(path, other.path);
        std::swap(width, other.width);
        std::swap(height, other.height);
    }
    return *this;
}

bool TextureIMG::operator==(const TextureIMG& other) const {
    return (this->path == other.path 
            && this->data->id == other.data->id
            && this->width == other.width 
            && this->height == other.height
        );
}

unsigned int TextureIMG::getOpenGLID() const {
    if(data){
        return data->id;
    }
    return 0;
}


SpriteSheet::SpriteSheet(const std::string& name, const TextureIMG* tex, const std::vector<Rect>& sources) : sources(sources), name(name) {
    texAtlas = tex;
    if (!texAtlas) throw std::runtime_error("Texture Atlas is NULLPTR");
    if (texAtlas->getData().id == 0) throw std::runtime_error("Texture Atlas is MALFORMED");

    const int texW = texAtlas->getWidth();
    const int texH = texAtlas->getHeight();

    sprites.reserve(sources.size());

    int index = 0;
    for (const Rect& src : sources) {
        if (src.x < 0 || src.y < 0 || 
            (src.x + src.w) > texW || 
            (src.y + src.h) > texH) {
            
            TraceLog(LOG_WARNING, "SPRSHEET: Skipping invalid source rect: x:%.0f y:%.0f", src.x, src.y);
            continue; 
        }

        sprites.push_back({texAtlas, src, name, index++});
    }
}

const Sprite* SpriteSheet::getSprite(uint16_t index) const {
    if(index >= sprites.size()){TraceLog(LOG_ERROR, "SPRSHEET: sheet size: '%d', cannot get sprite at index: '%d'.", sprites.size(), index); return nullptr;}
    return &sprites[index];
}

std::string SpriteSheet::serialize(){
    json srcJSON = json::array();
    for(const Rect& r : sources){
        srcJSON.push_back(
            {{"x", r.x},{"y", r.y},{"w", r.w},{"h", r.h}}
        );
    }

    json data = {
        {"texAtlas", texAtlas->getFilePath()},
        {"sources", srcJSON},
        {"name", name}
    };

    return data.dump();
}
