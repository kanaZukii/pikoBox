#include "piko/components/drawableComp.hpp"
#include "piko/scene.hpp"
#include "piko/assets.hpp"
#include "piko/renderer.hpp"
#include "piko/sprite.hpp"
#include "piko/font.hpp"

#include "json.hpp"

using json = nlohmann::json;
using namespace piko;


void Drawable::setScissorClip(bool enabled, Rect region){
    useScissor = enabled;
    if(useScissor){
        if(region == Rect{0.0f, 0.0f, 0.0f, 0.0f}){
            scissorBox = getGlobalTransform();
        }else{
            scissorBox = region;
        }
    } else {
        scissorBox = {0.0f,0.0f,0.0f,0.0f};
    }
}

std::string Drawable::serialize(){
    json data = json::parse(Component::serialize());
    data["color"] = {{"r", color.r},{"g", color.g},{"b", color.b},{"a", color.a}};
    data["zIndex"] = zIndex;
    data["visible"] = visible;
    data["useScreenSpace"] = useScreenSpace;

    if(useScissor){
        data["useScissor"] = useScissor;
        data["scissorBox"] = {
            {"x", scissorBox.x},
            {"y", scissorBox.y},
            {"w", scissorBox.w},
            {"h", scissorBox.h}
        };
    }

    return data.dump();
}

void Drawable::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    zIndex  = data.value("zIndex", 0);
    visible = data.value("visible", true);
    useScreenSpace = data.value("useScreenSpace", false);
    useScissor = data.value("useScissor", false);

    if (data.contains("scissorBox") && data["scissorBox"].is_object()) {
        auto& sJson = data["scissorBox"];
        scissorBox.x = sJson.value("x", 0.0f);
        scissorBox.y = sJson.value("y", 0.0f);
        scissorBox.w = sJson.value("w", 0.0f);
        scissorBox.h = sJson.value("h", 0.0f);
    }

    if (data.contains("color") && data["color"].is_object()) {
        auto& cJson = data["color"];
        color.r = cJson.value("r", uint8_t(255));
        color.g = cJson.value("g", uint8_t(255));
        color.b = cJson.value("b", uint8_t(255));
        color.a = cJson.value("a", uint8_t(255));
    }
}

SpriteRenderer::SpriteRenderer(const Sprite* spr) : Drawable() {
    className = "SpriteRenderer";
    sprite = spr;
    if(sprite){
        size = {(float)sprite->source.w, (float)sprite->source.h};
    }
} 

void SpriteRenderer::setSprite(const Sprite* spr){
    if(!sprite && spr){
        size = {(float)spr->source.w, (float)spr->source.h};
    }

    sprite = spr;
}

void SpriteRenderer::update(float dt){
    Component::update(dt);
}

void SpriteRenderer::draw(Renderer& renderer){
    if((int)color.a <= 0 || !isVisible()) return;
    
    const TextureIMG* tex = nullptr; Rect src = {0.0f};
    if(sprite){
        tex = sprite->tex;
        src = sprite->source;
    }

    if (flipX) {
        src.x = src.x + src.w;
        src.w = -src.w;
    }
    if (flipY) {
        src.y = src.y + src.h;
        src.h = -src.h;
    }

    const Rect& bounds = getGlobalTransform();

    renderer.draw(
        tex, 
        src, 
        bounds,
        origin,
        color,
        rotation,
        zIndex,
        useScreenSpace,
        useScissor,
        scissorBox,
        0.0f
    );
}

std::string SpriteRenderer::serialize(){
    json data = json::parse(Drawable::serialize());
    data["origin"] = {{"x", origin.x}, {"y", origin.y}};
    data["rotation"] = rotation;
    data["flipX"] = flipX;
    data["flipY"] = flipY;
    
    if(sprite){
        json spriteJson = {
            {"texName", sprite->texName}
        };
        
        if(sprite->index > -1){
            spriteJson["index"] = sprite->index;
        }
        
        data["sprite"] = spriteJson;
    }

    return data.dump();
}

void SpriteRenderer::deserialize(const std::string& rawJson){
    Drawable::deserialize(rawJson);
    json data = json::parse(rawJson);
    
    rotation = data.value("rotation", 0.0f);
    flipX = data.value("flipX", false);
    flipY = data.value("flipY", false);
    
    if (data.contains("origin") && data["origin"].is_object()) {
        origin.x = data["origin"].value("x", 0.0f);
        origin.y = data["origin"].value("y", 0.0f);
    }

    sprite = nullptr;
    if (data.contains("sprite") && data["sprite"].is_object()) {
        auto& spriteJson = data["sprite"];
        std::string tex = spriteJson.value("texName", "");
        
        int idx = spriteJson.value("index", -1);
        if(idx > -1){
            sprite = owner->scene->getAssets()->getSpriteFromSheet(tex, idx);
        } else {
            sprite = owner->scene->getAssets()->getTexSprite(tex);
        }
    }
}

TextRenderer::TextRenderer(const std::string& text) : Drawable() {
    className = "TextRenderer";
    this->text = text;
}

void TextRenderer::draw(Renderer& renderer){
    if((int)color.a <= 0 || !isVisible() || !font) return;

    const Rect& bounds = getGlobalTransform();
    
    renderer.drawText(
        font,
        text,
        {bounds.x, bounds.y},
        fontSize,
        fontSpacing,
        color,
        zIndex,
        useScreenSpace,
        useScissor,
        scissorBox
    );
}

std::string TextRenderer::serialize() {
    json data = json::parse(Drawable::serialize());
    data["origin"] = {{"x",origin.x},{"y",origin.y}};
    data["fontSize"] = fontSize;
    data["fontSpacing"] = fontSpacing;
    if(font){
        data["fontPath"] = font->getFilePath();
    }
    if(!text.empty()){
        data["text"] = text;
    }
    return data.dump();
}

void TextRenderer::deserialize(const std::string& rawJson) {
    Drawable::deserialize(rawJson);
    json data = json::parse(rawJson);

    fontSize = data.value("fontSize", 32.0f);
    fontSpacing = data.value("fontSpacing", 0.0f);
    
    if (data.contains("origin") && data["origin"].is_object()) {
        origin.x = data["origin"].value("x", 0.0f);
        origin.y = data["origin"].value("y", 0.0f);
    }

    font = nullptr;
    if(data.contains("fontPath")){
        std::string fontPath = data.value("fontPath", "");
        font = owner->scene->getAssets()->getFontAtlasByPath(fontPath);
    }

    text = data.value("text", "");
}

void TextBoxRenderer::recalculateWordWrap() {
    cachedLines.clear();
    isDirty = false;

    if (text.empty() || !font) return;

    const Rect& bounds = getGlobalTransform();
    float maxLineWidth = bounds.w - (padding.x * 2.0f);
    if (maxLineWidth <= 0) return;

    // Tokenize text into words while preserving explicit user newlines
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string explicitLine;
    
    while (std::getline(ss, explicitLine, '\n')) {
        std::vector<std::string> tokens;
        std::stringstream lineStream(explicitLine);
        std::string token;
        
        while (lineStream >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            cachedLines.push_back(""); // Handle empty lines / double spaces
            continue;
        }

        std::string currentLine = tokens[0];
        for (size_t i = 1; i < tokens.size(); ++i) {
            std::string testLine = currentLine + " " + tokens[i];
            if (measureTextWidth(testLine) <= maxLineWidth) {
                currentLine = testLine;
            } else {
                cachedLines.push_back(currentLine);
                currentLine = tokens[i];
            }
        }
        cachedLines.push_back(currentLine);
    }
}

float TextBoxRenderer::measureTextWidth(const std::string& line) {
    if (line.empty() || !font) return 0.0f;
    
    float scaleFactor = fontSize / (float)font->getBaseSize();
    float totalWidth = 0.0f;

    for (size_t i = 0; i < line.length(); i++) {
        int character = (int)line[i];
        const FontGlyph* glyph = font->getGlyph(character);
        
        if (glyph) {
            if (i == line.length() - 1) {
                // Last character: use its actual visual width bounds
                totalWidth += glyph->source.w * scaleFactor;
            } else {
                // Intermediate characters: use advance metrics
                if (glyph->advanceX == 0) {
                    totalWidth += (glyph->source.w + fontSpacing) * scaleFactor;
                } else {
                    totalWidth += ((float)glyph->advanceX + fontSpacing) * scaleFactor;
                }
            }
        }
    }
    return totalWidth;
}

void TextBoxRenderer::setText(const std::string& newText) {
    if (text == newText) return; 
    text = newText;
    isDirty = true;
    visibleCharacterCount = 0;
    currentTypeTimer = 0.0f;
}

void TextBoxRenderer::update(float dt) {
    if (isDirty) {
        getGlobalTransform();
        recalculateWordWrap();
    }

    if (isTypewriterActive && visibleCharacterCount < text.length()) {
        currentTypeTimer += dt;
        float charsPerFrame = 1.0f / typeSpeed;
        if (currentTypeTimer >= charsPerFrame) {
            size_t charsToAdvance = (size_t)(currentTypeTimer / charsPerFrame);
            visibleCharacterCount = std::min(visibleCharacterCount + charsToAdvance, text.length());
            currentTypeTimer = std::fmod(currentTypeTimer, charsPerFrame);
        }
    } else if (!isTypewriterActive) {
        visibleCharacterCount = text.length();
    }
}

void TextBoxRenderer::draw(Renderer& renderer) {
    if ((int)color.a <= 0 || !isVisible() || cachedLines.empty()) return;

    const Rect& bounds = getGlobalTransform();
    float scaleFactor = fontSize / (float)font->getBaseSize();
    float fontHeight = (float)font->getBaseSize() * scaleFactor;
    float verticalAdvance = fontHeight * lineSpacing;

    Vect2 drawPos = { bounds.x + padding.x, bounds.y + padding.y };
    size_t charactersRendered = 0;

    for (const auto& line : cachedLines) {
        if (charactersRendered >= visibleCharacterCount) break;

        // Determine if we need to slice the line for the typewriter effect
        std::string lineToDraw = line;
        if (charactersRendered + line.length() > visibleCharacterCount) {
            size_t allowedLength = visibleCharacterCount - charactersRendered;
            lineToDraw = line.substr(0, allowedLength);
        }

        // Apply Alignment Offsets
        float lineXOffset = 0.0f;
        if (alignment == TEXTALIGN::CENTER) {
            float lineWidth = measureTextWidth(line); // Measure against full layout line
            lineXOffset = (bounds.w - (padding.x * 2.0f) - lineWidth) * 0.5f;
        } else if (alignment == TEXTALIGN::RIGHT) {
            float lineWidth = measureTextWidth(line);
            lineXOffset = bounds.w - (padding.x * 2.0f) - lineWidth;
        }

        Vect2 finalLinePos = { drawPos.x + lineXOffset, drawPos.y };

        // Pass line straight into your batch-driven drawText!
        renderer.drawText(
            font,
            lineToDraw,
            finalLinePos,
            fontSize,
            fontSpacing,
            color,
            zIndex,
            useScreenSpace,
            useScissor,
            scissorBox
        );

        // Advance baseline and character trackers
        drawPos.y += verticalAdvance;
        charactersRendered += line.length() + 1; // +1 accounts for the parsed space/newline
    }
}

std::string TextBoxRenderer::serialize(){
    json data = json::parse(TextRenderer::serialize());
    data["alignment"] = static_cast<int>(alignment);
    data["padding"] = {{"x", padding.x},{"y", padding.y}}; 
    data["lineSpacing"] = lineSpacing;

    data["isTypewriterActive"] = isTypewriterActive;
    data["typeSpeed"] = typeSpeed;

    return data.dump();
}

void TextBoxRenderer::deserialize(const std::string& rawJson){
    TextRenderer::deserialize(rawJson);
    json data = json::parse(rawJson);

    alignment = static_cast<TEXTALIGN>(data.value("alignment", 0));
    lineSpacing = data.value("lineSpacing", 1.2f);
    isTypewriterActive = data.value("isTypewriterActive", false);
    typeSpeed = data.value("typeSpeed", 30.0f);

    if (data.contains("padding") && data["padding"].is_object()) {
        auto& pJson = data["padding"];
        padding.x = pJson.value("x", 0.0f);
        padding.y = pJson.value("y", 0.0f);
    }

    currentTypeTimer = 0.0f;
    visibleCharacterCount = 0;
    isDirty = true;
}


void UIPanel::addChildEntity(uint32_t childId) {
    auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (it == childrenIds.end()) {
        childrenIds.push_back(childId);
        isLayoutDirty = true;
    }
}

void UIPanel::removeChildEntity(uint32_t childId) {
    auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (it != childrenIds.end()) {
        childrenIds.erase(it);
        isLayoutDirty = true;
    }
}

void UIPanel::recalculateLayout() {
    if (!isLayoutDirty || layoutMode == LAYOUT::FREE || !owner) return;

    // Fetch master panel boundary footprint
    const Rect& panelBounds = getGlobalTransform();

    // Setup cursors starting inside the padding margins
    float currentX = padding.x;
    float currentY = padding.y;

    for (uint32_t childId : childrenIds) {
        // Query the flat scene layout database
        Entity* childEnt = owner->scene->getEntity(childId);
        if (!childEnt) continue;

        if (layoutMode == LAYOUT::VERTICAL) {
            // Anchor child X to panel left margin, stack Y downward
            childEnt->transform.x = panelBounds.x + padding.x;
            childEnt->transform.y = panelBounds.y + currentY;

            // Advance the cursor downward by child height + the layout spacing gap
            currentY += childEnt->transform.h + childSpacing;

        } else if (layoutMode == LAYOUT::HORIZONTAL) {
            // Stack buttons left-to-right across a row
            childEnt->transform.x = panelBounds.x + currentX;
            childEnt->transform.y = panelBounds.y + padding.y;

            // Advance cursor rightward
            currentX += childEnt->transform.w + childSpacing;
        }

        // Flag all components on the child entity as dirty!
        childEnt->setDirtyTransform();
    }

    isLayoutDirty = false;
}

void UIPanel::update(float dt) {
    if (isLayoutDirty) {
        recalculateLayout();
    }
}

void UIPanel::draw(Renderer& renderer){
    if((int)color.a <= 0 || !isVisible()) return;

    const Rect& bounds = getGlobalTransform();

    renderer.draw(
        nullptr, 
        {0.0f, 0.0f, 0.0f, 0.0f}, 
        bounds,
        origin,
        color,
        rotation,
        zIndex,
        useScreenSpace,
        useScissor,
        scissorBox,
        0.0f
    );
}

std::string UIPanel::serialize() {
    json data = json::parse(Drawable::serialize());
    
    json serializedChild = json::array(); 
    for (uint32_t childId : childrenIds) {
        Entity* childEnt = owner->scene->getEntity(childId);
        if (!childEnt) continue;
        serializedChild.push_back(childEnt->alias); // Store alias strings to ensure file stability
    }

    data["layoutMode"] = static_cast<int>(layoutMode);
    data["childSpacing"] = childSpacing;
    data["padding"] = {{"x", padding.x}, {"y", padding.y}};
    data["origin"] = {{"x", origin.x}, {"y", origin.y}};
    data["rotation"] = rotation;
    data["childrenIds"] = serializedChild;

    return data.dump();
}

void UIPanel::deserialize(const std::string& rawJson) {
    Drawable::deserialize(rawJson);
    json data = json::parse(rawJson);

    layoutMode = static_cast<LAYOUT>(data.value("layoutMode", 0));
    childSpacing = data.value("childSpacing", 0.0f);
    rotation = data.value("rotation", 0.0f);

    if (data.contains("padding") && data["padding"].is_object()) {
        auto& pJson = data["padding"];
        padding.x = pJson.value("x", 0.0f);
        padding.y = pJson.value("y", 0.0f);
    }

    if (data.contains("origin") && data["origin"].is_object()) {
        auto& oJson = data["origin"];
        origin.x = oJson.value("x", 0.0f);
        origin.y = oJson.value("y", 0.0f);
    }

    // Clear runtime ID cache before parsing layout targets
    childrenIds.clear();

    if (data.contains("childrenIds") && data["childrenIds"].is_array()) {
        // Capture aliases by value into our vector to safely pass them into the post-load phase
        std::vector<std::string> childAliases = data["childrenIds"].get<std::vector<std::string>>();

        owner->scene->addPostLoadJob([this, childAliases]() {
            for (const auto& alias : childAliases) {
                Entity* childEnt = this->owner->scene->getEntity(alias);
                if (childEnt) {
                    this->childrenIds.push_back(childEnt->id);
                }
            }
            this->isLayoutDirty = true; 
        });
    }

    isLayoutDirty = true;
}
