#include "piko/tile.hpp"
#include "piko/renderer.hpp"

#include "json.hpp"

using json = nlohmann::json;
using namespace piko;

TileManager::TileManager(){

}

void TileManager::draw(Renderer& renderer){

}

void TileManager::setTileSize(uint8_t size){

}

void TileManager::loadMap(const std::string& rawJSON){

}

void TileManager::loadLayer(const std::string& rawJSON, uint8_t layer){

}

std::pair<int32_t, int32_t> TileManager::wCoordsToGrid(Vect2 wCoords) const {

}

const Tile* TileManager::getTile(int32_t x, int32_t y, size_t layerIndex = 0) const {

}

void TileManager::setTile(int32_t x, int32_t y, const Tile& t){

}

void TileManager::removeTile(int32_t x, int32_t y){

}

