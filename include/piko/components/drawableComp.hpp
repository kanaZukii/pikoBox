#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {

    class Renderer;
    struct Sprite;
    class FontAtlas;

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

            void update(float dt) override;
            void draw(Renderer& renderer) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

        protected:
            SpriteRenderer() : Drawable() {className = "SpriteRenderer";}
            SpriteRenderer(const Sprite* spr);

            friend class Scene;
            friend class SceneManager;

        private:
            const Sprite* sprite = nullptr;

            Vect2 origin = {0.0f, 0.0f};
            float rotation = 0.0f;
            bool flipX = false;
            bool flipY = false;
    };

    class TextRenderer : public Drawable{
        public:
            void update(float dt) override {}
            void draw(Renderer& renderer) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;
            
            void setText(const std::string& text){this->text = text;}
            void setFont(const FontAtlas* font){this->font = font;}

            void setFontSize(float fsize){fontSize = fsize;}
            void setFontSpacing(float fspacing){fontSpacing = fspacing;}

            inline std::string getText(){return text;}
            inline const FontAtlas* getFont(){return font;}

        protected:
            TextRenderer() : Drawable() { className = "TextRenderer"; }
            TextRenderer(const std::string& text);

            std::string text = "";
            const FontAtlas* font = nullptr;

            float fontSize = 32.0f;
            float fontSpacing = 0.0f;

            friend class Scene;
            friend class SceneManager;

        private:
            Vect2 origin = {0.0f, 0.0f};
    };

    class TextBoxRenderer : public TextRenderer {
        public:
            enum class TEXTALIGN { LEFT, CENTER, RIGHT };

            void draw(Renderer& renderer) override;
            void update(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setText(const std::string& newText); 
            void setAlignment(TEXTALIGN align) { alignment = align; isDirty = true; }
            void setPadding(Vect2 pad) { padding = pad; isDirty = true; }
            void setTypewriterActive(bool enabled) {isTypewriterActive = enabled;}
            void setTypeSpeed(float speed) {typeSpeed = speed;}

            bool isAllTextShown() {return visibleCharacterCount >= text.length();}

        protected:
            TextBoxRenderer() : TextRenderer() { className = "TextBoxRenderer"; }

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

            // Performance Cache
            bool isDirty = true;
            std::vector<std::string> cachedLines;
            void recalculateWordWrap();
            float measureTextWidth(const std::string& line);
    };

    class UIPanel : public Drawable{
        public:
            enum class LAYOUT { FREE, VERTICAL, HORIZONTAL };

            void update(float dt) override;
            void draw(Renderer& renderer) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void addChildEntity(uint32_t childId);
            void removeChildEntity(uint32_t childId);
            void popChildEntity();

            void setLayoutMode(LAYOUT mode) { layoutMode = mode; isLayoutDirty = true; }
            void setChildSpacing(float spacing) { childSpacing = spacing; isLayoutDirty = true; }
            void setPadding(Vect2 pad) { padding = pad; isLayoutDirty = true; }
            void setCenterContent(bool centered){centerContent = centered;}

        protected:
            UIPanel() : Drawable() {
                className = "UIPanel";
                color = {30, 30, 30, 150};
            }

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
            std::vector<uint32_t> childrenIds;

            void recalculateLayout();
    };
}