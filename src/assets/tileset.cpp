#include "piko/tileset.hpp"
#include "piko/sprite.hpp"

#include <algorithm>

#include "json.hpp"

using json = nlohmann::json;
using namespace piko;

TileSet::TileSet(const std::string& name, std::vector<Tile> tiles){
    if(tiles.empty() || name.empty()){
        throw std::runtime_error("Invalid constructor parameters for name and tiles.");
    }
    
    this->name = name;
    this->tiles = tiles;

    std::stable_sort(this->tiles.begin(), this->tiles.end(), [](const Tile& a, const Tile& b) {
        return a.id < b.id;
    });

    for(int i = 0; i < this->tiles.size(); ++i){
        uint16_t id = this->tiles[i].id;
        if(id != i){
            throw std::runtime_error("A Tile's ID is out of order.");
        }
    }
}

TileSet::~TileSet(){

}

const Tile* TileSet::getTile(uint16_t id) const{
    if(id > tiles.size() - 1){ return nullptr; }
    return &tiles.at(id);
}

uint16_t TileSet::getLastTileID() const {
    if(tiles.empty()) return 0;
    return tiles.back().id;
}

int TileSet::getSize() const {
    return tiles.size();
}

bool TileSet::setTag(uint16_t id, uint8_t tag) {
    if(id >= tiles.size()){ return false; }
    tiles[id].tag = tag;
    return true;
}

bool TileSet::setAttrib(uint16_t id, uint8_t attrib){
    if(id >= tiles.size()){ return false; }
    tiles[id].attrib = attrib;
    return true;
}

std::string TileSet::serialize(){
    json tilesJSON = json::array();
    for(const Tile& t : tiles){
        json tile = { {"id", t.id}, {"tag", t.tag}, {"attrib", t.attrib}};
        const Sprite* sprite = t.sprite;

        if(!t.animation.empty()){
            sprite = t.animation.at(0).sprite;
            json animJSON = json::array();
            for(const TileFrame& f : t.animation){
                json fJSON = {{"duration", f.duration}};
                if(f.sprite){
                    fJSON["sprite"] = {{"sheet", f.sprite->sheet}, {"index", f.sprite->index}};
                }
                animJSON.push_back(fJSON);
            }
            tile["animation"] = animJSON;
        }

        if(sprite) {tile["sprite"] = {{"sheet", sprite->sheet}, {"index", sprite->index}};}

        tilesJSON.push_back(tile);
    }

    json data = {
        {"sources", tilesJSON},
        {"name", name}
    };

    return data.dump();
}