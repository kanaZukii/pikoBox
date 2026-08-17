#include "piko/tile.hpp"
#include "piko/renderer.hpp"

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
    this->size = size;
}

void TileManager::setLayerTileSet(const TileSet* tileset, uint8_t layer){
    tileSets[layer] = tileset;
}

const TileSet* TileManager::getLayerTileSet(uint8_t layer){
    return tileSets.at(layer);
}

bool TileManager::loadMap(const std::string& rawJSON){
    json data = json::parse(rawJSON);
}

bool TileManager::loadLayer(const std::string& rawJSON, uint8_t layer){
    json data = json::parse(rawJSON);

}

bool TileManager::saveMap(const std::string& filepath){
    std::ifstream inFile(filepath);
}

bool TileManager::saveLayer(const std::string& filepath){
    std::ifstream inFile(filepath);
}

std::pair<int32_t, int32_t> TileManager::wCoordsToGrid(const Vect2& wCoords) const {
    int32_t x = static_cast<int32_t>(std::floor(wCoords.x / static_cast<float>(size)));
    int32_t y = static_cast<int32_t>(std::floor(wCoords.y / static_cast<float>(size)));

    return {x, y};
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
    
}

