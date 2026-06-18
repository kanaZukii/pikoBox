#include "piko/renderer.hpp"
#include "piko/shader.hpp"
#include "piko/cam.hpp"
#include "piko/sprite.hpp"
#include "piko/logger.hpp"

#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"

#include <algorithm>

using namespace piko;

RenderBatch::RenderBatch(int zIndex, bool useScreenSpace, bool clip, Rect clipRegion) 
    : zIndex(zIndex), useScreenSpace(useScreenSpace), doClipping(clip), clipRegion(clipRegion) {
    textures.fill(nullptr);
}

RenderBatch::~RenderBatch() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
}

RenderBatch::RenderBatch(RenderBatch&& other) noexcept 
    : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), 
      zIndex(other.zIndex), 
      useScreenSpace(other.useScreenSpace),
      doClipping(other.doClipping), 
      clipRegion(other.clipRegion),
      vertices(std::move(other.vertices)), 
      textures(std::move(other.textures)),
      cmds(std::move(other.cmds)),
      cmdTexSlots(std::move(other.cmdTexSlots)) 
{
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
}

// 2. Move Assignment Operator
RenderBatch& RenderBatch::operator=(RenderBatch&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources in this object first
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);

        // Steal the new ones
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        zIndex = other.zIndex;
        useScreenSpace = other.useScreenSpace;
        doClipping = other.doClipping;
        clipRegion = other.clipRegion;
        vertices = std::move(other.vertices);
        textures = std::move(other.textures);

        cmds = std::move(other.cmds);
        cmdTexSlots = std::move(other.cmdTexSlots);

        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }
    return *this;
}


void RenderBatch::init(){
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    
    std::vector<uint32_t> indices = generateIndices();
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    int stride = sizeof(Vertex);
    // 0: Position (x, y, z)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, x));
    
    // 1: Color (r, g, b, a)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, r));

    // 2: TexCoords (u, v)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, u));

    // 3: TexID
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, texID));

    glBindVertexArray(0);
}

std::vector<uint32_t> RenderBatch::generateIndices()
{
    std::vector<uint32_t> indices;
    indices.reserve(MAX_QUADS * 6);

    for (uint32_t i = 0; i < MAX_QUADS; i++)
    {
        uint32_t offset = i * 4;

        indices.push_back(offset + 3);
        indices.push_back(offset + 2);
        indices.push_back(offset + 0);

        indices.push_back(offset + 0);
        indices.push_back(offset + 2);
        indices.push_back(offset + 1);
    }

    return indices;
}

void RenderBatch::loadVertexProperties(const DrawCommand& cmd, Vertex* quadptr, int texSlot) {
    // Pre-calculate trig
    const float rad = cmd.rotation * DEG2RAD;
    const float cosA = cosf(rad);
    const float sinA = sinf(rad);

    const float w = cmd.dest.w;
    const float h = cmd.dest.h;
    
    // Local coordinates relative to origin
    const float x0 = 0.0f - cmd.origin.x;
    const float y0 = 0.0f - cmd.origin.y;
    const float x1 = w - cmd.origin.x;
    const float y1 = h - cmd.origin.y;

    const float localX[4] = { x0, x1, x1, x0 };
    const float localY[4] = { y0, y0, y1, y1 };

    // Texture Coordinates
    float uMin = 0.0f, vMin = 0.0f, uMax = 1.0f, vMax = 1.0f;
    if (cmd.texture != nullptr) {
        const float invW = cmd.texture->getInvWidth();
        const float invH = cmd.texture->getInvHeight();
        uMin = cmd.source.x * invW;
        vMin = cmd.source.y * invH;
        uMax = (cmd.source.x + cmd.source.w) * invW;
        vMax = (cmd.source.y + cmd.source.h) * invH;
    }
    
    const float uvs[4][2] = { {uMin, vMin}, {uMax, vMin}, {uMax, vMax}, {uMin, vMax} };

    // Color normalization
    const float r = cmd.tint.r / 255.0f;
    const float g = cmd.tint.g / 255.0f;
    const float b = cmd.tint.b / 255.0f;
    const float a = cmd.tint.a / 255.0f;
    const float tid = (float)texSlot;
    const float z = (float)cmd.zIndex;

    for(int i = 0; i < 4; i++) {
        quadptr[i].x = (localX[i] * cosA - localY[i] * sinA) + cmd.dest.x;
        quadptr[i].y = (localX[i] * sinA + localY[i] * cosA) + cmd.dest.y;
        quadptr[i].z = z;
        
        quadptr[i].r = r;
        quadptr[i].g = g;
        quadptr[i].b = b;
        quadptr[i].a = a;

        quadptr[i].u = uvs[i][0];
        quadptr[i].v = uvs[i][1];

        quadptr[i].texID = tid;
    }
}

bool RenderBatch::add(const DrawCommand& cmd){
    if(quadCount >= MAX_QUADS || cmd.zIndex != zIndex) return false;

    int texSlot = getTextureSlot(cmd.texture);
    if(texSlot == -1){
        if(texCount >= MAX_TEXTURES){ return false;}
        texSlot = texCount++;
        textures[texSlot] = cmd.texture;
    }
    
    cmds[quadCount] = cmd;
    cmdTexSlots[quadCount] = texSlot;

    quadCount++;
    return true;
}

int RenderBatch::getTextureSlot(const TextureIMG* tex){
    if(!tex){return 0;}

    for(int i = 0; i < texCount; i++){
        if(textures[i] == tex) return i;
    }
    return -1;
}


void RenderBatch::flush(RenderShader* shader, Cam* camera) {
    if (quadCount == 0) return;

    Vertex* vPtr = vertices.data();

    for (int i = 0; i < quadCount; i++) {
        // Pass the address of the specific 4-vertex block for this quad
        loadVertexProperties(cmds[i], vPtr, cmdTexSlots[i]);
        vPtr += 4;
    }

    // Upload the vertex data to the GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, quadCount * 4 * sizeof(Vertex), vertices.data());

    if(doClipping){
        BeginScissorMode(
            static_cast<int>(clipRegion.x),
            static_cast<int>(clipRegion.y),
            static_cast<int>(clipRegion.w),
            static_cast<int>(clipRegion.h)
        );
    }

    shader->begin();
    
    // Upload Camera Matrices
    SetShaderValueMatrix(shader->getProgram(), shader->getLocationID("uProjection"), camera->getProj());

    if(useScreenSpace){
        SetShaderValueMatrix(shader->getProgram(), shader->getLocationID("uView"), Mat4::Identity());
    } else {
        SetShaderValueMatrix(shader->getProgram(), shader->getLocationID("uView"), camera->getView());
    }
    
    // Bind Textures to Slots 0-7
    for (int i = 0; i < texCount; i++) {
        rlActiveTextureSlot(i);
        if (i == 0 || textures[i] == nullptr) {
            rlEnableTexture(rlGetTextureIdDefault()); // Raylib's 1x1 white pixel
        } else {
            rlEnableTexture(textures[i]->getData().id);
        }
    }

    // Inform the shader which sampler goes to which texture unit
    int samplerUnits[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    SetShaderValueV(shader->getProgram(), shader->getLocationID("uTextures"), samplerUnits, SHADER_UNIFORM_INT, 8);

    // Actual Draw Call
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, quadCount * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    shader->end();

    if (doClipping) {
        EndScissorMode();
    }

    // Reset for next frame
    quadCount = 0;
    texCount = 1;
}

void Renderer::init(RenderShader* defaultShader, Cam* defaultCam){
    batches.reserve(64);

    activeCam = defaultCam;
    activeShader = defaultShader;

    PBOX_INFO("RENDERER: Renderer Initialized.");
}

void Renderer::draw(const TextureIMG* tex, Rect texsrc, Rect dest, Vect2 origin,
                    Color4 tint, float rotation, int zIndex, bool useScreenSpace, bool clip, Rect clipRegion, float sortY)
{
    bool added = false;

    if(cmdCount >= MAX_COMMANDS){
        flush();
        cmdCount = 0;
    }

    DrawCommand& cmd = commands[cmdCount++];
    cmd.texture = tex;
    cmd.source = texsrc;
    cmd.dest = dest;
    cmd.origin = origin;
    cmd.tint = tint;
    cmd.zIndex = zIndex;
    cmd.sortY = sortY;
    cmd.useScreenSpace = useScreenSpace;
    cmd.rotation = rotation;
    cmd.clipRegion = clipRegion;
    cmd.clip = clip;
    
    for (auto& batch : batches) {
        if (batch.hasRoom() && batch.getZ() == zIndex && batch.hasClipping() == clip 
            && batch.isScreenSpaceMode() == useScreenSpace) {
            // Check if texture fits or already exists in this batch
            if(!clip || batch.getClipRegion() == clipRegion){
                if (batch.add(cmd)) {
                    added = true;
                    break;
                }
            }
        }
    }

    if (!added) {
        // No suitable batch found? Create a new one!
        RenderBatch newBatch(zIndex, useScreenSpace, clip, clipRegion);
        newBatch.init(); // Setup VAO/VBO/EBO
        newBatch.add(cmd);
        batches.push_back(std::move(newBatch));
        
        // Sort batches by Z so we always draw back-to-front
        std::sort(batches.begin(), batches.end(), [](const RenderBatch& a, const RenderBatch& b) {
            return a.getZ() < b.getZ();
        });
    }
}

void Renderer::drawText(const FontAtlas* font, const std::string& text, Vect2 position, 
                    float fontSize, float spacing, Color4 tint, int zIndex,  bool useScreenSpace, bool clip, Rect clipRegion) 
{
    
    if (!font || text.empty()) return;
    if (font->getTexture() == nullptr) return;

    int baseSize = font->getBaseSize();
    const TextureIMG* fontTex = font->getTexture();

    // Calculate the dynamic scale modifier based on the requested display size
    float scaleFactor = fontSize / (float)baseSize;
    float textX = position.x;
    float textY = position.y;

    for (size_t i = 0; i < text.length(); i++) {
        int character = (int)text[i];

        // Handle simple line wraps
        if (character == '\n') {
            textX = position.x;
            textY += (baseSize + (baseSize / 2.0f)) * scaleFactor;
            continue;
        }

        // Find the matching glyph metadata inside our atlas storage
        const FontGlyph* glyph = font->getGlyph(character);

        if (glyph) {
            // Compute the drawing footprint bounds on your viewport canvas canvas
            Rect destRect = {
                textX + (float)glyph->offsetX * scaleFactor,
                textY + (float)glyph->offsetY * scaleFactor,
                glyph->source.w * scaleFactor,
                glyph->source.h * scaleFactor
            };

            // PUSH STRAIGHT INTO YOUR BATCH ARCHITECTURE!
            if (character != ' ' && character != '\t') {
                this->draw(fontTex, glyph->source, destRect, {0.0f, 0.0f}, tint, 0.0f, zIndex, useScreenSpace, clip, clipRegion, 0.0f);
            }

            // Move the layout cursor forward using the font's native character metrics
            if (glyph->advanceX == 0) {
                textX += (glyph->source.w + spacing) * scaleFactor;
            } else {
                textX += ((float)glyph->advanceX + spacing) * scaleFactor;
            }
        }
    }
}

void Renderer::flush(){
    if (!activeShader || !activeCam) return;

    rlDrawRenderBatchActive();

    for(int i = 0; i < batches.size(); ++i){
        RenderBatch& b = batches[i];
        b.flush(activeShader, activeCam);
    }

    cmdCount = 0;
}