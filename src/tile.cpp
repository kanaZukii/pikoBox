#include "piko/tile.hpp"
#include "piko/renderer.hpp"

#include "json.hpp"

using json = nlohmann::json;
using namespace piko;

TileManager::TileManager(){

}

TileManager::~TileManager(){
    
}

void TileManager::setTileSize(uint8_t size){

}

void TileManager::setLayerTileSet(const TileSet&, uint8_t layer){

}

const TileSet* TileManager::getLayerTileSet(uint8_t layer){

}

void TileManager::loadMap(const std::string& rawJSON){

}

void TileManager::loadLayer(const std::string& rawJSON, uint8_t layer){

}

void TileManager::saveMap(const std::string& filepath){

}

void TileManager::saveLayer(const std::string& filepath){

}

std::pair<int32_t, int32_t> TileManager::wCoordsToGrid(Vect2 wCoords) const {

}

const Tile* TileManager::getTile(int32_t x, int32_t y, uint8_t layer = 0) const {

}

void TileManager::setTile(int32_t x, int32_t y, const Tile& t){

}

void TileManager::removeTile(int32_t x, int32_t y){

}

void TileManager::draw(Renderer& renderer){
    
}

