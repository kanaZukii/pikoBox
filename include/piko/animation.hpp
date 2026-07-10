#pragma once
#include <string>
#include <vector>

#include "math.hpp"

namespace piko{
    struct Sprite;

    struct SpriteKey {
        const Sprite* sprite = nullptr;
        float time = 0.0f;
    };

    struct TransformKey{
        Rect transform = {0.0f, 0.0f, 0.0f, 0.0f};
        float time = 0.0f;
    };

    struct ColorKey{
        Color4 color = {0, 0, 0, 0};
        float time = 0.0f;
    };


    class AnimationClip{
    public:
        AnimationClip(
            const std::string& name, 
            const std::vector<SpriteKey>& sprKeys, 
            const std::vector<TransformKey>& tranKeys, 
            const std::vector<ColorKey>& colKeys
        );

        const std::string& getName() const {return name;}
        float getDuration() const { return duration; }

        int findSpriteIndexAt(float time) const;
        int findTransIndexAt(float time) const;
        int findColorIndexAt(float time) const;

        const SpriteKey* getSpriteKey(uint16_t index) const;
        const TransformKey* getTransKey(uint16_t index) const;
        const ColorKey* getColorKey(uint16_t index) const;

        int spriteTrackSize() const { return sprKeys.size(); }
        int transformTrackSize() const { return tranKeys.size(); }
        int colorTrackSize() const { return colKeys.size(); }

        std::string serialize();

    private:
        std::string name;
        float duration = 0.0f;
        std::vector<SpriteKey> sprKeys;
        std::vector<TransformKey> tranKeys; 
        std::vector<ColorKey> colKeys;
    };
};