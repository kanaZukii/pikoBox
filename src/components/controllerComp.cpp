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
    isPlaying = true;
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
            this->rendererPtr = this->owner->scene->getComponent<SpriteRenderer>(this->owner->id, rendererStr);
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

void AudioPlayer::addTrack(const std::string& trackName, const AudioClip* clip) {
    if (clip) {
        tracks[trackName] = clip;
    }
}

void AudioPlayer::play(const std::string& trackName, bool loop) {
    auto it = tracks.find(trackName);
    if (it == tracks.end()) return;

    activeClip = it->second;
    currentTrackName = trackName;

    // Dispatches to your abstract mixing board using the clip's hardcoded channel
    owner->scene->getAudio()->playClip(activeClip, loop);
}

void AudioPlayer::stop() {
    if (activeClip) {
        owner->scene->getAudio()->stopChannelStream(activeClip->getDefaultChannel());
        activeClip = nullptr;
        currentTrackName = "";
    }
}

void AudioPlayer::setVolumeModifier(float volume) {
    volumeMod = volume;
    if (activeClip) {
        owner->scene->getAudio()->setChannelVolume(activeClip->getDefaultChannel(), volume);
    }
}

std::string AudioPlayer::serialize(){
    json data = json::parse(Component::serialize());
    data["volumeMod"] = volumeMod;

    if(activeClip){
        data["activeClip"] = activeClip->getFilePath();
    }

    if(!currentTrackName.empty()){
        data["currentTrackName"] = currentTrackName;
    }

    if(!tracks.empty()){
        json serializedTracks = json::object();
        for(const auto& [key, val] : tracks){
            if(val){
                serializedTracks[key] = val->getFilePath();
            }
        }

        data["tracks"] = serializedTracks;
    }

    return data.dump();
} 

void AudioPlayer::deserialize(const std::string& rawJson) {
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);
    
    volumeMod = data.value("volumeMod", 1.0f);
    currentTrackName = data.value("currentTrackName", "");

    activeClip = nullptr;
    if(data.contains("activeClip")){
        std::string audioPath = data.value("activeClip", "");
        activeClip = owner->scene->getAssets()->getAudioClipByPath(audioPath);
    }

    if (data.contains("tracks") && data["tracks"].is_object()) {
        this->tracks.clear();
        auto& serializedTracks = data["tracks"];
        
        for (const auto& [trackName, filePathJson] : serializedTracks.items()) {
            if (filePathJson.is_string()) {
                std::string pathStr = filePathJson.get<std::string>();
                const AudioClip* clip = owner->scene->getAssets()->getAudioClipByPath(pathStr);

                if (clip) {
                    this->tracks[trackName] = clip;
                }
            }
        }
    }
}