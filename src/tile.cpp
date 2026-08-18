#include "piko/tile.hpp"
#include "piko/renderer.hpp"
#include "piko/sprite.hpp"
#include "piko/cam.hpp"
#include "piko/logger.hpp"

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

void TileManager::setLayerTileSet(uint8_t layer, const TileSet* tileset){
    tileSets[layer] = tileset;
}

const TileSet* TileManager::getLayerTileSet(uint8_t layer){
    return tileSets.at(layer);
}

std::string TileManager::serializeLayer(uint8_t layer){
    const TileLayer& l = layers.at(layer);
    const TileSet* ts = tileSets.at(layer);

    if(l.empty() || !ts){return "{}";}

    json layerData = json::array();

    for(const auto& [coord, tileId] : l){
        const auto& [x, y] = coord; 
        
        layerData.push_back(json::object({
            {"x", x},
            {"y", y},
            {"id", tileId}
        }));
    }

    json data = {
        {"data", std::move(layerData)},
        {"tileset", ts->getName()}
    };

    return data.dump();
}

bool TileManager::loadLayer(uint8_t layer, const std::string& rawJSON){
    try{
        json jData = json::parse(rawJSON);
        TileLayer& l = layers.at(layer);

        if(jData.contains("data") &&  jData["data"].is_array()){
            l.clear();

            for (const auto& tJson : jData["data"]) {
                int32_t x = tJson.value("x", static_cast<int32_t>(0));
                int32_t y = tJson.value("y", static_cast<int32_t>(0));
                uint16_t id = tJson.value("id", static_cast<uint16_t>(0));
                
                if(id > 0){ l[{x,y}] = id; }
            }

            PBOX_INFO("TILE_MAN: Successfully loaded map layer data to layer %d.", layer);
            return true;
        }
        PBOX_ERROR("TILE_MAN: Cannot load map layer to layer %d. Missing 'data' field from JSON file.", layer);
        return false;
    } catch (const std::exception& e) {
        PBOX_ERROR("TILE_MAN: Map layer loading failed. ERROR: '%s'", e.what());
        return false;
    }
}

bool TileManager::loadLayer(uint8_t layer, TileLayer data){
    layers[layer] = std::move(data);
    return true;
}

bool TileManager::saveMapToFile(const std::string& filepath){
     try {
        
        std::ofstream outfile(filepath);
        
        if (!outfile.is_open()) {
            PBOX_ERROR("TILE_MAN: Serialization failed. Could not open or create file at '%s'", filepath.c_str());
            return false;
        }


        json mapLayers = json::object();

        for(int i = 0; i < layers.size(); ++i){
            const TileLayer& l = layers.at(i);
            const TileSet* ts = tileSets.at(i);

            if(l.empty() || !ts){continue;}

            mapLayers[std::to_string(i)] = json::parse(serializeLayer(static_cast<uint8_t>(i)));
        }

        json mapData = {
            {"layers", mapLayers},
            {"tileSize", tileSize}
        };

        outfile << mapData.dump(4);

        outfile.close();
        PBOX_INFO("TILE_MAN: Successfully saved map data to '%s'", filepath.c_str());
        return true;

    } catch (const std::exception& e) {
        PBOX_ERROR("TILE_MAN: Serialization failed. ERROR: '%s'", e.what());
        return false;
    }
}

bool TileManager::loadMapFromFile(const std::string& filepath){
    std::ifstream inFile(filepath);
    
    // Check if the file exists and is accessible
    if (!inFile.is_open()) {
        PBOX_ERROR("TILE_MAN: Deserialization failed. Could not open file at '%s'", filepath.c_str());
        return false;
    }

    try {
        json mapData;
        inFile >> mapData;
        inFile.close();

        if (!mapData.contains("layers")) {
            PBOX_ERROR("TILE_MAN: Deserialization failed. Missing 'layers' field inside tilemap JSON.");
            return false;
        } else if (!mapData["layers"].is_object()){
            PBOX_ERROR("TILE_MAN: Deserialization failed. Layers field is in the wrong format inside tilemap JSON.");
            return false;
        }

        for (const auto& [layerZ, layerData] : mapData["layers"].items()) {
            int tmp_z = std::stoi(layerZ);
            if (tmp_z < 0 || tmp_z > 255) {
                PBOX_WARN("TILE_MAN: Deserializing... Skipping layer %d... Invalid layer Z index must be within 0-255", tmp_z);
                continue;
            }
            uint8_t z = static_cast<uint8_t>(tmp_z);

            if(!layerData.contains("tileset")){
                PBOX_WARN("TILE_MAN: Deserializing layer %d... 'tileset' field is missing. Ignoring for now, please set it manually.", z);
            } else {
                
            }

            loadLayer(z, layerData.dump());
        }

       return true;

    } catch (const json::parse_error& e) {
        PBOX_ERROR("TILE_MAN: Deserialization failed. Load Parse Error in file '%s': %s", filepath.c_str(), e.what());
        return false;
    } catch (const std::exception& e) {
        PBOX_ERROR("TILE_MAN: Deserialization failed. Error: %s", e.what());
        return false;
    }
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

