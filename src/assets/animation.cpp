#include "piko/animation.hpp"
#include "piko/sprite.hpp"
#include "json.hpp"

#include <stdexcept>

using json = nlohmann::json;
using namespace piko;

#include <algorithm> // Required for std::stable_sort

AnimationClip::AnimationClip( 
    const std::string& name, 
    const std::vector<SpriteKey>& sprKeys, 
    const std::vector<TransformKey>& tranKeys, 
    const std::vector<ColorKey>& colKeys
) : name(name), sprKeys(sprKeys), tranKeys(tranKeys), colKeys(colKeys) 
{
    if (this->sprKeys.empty() && this->tranKeys.empty() && this->colKeys.empty()) 
        throw std::runtime_error("Cannot create an AnimationClip with no keyframes.");

    // 1. Sort all tracks by time
    std::stable_sort(this->sprKeys.begin(), this->sprKeys.end(), [](const SpriteKey& a, const SpriteKey& b) {
        return a.time < b.time;
    });
    std::stable_sort(this->tranKeys.begin(), this->tranKeys.end(), [](const TransformKey& a, const TransformKey& b) {
        return a.time < b.time;
    });
    std::stable_sort(this->colKeys.begin(), this->colKeys.end(), [](const ColorKey& a, const ColorKey& b) {
        return a.time < b.time;
    });

    // 2. Determine duration as the max time found across all tracks
    float maxTime = 0.0f;
    if (!this->sprKeys.empty()) maxTime = std::max(maxTime, this->sprKeys.back().time);
    if (!this->tranKeys.empty()) maxTime = std::max(maxTime, this->tranKeys.back().time);
    if (!this->colKeys.empty()) maxTime = std::max(maxTime, this->colKeys.back().time);
    
    this->duration = maxTime;
}

int AnimationClip::findSpriteIndexAt(float time) const {
    if (sprKeys.empty()) return -1;
    
    int index = 0;
    for (int i = 0; i < sprKeys.size(); ++i) {
        if (time >= sprKeys[i].time) index = i;
        else break;
    }
    return index;
}

int AnimationClip::findTransIndexAt( float time) const {
    if (tranKeys.empty()) return -1;
    
    int index = 0;
    for (int i = 0; i < tranKeys.size(); ++i) {
        if (time >= tranKeys[i].time) index = i;
        else break;
    }
    return index;
}

int AnimationClip::findColorIndexAt(float time) const {
    if (colKeys.empty()) return -1;
    
    int index = 0;
    for (int i = 0; i < colKeys.size(); ++i) {
        if (time >= colKeys[i].time) index = i;
        else break;
    }
    return index;
}

const SpriteKey* AnimationClip::getSpriteKey(uint16_t index) const{
    if(index >= sprKeys.size()){ return nullptr;}
    return &sprKeys[index];
}

const TransformKey* AnimationClip::getTransKey(uint16_t index) const{
    if(index >= tranKeys.size()){ return nullptr;}
    return &tranKeys[index];
}

const ColorKey* AnimationClip::getColorKey(uint16_t index) const{
    if(index >= colKeys.size()){ return nullptr;}
    return &colKeys[index];
}

std::string AnimationClip::serialize(){
    json sJSON = json::array();
    json tJSON = json::array();
    json cJSON = json::array();

    for(const SpriteKey& key : sprKeys){
        
        std::string sprName = key.sprite ? key.sprite->sheet : "";
        int sprIndex = key.sprite ? key.sprite->index : 0;

        sJSON.push_back(
            {
                {"sprite", {{"sheet", sprName}, {"index", sprIndex}}},
                {"time", key.time}
            }
        );
    }

    for(const TransformKey& key : tranKeys){
        const Rect& t = key.transform;
        tJSON.push_back(
            {
                {"transform",{{"x", t.x}, {"y", t.y}, {"w", t.w}, {"h", t.h}}},
                {"time", key.time}
            }
        );
    }

    for(const ColorKey& key : colKeys){
        const Color4& c = key.color;
        cJSON.push_back(
            {
                {"color", {{"r", c.r},{"g", c.g},{"b", c.b},{"a", c.a}}},
                {"time", key.time}
            }
        );
    }
    
    json data = {
        {"sprKeys", sJSON},
        {"tranKeys", tJSON},
        {"colKeys", cJSON},
        {"name", name}
    };
    
    return data.dump();
}
