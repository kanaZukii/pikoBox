#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <string>

#include "piko/math.hpp"
#include "piko/font.hpp"

namespace piko {
    class Engine;
    class Renderer;
    class TextureIMG;
    class Cam;
    class RenderShader;
    
    struct RenderQuad {
        const TextureIMG* texture = nullptr;
        Rect source = {0};
        Rect dest = {0};
        Vect2 origin = {0};
        Color4 tint = {255, 255, 255, 255};

        int zIndex = 0;
        float sortY = 0.0f;
        float rotation = 0.0f;
        bool useScreenSpace = false;
        bool clip = false;
        Rect clipRegion = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    struct Vertex {
        float x, y, z;          // Position
        float r, g, b, a;       // Color
        float u, v;             // TexCoords
        float texID;            // Texture ID
    };

    class RenderBatch {
    public:
        ~RenderBatch();

        RenderBatch(const RenderBatch&) = delete;
        RenderBatch& operator=(const RenderBatch&) = delete;

        RenderBatch(RenderBatch&& other) noexcept;
        RenderBatch& operator=(RenderBatch&& other) noexcept;

        void init();
        bool add(const RenderQuad& rQuad);

        void flush(RenderShader* shader, Cam* camera);

        const bool hasRoom() const{ return quadCount < MAX_QUADS;}
        const bool hasRoomForTexture() const{ return texCount < MAX_TEXTURES;}
        
        int getZ() const { return zIndex; }
        bool isScreenSpaceMode() const {return useScreenSpace;}
        const Rect& getClipRegion() const{ return clipRegion;}
        const bool hasClipping() const{ return doClipping;}

    private:
        RenderBatch(int zIndex, bool useScreenSpace, bool clip, Rect clipRegion);
        static constexpr int MAX_TEXTURES = 8;
        static constexpr int MAX_QUADS = 1000;

        static constexpr int VERTEX_SIZE = 10;
        static constexpr int VERTEX_SIZE_BYTES = VERTEX_SIZE * sizeof(float);

        int texCount = 1;
        int quadCount = 0;

        uint32_t VAO = 0;
        uint32_t VBO = 0;
        uint32_t EBO = 0;

        int zIndex = 0;
        bool useScreenSpace = false;
        bool doClipping = false;
        Rect clipRegion = {0.0f, 0.0f, 0.0f, 0.0f};

        std::array<Vertex, MAX_QUADS * 4> vertices;
        std::array<const TextureIMG*, MAX_TEXTURES> textures;
        
        std::array<RenderQuad, MAX_QUADS> rQuads;
        std::array<int, MAX_QUADS> rQuadTexSlots;

        std::array<int, MAX_TEXTURES> texSlots = {0,1,2,3,4,5,6,7};

        int getTextureSlot(const TextureIMG* tex);
        void loadVertexProperties(const RenderQuad& rQuad, Vertex* quadPtr, int texSlot);

        std::vector<uint32_t> generateIndices();
        friend class Renderer;
    };

    class Renderer {
    public:
        ~Renderer() = default;
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        void init(RenderShader* defaultShader, Cam* defaultCam);
        
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
        
        void flush();

        void setCamera(Cam* cam) { activeCam = cam; }
        void setShader(RenderShader* shader) { activeShader = shader; }

    private:
        Renderer(){}
        static constexpr int MAX_RQUAD = 50000;
        int rQuadCount = 0;
        RenderQuad rQuadQueue[MAX_RQUAD];
        std::vector<RenderBatch> batches;
        Cam* activeCam = nullptr;
        RenderShader* activeShader = nullptr;

        friend class Engine;
    };
}


