#include "piko/tile.hpp"
#include "piko/renderer.hpp"
#include "piko/sprite.hpp"
#include "piko/cam.hpp"

#include <fstream>

#include "json.hpp"

using json = nlohmann::json;
using namespace piko;

TileManager::TileManager(){
    layers.resize(256);
    tileSets.resize(256);
}

TileManager::~TileManager(){
    layers.clear();
    tileSets.clear();
}

void TileManager::setTileSize(uint8_t size){
    if(size < 1){size = 1;}
    tileSize = size;
}

void TileManager::setLayerTileSet(const TileSet* tileset, uint8_t layer){
    tileSets[layer] = tileset;
}

const TileSet* TileManager::getLayerTileSet(uint8_t layer){
    return tileSets.at(layer);
}

bool TileManager::loadMap(const std::string& filepath){
    std::ifstream inFile(filepath);
}

bool TileManager::loadLayer(const std::string& filepath, uint8_t layer){
    std::ifstream inFile(filepath);
    
}

bool TileManager::saveMap(const std::string& filepath){
    std::ofstream outFile(filepath);
}

bool TileManager::saveLayer(const std::string& filepath){
    std::ofstream outFile(filepath);
}

std::pair<int32_t, int32_t> TileManager::wCoordsToGrid(const Vect2& wCoords) const {
    int32_t x = static_cast<int32_t>(std::floor(wCoords.x / static_cast<float>(tileSize)));
    int32_t y = static_cast<int32_t>(std::floor(wCoords.y / static_cast<float>(tileSize)));

    return {x, y};
}

Vect2 TileManager::gridToWCoords(int32_t gx, int32_t gy) const {
    float x = static_cast<float>(gx) * static_cast<float>(tileSize);
    float y = static_cast<float>(gy) * static_cast<float>(tileSize);
    
    return Vect2{x, y};
}

const Tile* TileManager::getTile(int32_t x, int32_t y, uint8_t layer) const {
    const TileLayer& l = layers.at(layer);
    const TileSet* ts = tileSets.at(layer);
    if(l.empty() || !ts) {return nullptr;}
    
    auto it = l.find({x, y});
    if(it != l.end()) {return ts->getTile(it->second);} 
    return nullptr;
}

bool TileManager::setTile(const Tile& t, int32_t x, int32_t y, uint8_t layer){
    TileLayer& l = layers.at(layer);
    const TileSet* ts = tileSets.at(layer);
    if(t.id == 0 || !ts){ return false; }
    
    if(t.id <= ts->getLastTileID()){
        l[{x,y}] = t.id;
        return true;
    }

    return false;
}

bool TileManager::removeTile(int32_t x, int32_t y, uint8_t layer){
    TileLayer& l = layers.at(layer);
    if(l.empty()) {return false;}
    
    auto it = l.find({x, y});
    if(it != l.end()) {
        l.erase(it);
        return true;
    } 
    return false;
}

void TileManager::draw(Renderer& renderer){
    Cam* camera = renderer.getActiveCam();
    if(!camera){return;}

    const Rect viewArea = camera->getViewSpaceBubble();
    std::pair<int32_t, int32_t> startGrid = wCoordsToGrid({viewArea.x, viewArea.y});
    std::pair<int32_t, int32_t> endGrid = wCoordsToGrid({viewArea.x + viewArea.w, viewArea.y + viewArea.h});
    
    for(int i = 0; i < layers.size(); ++i){
        const TileLayer& l = layers.at(i);
        const TileSet* ts = tileSets.at(i);
        
        if(l.empty() || !ts){continue;}

        for(int32_t y = startGrid.second; y <= endGrid.second; ++y){

            for(int32_t x = startGrid.first; x <= endGrid.first; ++x){
                const TextureIMG* tex = nullptr;
                Rect src = {0.0f};

                auto it = l.find({x, y});

                if(it == l.end()) { continue; }

                const Tile* t = ts->getTile(it->second);

                if(t && t->sprite){
                    tex = t->sprite->tex;
                    src = t->sprite->source;
                }

                Vect2 wCoords = gridToWCoords(x, y);

                renderer.draw(
                    tex, 
                    src, 
                    {wCoords.x, wCoords.y, (float)tileSize, (float)tileSize},
                    {0.0f, 0.0f},
                    {255, 255, 255, 255},
                    0.0f,
                    i
                );

            }

        }
    }
}

