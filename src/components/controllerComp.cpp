#include "piko/components/controllerComp.hpp"
#include "piko/scene.hpp"
#include "piko/assets.hpp"
#include "piko/audio.hpp"
#include "piko/audioClip.hpp"

#include "raylib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace piko;


void AnimationPlayer::update(float dt){
    if (!isPlaying || !currentClip) return;

    if(rendererPtr && !owner->scene->componentExist(rendererID)){
        rendererPtr = nullptr;
        stop();
    }

    // 1. Lazy-fallback to check if the target renderer string version was set
    if (!rendererPtr || currentClip->frames.empty()) return; 

    // 2. Accumulate delta time
    frameTimer += dt;

    // 3. Evaluate Frame Transitions
    const auto& currentFrame = currentClip->frames[currentFrameIndex];
    if (frameTimer >= currentFrame.duration) {
        frameTimer -= currentFrame.duration;
        currentFrameIndex++;

        // Handle looping limits
        if (currentFrameIndex >= currentClip->frames.size()) {
            if (currentClip->loop) {
                currentFrameIndex = 0;
            } else {
                currentFrameIndex = currentClip->frames.size() - 1;
                isPlaying = false; // Halt playback
            }
        }
    }

    // 4. Update the Active Renderer Profile
    const auto& activeFrame = currentClip->frames[currentFrameIndex];
    if (activeFrame.sprite) {
        // Assign the active frame's texture and source rect bounds
        rendererPtr->setSprite(activeFrame.sprite);
        
        // Pass the frame's origin offset down to the renderer's rendering origin!
        rendererPtr->setOffset(activeFrame.offset);
    }
}

void AnimationPlayer::setRenderer(SpriteRenderer* renderer){
    rendererPtr = renderer;
    if(rendererPtr){
        rendererID = rendererPtr->getID();
    }
}

void AnimationPlayer::setRenderer(uint32_t id){
    rendererID = id;
    rendererPtr = owner->scene->getComponent<SpriteRenderer>(rendererID);
}

void AnimationPlayer::setRenderer(const std::string& sprrenderer){
    rendererPtr = owner->scene->getComponent<SpriteRenderer>(ownerID, sprrenderer);
    if(rendererPtr){
        rendererID = rendererPtr->getID();
    }
}

void AnimationPlayer::addClip(const Clip& clip){
    clips[clip.name] = clip;
}

void AnimationPlayer::play(const std::string& name){
    if (currentClip){
        if(currentClip->name == name && isPlaying) return;
    }

    auto it = clips.find(name);
    if (it != clips.end()) {
        currentClip = &it->second;
        currentFrameIndex = 0;
        frameTimer = 0.0f;
        isPlaying = true;
    }
}

void AnimationPlayer::stop(){
    isPlaying = false;
    currentFrameIndex = 0;
    frameTimer = 0.0f;
}

void AnimationPlayer::pause(){
    isPlaying = false;
}

void AnimationPlayer::resume(){
    if(currentClip){isPlaying = true;}
}

std::string AnimationPlayer::serializeClip(const Clip& clip){
    json serializedFrames = json::array();
    
    for(const Frame& f : clip.frames ){
        json frameJson = {
            {"duration", f.duration},
            {"offset", {{"x", f.offset.x}, {"y", f.offset.y}}}
        };
        
        if(f.sprite){
            json spriteJson = {
                {"sheet", f.sprite->texName}
            };
            if(f.sprite->index > -1) {
                spriteJson["index"] = f.sprite->index;
            }
            frameJson["sprite"] = spriteJson;
        }
        
        serializedFrames.push_back(frameJson);
    }
    json data = {
        {"loop", clip.loop},
        {"frames", serializedFrames}
    };
    
    return data.dump();
}

AnimationPlayer::Clip AnimationPlayer::deserializeClip(const std::string& clipName, const std::string& rawClipJson) {
    json data = json::parse(rawClipJson);
    
    Clip clip;
    clip.name = clipName;
    clip.loop = data.value("loop", true);

    if (data.contains("frames") && data["frames"].is_array()) {
        for (const auto& frameJson : data["frames"]) {
            Frame frame;
            
            frame.duration = frameJson.value("duration", 0.2f);
            
            if (frameJson.contains("offset")) {
                frame.offset.x = frameJson["offset"].value("x", 0.0f);
                frame.offset.y = frameJson["offset"].value("y", 0.0f);
            }

            if (frameJson.contains("sprite") && !frameJson["sprite"].is_null()) {
                auto& spriteJson = frameJson["sprite"];
                
                std::string sheetName = spriteJson.value("sheet", "");
                int index = spriteJson.value("index", -1);
                if(index > -1){
                    frame.sprite = owner->scene->getAssets()->getSpriteFromSheet(sheetName, index);
                } else {
                    frame.sprite = owner->scene->getAssets()->getTexSprite(sheetName);
                }
            } else {
                frame.sprite = nullptr;
            }

            // Push the fully reconstructed frame into your clip's frame pool
            clip.frames.push_back(frame);
        }
    }

    return clip;
}

std::string AnimationPlayer::serialize(){
    json data = json::parse(Component::serialize());
    data["currentFrameIndex"] = currentFrameIndex;
    data["frameTimer"] = frameTimer;
    data["isPlaying"] = isPlaying;

    if(rendererPtr){
        data["rendererPtr"] = rendererPtr->getAlias();
    }

    if(currentClip){
        data["currentClip"] = currentClip->name;
    }

    if(!clips.empty()){
        json serializedClips = json::object();
        for(const auto& [key, val] : clips) serializedClips[key] = json::parse(serializeClip(val));
        data["clips"] = serializedClips;
    }

    return data.dump();
}

void AnimationPlayer::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    isPlaying = data.value("isPlaying", false); 
    frameTimer = data.value("frameTimer", 0.0f);
    currentFrameIndex = data.value("currentFrameIndex", 0);

    rendererPtr = nullptr;
    if(data.contains("rendererPtr")){
        std::string rendererStr = data.value("rendererPtr", "");
        owner->scene->addPostLoadJob([this, rendererStr]() {
            this->setRenderer(
                this->owner->scene->getComponent<SpriteRenderer>(this->owner->id, rendererStr)
            );
        });
    }

    if (data.contains("clips") && data["clips"].is_object()) {
        this->clips.clear();

        for (const auto& [clipName, clipData] : data["clips"].items()) {
            std::string serializedClipState = clipData.dump();
            this->clips[clipName] = deserializeClip(clipName, serializedClipState);
        }
    }

    currentClip = nullptr;
    if(data.contains("currentClip")){
        std::string clipName = data.value("currentClip", "");
        auto it = this->clips.find(clipName);
        if(it != this->clips.end()) {
            currentClip = &(it->second);
        }
    }
}

void AudioPlayer::play(const std::string& audio, bool loop, int channel, float startAt) {
    const AudioClip* newAudio = owner->scene->getAssets()->get<AudioClip>(audio);
    if(!newAudio) return;

    
    if (newAudio->getType() == AudioClip::AudioType::STREAM_MUSIC && newAudio != currentClip) {
        currentClip = newAudio;
        currentChannel = channel;
        owner->scene->getAudio()->playClip(currentClip, loop, channel);
    } else if (newAudio->getType() == AudioClip::AudioType::STATIC_SFX) {
        owner->scene->getAudio()->playClip(newAudio, loop, channel);
    }
}

void AudioPlayer::play(const AudioClip* audio, bool loop, int channel, float startAt){
    if(!audio) return;

    
    if (audio->getType() == AudioClip::AudioType::STREAM_MUSIC && audio != currentClip) {
        currentClip = audio;
        currentChannel = channel;
        owner->scene->getAudio()->playClip(currentClip, loop, channel);
    }else if (audio->getType() == AudioClip::AudioType::STATIC_SFX) {
        owner->scene->getAudio()->playClip(audio, loop, channel);
    }
}

void AudioPlayer::stop() {
    if (currentClip) {
        owner->scene->getAudio()->stopChannelStream(currentChannel);
        currentClip = nullptr;
    }
}

void AudioPlayer::setVolumeModifier(float volume) {
    volumeMod = volume;
    if (currentClip) {
        owner->scene->getAudio()->setChannelVolume(currentChannel, volume);
    }
}

bool AudioPlayer::isPlaying() const {
    if (!currentClip) return false;

    return owner->scene->getAudio()->isChannelPlaying(currentChannel); 
}

std::string AudioPlayer::serialize(){
    json data = json::parse(Component::serialize());
    data["volumeMod"] = volumeMod;
    data["currentChannel"] = currentChannel;

    if(currentClip){
        data["currentClip"] = currentClip->getFilePath();
    }

    return data.dump();
} 

void AudioPlayer::deserialize(const std::string& rawJson) {
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);
    
    volumeMod = data.value("volumeMod", 1.0f);
    currentChannel = data.value("currentChannel", 0);

    currentClip = nullptr;
    if(data.contains("currentClip")){
        std::string audioPath = data.value("currentClip", "");
        currentClip = owner->scene->getAssets()->getByPath<AudioClip>(audioPath);
    }
}

void CompTransformAnimator::update(float dt){
    if(targetC && !owner->scene->componentExist(targetID)) {
        targetC = nullptr;
        stop(); 
    }

    if (!targetC || !isPlaying || !currentClip || currentClip->keyframes.empty()) return;

    const TransformFrame& frame = currentClip->keyframes[currentFrameIndex];
    frameTimer += dt;

    // Establish a completely stable layout origin baseline
    Rect startTransform;
    if (currentFrameIndex > 0) {
        // Normal sequence: start from the previous keyframe's target destination
        startTransform = currentClip->keyframes[currentFrameIndex - 1].target;
    } else {
        // Rollover sequence: if looping, smoothly interpolate from the absolute final frame target!
        if (currentClip->loop && currentClip->keyframes.size() > 1) {
            startTransform = currentClip->keyframes.back().target;
        } else {
            // Fallback for absolute first-run frame entry initialization step
            startTransform = frame.target; 
        }
    }

    // Compute Alpha Clamped Execution Threshold
    float alpha = (frame.duration > 0.0f) ? (frameTimer / frame.duration) : 1.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    Rect frameTransform;
    // Apply Linear Interpolation transformations predictably using the static boundary snapshots
    frameTransform.x = startTransform.x + alpha * (frame.target.x - startTransform.x);
    frameTransform.y = startTransform.y + alpha * (frame.target.y - startTransform.y);
    frameTransform.w = startTransform.w + alpha * (frame.target.w - startTransform.w);
    frameTransform.h = startTransform.h + alpha * (frame.target.h - startTransform.h);
    
    targetC->setOffset({frameTransform.x, frameTransform.y});
    targetC->setSize({frameTransform.w, frameTransform.h});

    // Handle frame sequence steps rollover updates conditions check logic blocks
    if (frameTimer >= frame.duration) {
        // Delta overflow tracking ensures time isn't lost on frame boundary drops
        float overflowTime = (frame.duration > 0.0f) ? (frameTimer - frame.duration) : 0.0f;
        frameTimer = overflowTime; 
        currentFrameIndex++;

        if (currentFrameIndex >= static_cast<int>(currentClip->keyframes.size())) {
            if (currentClip->loop) {
                currentFrameIndex = 0;
            } else {
                isPlaying = false;
                currentFrameIndex = static_cast<int>(currentClip->keyframes.size()) - 1;
                frameTimer = 0.0f;
            }
        }
    }
}

void CompTransformAnimator::setTargetComponent(uint32_t c){
    targetC = owner->scene->getComponent<Component>(c);
    targetID = c;
}

void CompTransformAnimator::setTargetComponent(Component* c){
    targetC = c;
    if(targetC){
        targetID = targetC->getID();
    }
}

void CompTransformAnimator::addClip(const TransformClip& clip){
    clips[clip.name] = clip;
}

void CompTransformAnimator::play(const std::string& name){
    if (currentClip){
        if(currentClip->name == name && isPlaying) return;
    }

    auto it = clips.find(name);
    if (it != clips.end()) {
        currentClip = &it->second;
        currentFrameIndex = 0;
        frameTimer = 0.0f;
        isPlaying = true;
    }
}

void CompTransformAnimator::stop(){
    currentFrameIndex = 0;
    frameTimer = 0.0f;
    isPlaying = false;
}

void CompTransformAnimator::pause(){
    isPlaying = false;
}

void CompTransformAnimator::resume(){
    if(currentClip){isPlaying = true;}
}

std::string CompTransformAnimator::serialize(){
    json data = json::parse(Component::serialize());
    data["currentFrameIndex"] = currentFrameIndex;
    data["frameTimer"] = frameTimer;
    data["isPlaying"] = isPlaying;

    if(currentClip){
        data["currentClip"] = currentClip->name;
    }

    if(targetC){
        data["targetC"] = targetC->getAlias();
    }

    if(!clips.empty()){
        json serializedClips = json::object();
        for(const auto& [key, val] : clips) serializedClips[key] = json::parse(serializeClip(val));
        data["clips"] = serializedClips;
    }
    return data.dump();
}

void CompTransformAnimator::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    isPlaying = data.value("isPlaying", false); 
    frameTimer = data.value("frameTimer", 0.0f);
    currentFrameIndex = data.value("currentFrameIndex", 0);

    if (data.contains("clips") && data["clips"].is_object()) {
        this->clips.clear();

        for (const auto& [clipName, clipData] : data["clips"].items()) {
            std::string serializedClipState = clipData.dump();
            this->clips[clipName] = deserializeClip(clipName, serializedClipState);
        }
    }

    targetC = nullptr;
    if(data.contains("targetC")){
        std::string compStr = data.value("targetC", "");
        owner->scene->addPostLoadJob([this, compStr]() {
            this->setTargetComponent(
                this->owner->scene->getComponent<Component>(this->owner->id, compStr)
            );
        });
    }

    currentClip = nullptr;
    if(data.contains("currentClip")){
        std::string clipName = data.value("currentClip", "");
        auto it = this->clips.find(clipName);
        if(it != this->clips.end()) {
            currentClip = &(it->second);
        }
    }
}

std::string CompTransformAnimator::serializeClip(const TransformClip& clip){
    json serializedKeyframes = json::array();
    
    for(const TransformFrame& f : clip.keyframes ){
        json frameJson = {
            {"duration", f.duration},
            {"target", {
                {"x", f.target.x}, {"y", f.target.y}, 
                {"w", f.target.w}, {"h", f.target.h}
            }}
        };
        serializedKeyframes.push_back(frameJson);
    }
    json data = {
        {"loop", clip.loop},
        {"keyframes", serializedKeyframes}
    };
    
    return data.dump();
}

CompTransformAnimator::TransformClip CompTransformAnimator::deserializeClip(
    const std::string& clipName, const std::string& rawJson){

    json data = json::parse(rawJson);
    
    TransformClip clip;
    clip.name = clipName;
    clip.loop = data.value("loop", true);

    if (data.contains("keyframes") && data["keyframes"].is_array()) {
        for (const auto& frameJson : data["keyframes"]) {
            TransformFrame keyframe;
            
            keyframe.duration = frameJson.value("duration", 0.2f);
            
            if (frameJson.contains("target")) {
                keyframe.target.x = frameJson["target"].value("x", 0.0f);
                keyframe.target.y = frameJson["target"].value("y", 0.0f);
                keyframe.target.w = frameJson["target"].value("w", 0.0f);
                keyframe.target.h = frameJson["target"].value("h", 0.0f);
            }

            clip.keyframes.push_back(keyframe);
        }
    }

    return clip;
}

void DrawColorAnimator::update(float dt){
    if(targetD && !owner->scene->componentExist(targetID)) {
        targetD = nullptr;
        stop(); 
    }

    if (!targetD || !isPlaying || !currentClip || currentClip->keyframes.empty()) return;
     
    const ColorFrame& frame = currentClip->keyframes[currentFrameIndex];
    frameTimer += dt;

    // Establish a completely stable color origin
    Color4 startColor;
    if (currentFrameIndex > 0) {
        // Normal sequence: start from the previous keyframe's target color
        startColor = currentClip->keyframes[currentFrameIndex - 1].target;
    } else {
        // Rollover sequence: if looping, smoothly interpolate from the absolute final color target!
        if (currentClip->loop && currentClip->keyframes.size() > 1) {
            startColor = currentClip->keyframes.back().target;
        } else {
            // Fallback for absolute first-run frame entry initialization step
            startColor = frame.target; 
        }
    }

    // Compute Alpha Clamped Execution Threshold
    float alpha = (frame.duration > 0.0f) ? (frameTimer / frame.duration) : 1.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    Color4 frameColor;
    frameColor.r = static_cast<uint8_t>(startColor.r + alpha * (static_cast<float>(frame.target.r) - startColor.r));
    frameColor.g = static_cast<uint8_t>(startColor.g + alpha * (static_cast<float>(frame.target.g) - startColor.g));
    frameColor.b = static_cast<uint8_t>(startColor.b + alpha * (static_cast<float>(frame.target.b) - startColor.b));
    frameColor.a = static_cast<uint8_t>(startColor.a + alpha * (static_cast<float>(frame.target.a) - startColor.a));
    
    targetD->setColor(frameColor);

    // Handle frame sequence steps rollover updates conditions check logic blocks
    if (frameTimer >= frame.duration) {
        // Delta overflow tracking ensures time isn't lost on frame boundary drops
        float overflowTime = (frame.duration > 0.0f) ? (frameTimer - frame.duration) : 0.0f;
        frameTimer = overflowTime; 
        currentFrameIndex++;

        if (currentFrameIndex >= static_cast<int>(currentClip->keyframes.size())) {
            if (currentClip->loop) {
                currentFrameIndex = 0;
            } else {
                isPlaying = false;
                currentFrameIndex = static_cast<int>(currentClip->keyframes.size()) - 1;
                frameTimer = 0.0f;
            }
        }
    }
}

void DrawColorAnimator::addClip(const ColorClip& clip){
    clips[clip.name] = clip;
}

void DrawColorAnimator::play(const std::string& name){
    if (currentClip){
        if(currentClip->name == name && isPlaying) return;
    }

    auto it = clips.find(name);
    if (it != clips.end()) {
        currentClip = &it->second;
        currentFrameIndex = 0;
        frameTimer = 0.0f;
        isPlaying = true;
    }
}

void DrawColorAnimator::stop(){
    currentFrameIndex = 0;
    frameTimer = 0.0f;
    isPlaying = false;
}

void DrawColorAnimator::pause(){
    isPlaying = false;
}

void DrawColorAnimator::resume(){
    if(currentClip){isPlaying = true;}
}

void DrawColorAnimator::setTargetDrawable(uint32_t d){
    targetD = owner->scene->getComponent<Drawable>(d);
    targetID = d;
}

void DrawColorAnimator::setTargetDrawable(Drawable* d){
    targetD = d;
    if(targetD){
        targetID = targetD->getID();
    }
}

std::string DrawColorAnimator::serializeClip(const ColorClip& clip){
    json serializedKeyframes = json::array();
    
    for(const ColorFrame& f : clip.keyframes ){
        json frameJson = {
            {"duration", f.duration},
            {"target", {
                {"r", f.target.r}, {"g", f.target.g}, 
                {"b", f.target.b}, {"a", f.target.a}
            }}
        };
        serializedKeyframes.push_back(frameJson);
    }
    json data = {
        {"loop", clip.loop},
        {"keyframes", serializedKeyframes}
    };
    
    return data.dump();
}

DrawColorAnimator::ColorClip DrawColorAnimator::deserializeClip(
    const std::string& clipName, const std::string& rawJson){

    json data = json::parse(rawJson);
    
    ColorClip clip;
    clip.name = clipName;
    clip.loop = data.value("loop", true);

    if (data.contains("keyframes") && data["keyframes"].is_array()) {
        for (const auto& frameJson : data["keyframes"]) {
            ColorFrame keyframe;
            
            keyframe.duration = frameJson.value("duration", 0.2f);
            
            if (frameJson.contains("target")) {
                keyframe.target.r = frameJson["target"].value("r", 0);
                keyframe.target.g = frameJson["target"].value("g", 0);
                keyframe.target.b = frameJson["target"].value("b", 0);
                keyframe.target.a = frameJson["target"].value("a", 0);
            }

            clip.keyframes.push_back(keyframe);
        }
    }

    return clip;
}

std::string DrawColorAnimator::serialize(){
    json data = json::parse(Component::serialize());
    data["currentFrameIndex"] = currentFrameIndex;
    data["frameTimer"] = frameTimer;
    data["isPlaying"] = isPlaying;

    if(currentClip){
        data["currentClip"] = currentClip->name;
    }

    if(targetD){
        data["targetD"] = targetD->getAlias();
    }

    if(!clips.empty()){
        json serializedClips = json::object();
        for(const auto& [key, val] : clips) serializedClips[key] = json::parse(serializeClip(val));
        data["clips"] = serializedClips;
    }
    return data.dump();
}

void DrawColorAnimator::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    isPlaying = data.value("isPlaying", false); 
    frameTimer = data.value("frameTimer", 0.0f);
    currentFrameIndex = data.value("currentFrameIndex", 0);

    if (data.contains("clips") && data["clips"].is_object()) {
        this->clips.clear();

        for (const auto& [clipName, clipData] : data["clips"].items()) {
            std::string serializedClipState = clipData.dump();
            this->clips[clipName] = deserializeClip(clipName, serializedClipState);
        }
    }

    targetD = nullptr;
    if(data.contains("targetD")){
        std::string compStr = data.value("targetD", "");
        owner->scene->addPostLoadJob([this, compStr]() {
            this->setTargetDrawable(
                this->owner->scene->getComponent<Drawable>(this->owner->id, compStr)
            );
        });
    }

    currentClip = nullptr;
    if(data.contains("currentClip")){
        std::string clipName = data.value("currentClip", "");
        auto it = this->clips.find(clipName);
        if(it != this->clips.end()) {
            currentClip = &(it->second);
        }
    }
}