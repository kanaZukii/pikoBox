// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <string>

#include "piko/math.hpp"

namespace piko {
    // Forward declarations, keeps headers decoupled
    class Engine;
    class Renderer;
    class Cam;
    class TextureIMG;
    class FontAtlas;
    class RenderShader;
    
    /*
        RenderQuad contains the visual properties of a single sprite.
        Intended to be passed to the renderer to be pushed into a batch buffer.
    */
    struct RenderQuad {
        const TextureIMG* texture = nullptr;    // Reference to the texture atlas for textured sprite.
        Rect source = {0.0f};                   // Sub-region within the atlas (Pixel coordinates).
        Rect dest = {0.0f};                     // Position and size in world/screen space.
        Vect2 origin = {0.0f};                  // Rotation/scaling pivot point.
        Color4 tint = {255, 255, 255, 255};     // Color RGBA values.

        int zIndex = 0;                         // Layering order.
        float sortY = 0.0f;                     // Y-sorting for depth. Not yet implemented.
        float rotation = 0.0f;                  // Rotation in degrees.
        bool useScreenSpace = false;            // If true, ignores camera's view.
        bool clip = false;                      // Enables scissoring/clipping.
        Rect clipRegion = {0.0f};               // The boundaries for clipping.
    };

    /*
        Minimal data required for a single vertex on the GPU.
    */
    struct Vertex {
        float x, y, z;          // Position
        float r, g, b, a;       // Color
        float u, v;             // TexCoords
        float texID;            // Texture ID
    };

    /*
        RenderBatch, a buffer with a collection of sprites sharing identical visual settings.
        It packs vertices into a single VBO/VAO to reduce draw calls. 
        Must be handled by the Renderer class.
    */
    class RenderBatch {
    public:
        ~RenderBatch();
        RenderBatch(){textures.fill(nullptr);}

        // Prevent copying to avoid VAO/VBO ownership conflicts.
        RenderBatch(const RenderBatch&) = delete;
        RenderBatch& operator=(const RenderBatch&) = delete;

        // Can be moved for sorting and transfering.
        RenderBatch(RenderBatch&& other) noexcept;
        RenderBatch& operator=(RenderBatch&& other) noexcept;

        // Setup buffers (VAO, VBO, EBO).
        void init();    

        // Sets the necessary settings for batching, flips the active flag to true.
        void activate (int zIndex, bool useScreenSpace, bool clip, Rect clipRegion);

        // Clear the batch's settings so it can be recycled.
        void reset();

        // Attempts to add a quad. Returns false if batch capacity or texture limit is reached.
        bool add(const RenderQuad& rQuad);

        // Sends all accumulated vertex data to the GPU and executes a draw call.
        void flush(const RenderShader* shader, Cam* camera);


        bool isActive() const { return active; }

        int getQuadCount() const { return quadCount; }
        int getTextCount() const { return texCount; }
        
        bool hasRoom() const{ return quadCount < MAX_QUADS;}
        bool hasRoomForTexture() const{ return texCount < MAX_TEXTURES;}
        
        int getZ() const { return zIndex; }
        bool isScreenSpaceMode() const {return useScreenSpace;}
        const Rect& getClipRegion() const{ return clipRegion;}
        bool hasClipping() const{ return doClipping;}

    private:
        static constexpr int MAX_TEXTURES = 8;      // Matches the default shader's texture slots.
        static constexpr int MAX_QUADS = 1000;      // 4000 Vertices per batch.

        static constexpr int VERTEX_SIZE = 10;
        static constexpr int VERTEX_SIZE_BYTES = VERTEX_SIZE * sizeof(float);

        bool active = false;

        int texCount = 1;
        int quadCount = 0;

        uint32_t VAO = 0;
        uint32_t VBO = 0;
        uint32_t EBO = 0;

        // Batch state settings
        int zIndex = 0;
        bool useScreenSpace = false;
        bool doClipping = false;
        Rect clipRegion = {0.0f, 0.0f, 0.0f, 0.0f};

        std::array<Vertex, MAX_QUADS * 4> vertices;
        std::array<const TextureIMG*, MAX_TEXTURES> textures;
        
        // Tracking source data for batch operations.
        std::array<RenderQuad, MAX_QUADS> rQuads;
        std::array<int, MAX_QUADS> rQuadTexSlots;
        std::array<int, MAX_TEXTURES> texSlots = {0,1,2,3,4,5,6,7};

        int getTextureSlot(const TextureIMG* tex);

        // Loads RenderQuad data to a Vertex pointer
        void loadVertexProperties(const RenderQuad& rQuad, Vertex* quadPtr, int texSlot);

        std::vector<uint32_t> generateIndices();
    };

    /*
        Renderer acts as the high-level API for all draw operations.
        Abstracts the complexity of our batched rendering logic.
    */
    class Renderer {
    public:
        ~Renderer() = default;
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        
        // Queues a sprite to be rendered.
        void draw(
                const TextureIMG* tex,
                Rect texsrc,
                Rect dest,
                Vect2 origin,
                Color4 tint = {255, 255, 255, 255},
                float rotation = 0.0f,
                int zIndex = 0,
                bool useScreenSpace = false,
                bool clip = false, 
                Rect clipRegion = {0.0f, 0.0f, 0.0f, 0.0f},
                float sortY = 0.0f
            );
        
        // Queues a string to be rendered using a font atlas.
        void drawText(
                    const FontAtlas* font, 
                    const std::string& text, 
                    Vect2 position, 
                    float fontSize, 
                    float spacing, 
                    Color4 tint = {255, 255, 255, 255}, 
                    int zIndex = 0,
                    bool useScreenSpace = false,
                    bool clip = false, 
                    Rect clipRegion = {0.0f, 0.0f, 0.0f, 0.0f}
                );
        
    protected:
        Renderer(){}
        void init();
        void terminate(){}

        // Finalizes all batches, sorts them if needed and executes GPU draw calls.
        void flush();

        // Clears tracking data and resets batch counters for a new frame.
        void clear();

        void setCamera(Cam* camera) { activeCam = camera; }
        void setShader(const RenderShader* shader) { activeShader = shader; }

        friend class Engine;
        
    private:
        static constexpr int MAX_BATCHES = 128;     // Capacity for active batches. Flushes when exceeded.
        static constexpr int MAX_RQUAD = 50000;     // Capacity for queuing sprites. Flushes when exceeded.
        
        int rQuadCount = 0;
        int batchCount = 0;
        
        // Flag to sort batches based from their Z-Index
        bool sortBatches = true;    

        RenderQuad rQuadQueue[MAX_RQUAD];               // Storage for all draw requests
        std::array<RenderBatch, MAX_BATCHES> batches;   // Pre-allocated render batches.

        Cam* activeCam = nullptr;
        const RenderShader* activeShader = nullptr;
    };
}


