// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include <string>
#include <vector>

#include "math.hpp"

namespace piko{
    struct Sprite;

    /*
        Key frame structure for Sprite manipulation at a 
        given timestamp, must be stored in an AnimationClip.
    */
    struct SpriteKey {
        const Sprite* sprite = nullptr;
        float time = 0.0f;
    };

    /*
        Key frame structure for transform manipulation at a 
        given timestamp, must be stored in an AnimationClip.
    */
    struct TransformKey{
        Rect transform = {0.0f, 0.0f, 0.0f, 0.0f};
        float time = 0.0f;
    };

    /*
        Key frame structure for Color4 manipulation at a 
        given timestamp, must be stored in an AnimationClip.
    */
    struct ColorKey{
        Color4 color = {0, 0, 0, 0};
        float time = 0.0f;
    };


    /*
        AnimationClip asset, contains an animation sequence containing multiple tracks.
        Tracks are independent and are time-bound, allowing for sprite changes, transform updates,
        and color modulation to occur at different keyframe intervals.
     */
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

        // Find index of a Sprite keyframe active at the given timestamp
        int findSpriteIndexAt(float time) const;
        // Find index of a Transform keyframe active at the given timestamp
        int findTransIndexAt(float time) const;
        // Find index of a Color keyframe active at the given timestamp
        int findColorIndexAt(float time) const;

        // Direct access to Sprite keyframe data via its index
        const SpriteKey* getSpriteKey(uint16_t index) const;
        // Direct access to Transform keyframe data via its index
        const TransformKey* getTransKey(uint16_t index) const;
        // Direct access to Color keyframe data via its index
        const ColorKey* getColorKey(uint16_t index) const;

        // Total number of keyframes in the SpriteKey track.
        int spriteTrackSize() const { return sprKeys.size(); }
        // Total number of keyframes in the TransformKey track.
        int transformTrackSize() const { return tranKeys.size(); }
        // Total number of keyframes in the ColorKey track.
        int colorTrackSize() const { return colKeys.size(); }

        // Serialize animation data to string in JSON format
        std::string serialize();

    private:
        std::string name;
        float duration = 0.0f;
        std::vector<SpriteKey> sprKeys;
        std::vector<TransformKey> tranKeys; 
        std::vector<ColorKey> colKeys;
    };
};