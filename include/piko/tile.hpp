#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

#include "piko/math.hpp"

namespace piko{

    class Engine;
    class Renderer;

    struct PairHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    enum TILEATTRIB : uint8_t {
        NONE = 0,
        ANIMATED = 1 << 0,
        SOLID = 1 << 1,
        TRIGGER = 1 << 2,
        FRICTION = 1 << 3,
        SLOPE = 1 << 4
    };

    struct Tile{
        uint16_t index = 0;
        uint8_t tag = 0;
        uint8_t attrib = TILEATTRIB::NONE;
    };

    using TileLayer = std::unordered_map<std::pair<int32_t, int32_t>, Tile, PairHash>;

    class TileManager{
        public:
            ~TileManager();

            void setTileSize(uint8_t size);

            void loadMap(const std::string& rawJSON);
            void loadLayer(const std::string& rawJSON, uint8_t layer);

            std::pair<int32_t, int32_t> wCoordsToGrid(Vect2 wCoords) const;

            const Tile* getTile(int32_t x, int32_t y, size_t layerIndex = 0) const;
            void setTile(int32_t x, int32_t y, const Tile& t);
            void removeTile(int32_t x, int32_t y);

            void draw(Renderer& renderer);
        
        protected:
            TileManager();

            friend class Engine;
        private:
            uint8_t size = 32;
            
            std::vector<TileLayer> layers;

    };

}