// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {

    class Renderer;
    struct Sprite;
    class FontAtlas;

    /*
        SpriteRenderer Component, a Basic Component for sprite rendering.
        Draws a texture within a certain region area.
     */
     class SpriteRenderer : public Drawable{
        public:
            void setSprite(const Sprite* spr);

            void setRotation(float degrees) {rotation = degrees;}

            void setFlipX(bool flip) {flipX = flip;}
            void setFlipY(bool flip) {flipY = flip;}
            void toggleFlipX() {flipX = !flipX;}
            void toggleFlipY() {flipY = !flipY;}

            bool isXFlipped() {return flipX;}
            bool isYFlipped() {return flipY;}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

        protected:
            SpriteRenderer() : Drawable() {className = "SpriteRenderer";}
            SpriteRenderer(const Sprite* spr);
            
            void update(float dt) override;
            void draw(Renderer& renderer) override;

            friend class Scene;
            friend class SceneManager;

        private:
            const Sprite* sprite = nullptr;

            Vect2 origin = {0.0f, 0.0f};
            float rotation = 0.0f;
            bool flipX = false;
            bool flipY = false;
    };

    /*
        TextRenderer Component, for simple text rendering.
        Provides basic text, font, and scaling configuration.
        Requires a loaded FontAtlas to render.
     */
    class TextRenderer : public Drawable{
        public:
           
            void setText(const std::string& text){this->text = text;}
            void setFont(const FontAtlas* font){this->font = font;}

            void setFontSize(float fsize){fontSize = fsize;}
            void setFontSpacing(float fspacing){fontSpacing = fspacing;}

            inline std::string getText(){return text;}
            inline const FontAtlas* getFont(){return font;}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;
            
        protected:
            TextRenderer() : Drawable() { className = "TextRenderer"; }
            TextRenderer(const std::string& text); 
            
            void update(float dt) override {}
            void draw(Renderer& renderer) override;

            std::string text = "";
            const FontAtlas* font = nullptr;

            float fontSize = 32.0f;
            float fontSpacing = 0.0f;

            friend class Scene;
            friend class SceneManager;

        private:
            Vect2 origin = {0.0f, 0.0f};
    };

    /*
        TextBoxRenderer Component, for advanced text rendering with word-wrapping and typewriter animation.
        Uses a 'dirty' flag pattern to cache text layout and minimize per-frame calculations.
        To define the box region where it will be rendered, use setSize(Vect2 size)
     */
    class TextBoxRenderer : public TextRenderer {
        public:
            // For text alignment within the box region 
            enum class TEXTALIGN { LEFT, CENTER, RIGHT };

            void setText(const std::string& newText); 
            void setAlignment(TEXTALIGN align) { alignment = align; isDirty = true; }
            void setPadding(Vect2 pad) { padding = pad; isDirty = true; }
            void setTypewriterActive(bool enabled) {isTypewriterActive = enabled;}
            void setTypeSpeed(float speed) {typeSpeed = speed;}

            bool isAllTextShown() {return visibleCharacterCount >= text.length();}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

        protected:

            TextBoxRenderer() : TextRenderer() { className = "TextBoxRenderer"; }
            
            void draw(Renderer& renderer) override;
            void update(float dt) override;

            friend class Scene;
            friend class SceneManager;

        private:
            TEXTALIGN alignment = TEXTALIGN::LEFT;
            Vect2 padding = {0.0f, 0.0f};
            float lineSpacing = 1.2f;

            // Typewriter Fields
            bool isTypewriterActive = false;
            float typeSpeed = 30.0f; 
            float currentTypeTimer = 0.0f;
            size_t visibleCharacterCount = 0;

            // Cache to prevent redundant text measurement
            bool isDirty = true;
            std::vector<std::string> cachedLines;
            void recalculateWordWrap();
            float measureTextWidth(const std::string& line);
    };

    /*
        UIPanel Component, a container for managing spatial layouts of child entities.
        Automatically repositions children based on VERTICAL/HORIZONTAL alignment settings.
     */
    class UIPanel : public Drawable{
        public:
            enum class LAYOUT { FREE, VERTICAL, HORIZONTAL };

            void addChildEntity(uint32_t childId);
            void removeChildEntity(uint32_t childId);
            void popChildEntity();

            void setLayoutMode(LAYOUT mode) { layoutMode = mode; isLayoutDirty = true; }
            void setChildSpacing(float spacing) { childSpacing = spacing; isLayoutDirty = true; }
            void setPadding(Vect2 pad) { padding = pad; isLayoutDirty = true; }
            
            // This enables auto vertical and horizontal centering.
            void setCenterContent(bool centered){centerContent = centered;}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

        protected:
            UIPanel() : Drawable() {
                className = "UIPanel";
                color = {30, 30, 30, 150};
            }  
            
            void update(float dt) override;
            void draw(Renderer& renderer) override;

            friend class Scene;
            friend class SceneManager;

        private:
            LAYOUT layoutMode = LAYOUT::FREE;
            bool centerContent = false;
            float childSpacing = 0.0f;
            Vect2 padding = {0.0f, 0.0f};
            Vect2 origin = {0.0f, 0.0f};
            float rotation = 0.0f;

            bool isLayoutDirty = true;

            // Does not hold pointers to Entities, only used when setting new positions.
            std::vector<uint32_t> childrenIds;

            void recalculateLayout();
    };
}