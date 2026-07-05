#pragma once
#include <string>
#include <vector>

#include "math.hpp"

namespace piko{
    struct Sprite;

    struct AnimationFrame {
        const Sprite* sprite = nullptr;
        float duration = 0.2f;
        Vect2 offset = {0.0f, 0.0f};
    };

    class AnimationClip{
        public:
            AnimationClip(const std::vector<AnimationFrame>& frames, const std::string& name);

            const std::string& getName() const {return name;}
            int getSize() const { return frames.size();}
            float getDuration() const { return duration; }
            const AnimationFrame* getFrame(uint16_t index) const;

            std::string serialize();

        private:
            std::string name;
            float duration = 0.0f;
            std::vector<AnimationFrame> frames;
    };

};