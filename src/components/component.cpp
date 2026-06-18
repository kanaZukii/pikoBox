#include "piko/component.hpp"
#include "piko/scene.hpp"
#include "piko/audio.hpp"
#include "piko/audioClip.hpp"

#include "raylib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace piko;

std::vector<uint32_t>  Component::discardedIDs = {};
uint32_t Component::nextID = 0;

void Component::setOffset(Vect2 position){
    offsetPos.x = position.x;
    offsetPos.y = position.y;
    isTransformDirty = true;
}

void Component::setSize(Vect2 size){
    this->size.x = size.x;
    this->size.y = size.y;
    isTransformDirty = true;
}

const Rect& Component::getGlobalTransform() {
    if (isTransformDirty && owner) {
        const Rect& ownerT = owner->transform;

        // PHASE 1: EVALUATE SIZING RULES
        switch(sizing) {
            case SIZING::STRETCH_FILL:
                size.x = ownerT.w;
                size.y = ownerT.h;
                break;
            case SIZING::MANUAL:
            default:
                break;
        }

        // PHASE 2: CALCULATE LOCALIZED ANCHOR SHIFTS
        Vect2 anchorOffset = {0.0f, 0.0f};
        
        switch(anchor) {
            case ANCHOR::TOP_LEFT:
                anchorOffset.x = 0.0f;
                anchorOffset.y = 0.0f;
                break;
                
            case ANCHOR::CENTER:
                anchorOffset.x = (ownerT.w - size.x) / 2.0f;
                anchorOffset.y = (ownerT.h - size.y) / 2.0f;
                break;
                
            case ANCHOR::BOTTOM_MID:
                anchorOffset.x = (ownerT.w - size.x) / 2.0f;
                anchorOffset.y = ownerT.h - size.y;
                break;
        }

        // PHASE 3: STACK ABSOLUTE TRANSFORM MATRIX
        // Owner position + Automatic Anchor Alignment + Your Manual offsetPos padding!
        globalTransform.x = ownerT.x + anchorOffset.x + offsetPos.x;
        globalTransform.y = ownerT.y + anchorOffset.y + offsetPos.y;
        globalTransform.w = size.x;
        globalTransform.h = size.y;
        
        isTransformDirty = false;
    }
    return globalTransform;
}

void Component::setSizing(SIZING sizing){
    this->sizing = sizing;
    isTransformDirty = true;
}

void Component::setAnchor(ANCHOR anchor){
    this->anchor = anchor;
    isTransformDirty = true;
}

bool Component::isMouseOver(const Vect2& rawMousePos) {
    const Rect& bounds = getGlobalTransform();

    return (rawMousePos.x >= bounds.x && 
            rawMousePos.x <= bounds.x + bounds.w &&
            rawMousePos.y >= bounds.y && 
            rawMousePos.y <= bounds.y + bounds.h);
}

std::string Component::serialize(){
    json data = {
        {"id", ID},
        {"className", className},
        {"offsetPos", { {"x", offsetPos.x}, {"y", offsetPos.y}}},
        {"size", { {"x", size.x}, {"y", size.y}}},
        {"anchor", static_cast<int>(anchor)},
        {"sizing", static_cast<int>(sizing)},
        {"doUpdate", doUpdate}
    };

    return data.dump();
}

void Component::deserialize(const std::string& rawJson){
    json data = json::parse(rawJson);
    
    if (data.contains("offsetPos")) {
        offsetPos.x = data["offsetPos"]["x"].get<float>();
        offsetPos.y = data["offsetPos"]["y"].get<float>();
    }
    
    if (data.contains("size")) {
        size.x = data["size"]["x"].get<float>();
        size.y = data["size"]["y"].get<float>();
    }

    if(data.contains("doUpdate")) doUpdate = data["doUpdate"].get<bool>();

    int raw_anchor = data.value("anchor", 0);
    anchor = static_cast<ANCHOR>(raw_anchor);

    int raw_sizing = data.value("sizing", 0);
    sizing = static_cast<SIZING>(raw_sizing);

    invalidateTransform();
}