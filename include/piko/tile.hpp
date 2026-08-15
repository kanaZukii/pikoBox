// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

#include "piko/math.hpp"
#include "piko/tileset.hpp"

namespace piko{

    class Engine;
    class Renderer;

    // Custom hash functor for std::pair to enable coordinate based map keys.
    struct PairHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
  
    // Sparse representation of a single map layer mapping grid coordinates to a Tile Index.
    using TileLayer = std::unordered_map<std::pair<int32_t, int32_t>, uint16_t, PairHash>;

    // TileManager manages multi-layered sparse tilemaps, coordinate transformations and tile rendering.        
    class TileManager{
        public:
            ~TileManager();

            // Sets the pixel dimension size for all tiles.
            void setTileSize(uint8_t size);
            // Sets the tileset for the targeted layer (0 to 255).
            void setLayerTileSet(const TileSet&, uint8_t layer);
            // Retrieves the layer's assigned tileset.
            const TileSet* getLayerTileSet(uint8_t layer);

            // Loads all map layers from raw JSON data.
            void loadMap(const std::string& rawJSON);
            /*
                Loads or overwrites a specific map layer from raw JSON data.
                Target layer index (0 to 255).
            */ 
            void loadLayer(const std::string& rawJSON, uint8_t layer);

            // Serializes and saves the entire multi-layer map to disk.
            void saveMap(const std::string& filepath);
            // Serializes and saves a specific map layer to disk.
            void saveLayer(const std::string& filepath);

            /*
                Converts world-space coordinates into discrete grid indices.
                Takes Vect2 coordinates and returns a pair gird coordinates {x, y}.
            */
            std::pair<int32_t, int32_t> wCoordsToGrid(Vect2 wCoords) const;

            // Retrieves a pointer to the tile at the specified grid location.
            const Tile* getTile(int32_t x, int32_t y, uint8_t layer = 0) const;

            // Places or updates a tile at the given grid location.
            void setTile(int32_t x, int32_t y, const Tile& t);

            // Removes a tile from the specified grid location.
            void removeTile(int32_t x, int32_t y);

            // Renders all active visible tile layers using the provided renderer.
            void draw(Renderer& renderer);
        
        protected:
            TileManager();

            friend class Engine;
        private:
            uint8_t size = 32;
            
            std::vector<TileLayer> layers;
            std::vector<const TileSet*> tileSets;

    };

}