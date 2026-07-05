#include "piko/animation.hpp"
#include "piko/sprite.hpp"
#include "json.hpp"

#include <stdexcept>

using json = nlohmann::json;
using namespace piko;

AnimationClip::AnimationClip(const std::vector<AnimationFrame>& frames, const std::string& name){
    if (frames.empty()) throw std::runtime_error("Cannot create an AnimationClip with no frames.");
    this->frames = frames;
    this->name = name;
    for(const AnimationFrame& frame : this->frames){
        duration += frame.duration;
    }
}

const AnimationFrame* AnimationClip::getFrame(uint16_t index) const { 
    return (index < frames.size()) ? &frames[index] : nullptr; 
}


std::string AnimationClip::serialize(){
    json serializedFrames = json::array();
    
    for(const AnimationFrame& f : frames ){
        json frameJson = {
            {"duration", f.duration},
            {"offset", {{"x", f.offset.x}, {"y", f.offset.y}}}
        };
        
        if(f.sprite){
            json spriteJson = {
                {"sheet", f.sprite->sheet}
            };
            if(f.sprite->index > -1) {
                spriteJson["index"] = f.sprite->index;
            }
            frameJson["sprite"] = spriteJson;
        }
        
        serializedFrames.push_back(frameJson);
    }
    json data = {
        {"frames", serializedFrames}
    };
    
    return data.dump();
}