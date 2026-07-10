#include "piko/components/controllerComp.hpp"
#include "piko/scene.hpp"
#include "piko/assets.hpp"
#include "piko/audio.hpp"
#include "piko/audioClip.hpp"
#include "piko/animation.hpp"

#include "raylib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace piko;


void AnimationPlayer::update(float dt) {
    if (!playing || !currentClip) return;

    // 1. Safety check for the renderer
    if (rendererPtr && !owner->scene->componentExist(rendererID)) {
        rendererPtr = nullptr;
        stop();
        return;
    }
    if (!rendererPtr) return;

    // 2. Accumulate global playback time
    time += dt;

    // 3. Handle Loop / Stop logic
    if (time >= currentClip->getDuration()) {
        if (onLoop) {
            time = fmod(time, currentClip->getDuration());
        } else {
            time = currentClip->getDuration();
            playing = false;
        }
    }

    // 4. Update Tracks (Snap/Non-Interpolated version)
    
    // SPRITE TRACK
    if (currentClip->spriteTrackSize() > 0) {
        int idx = currentClip->findSpriteIndexAt(time);
        if (idx != -1) {
            const auto* key = currentClip->getSpriteKey((uint16_t)idx);
            if (key) { rendererPtr->setSprite(key->sprite); }
        }
    }

    // TRANSFORM TRACK
    if (currentClip->transformTrackSize() > 0) {
        int idx = currentClip->findTransIndexAt(time);
        
        // We need the current key and the "next" key to lerp
        const auto* currentKey = currentClip->getTransKey((uint16_t)idx);
        
        if (lerpTransform && idx + 1 < currentClip->transformTrackSize()) {
            const auto* nextKey = currentClip->getTransKey((uint16_t)idx + 1);
            
            // Calculate the range of this segment
            float segmentDuration = nextKey->time - currentKey->time;
            float localTime = time - currentKey->time;
            float alpha = (segmentDuration > 0.0f) ? (localTime / segmentDuration) : 1.0f;

            // Linear Interpolation
            float x = currentKey->transform.x + alpha * (nextKey->transform.x - currentKey->transform.x);
            float y = currentKey->transform.y + alpha * (nextKey->transform.y - currentKey->transform.y);
            float w = currentKey->transform.w + alpha * (nextKey->transform.w - currentKey->transform.w);
            float h = currentKey->transform.h + alpha * (nextKey->transform.h - currentKey->transform.h);
            
            rendererPtr->setOffset({x, y});
            rendererPtr->setSize({w, h});
        } else {
            // Snap behavior (if not lerping, or if it's the very last frame)
            if (currentKey) {
                rendererPtr->setOffset({currentKey->transform.x, currentKey->transform.y});
                rendererPtr->setSize({currentKey->transform.w, currentKey->transform.h});
            }
        }
    }

    // COLOR TRACK
    if (currentClip->colorTrackSize() > 0) {
        int idx = currentClip->findColorIndexAt(time);
        
        // Get the current key
        const auto* currentKey = currentClip->getColorKey((uint16_t)idx);
        
        if (lerpColor && idx + 1 < currentClip->colorTrackSize()) {
            const auto* nextKey = currentClip->getColorKey((uint16_t)idx + 1);
            
            // Calculate segment alpha
            float segmentDuration = nextKey->time - currentKey->time;
            float localTime = time - currentKey->time;
            float alpha = (segmentDuration > 0.0f) ? (localTime / segmentDuration) : 1.0f;
            
            // Apply Color LERP
            Color4 frameColor;
            auto lerpChannel = [&](uint8_t start, uint8_t end) -> uint8_t {
                return static_cast<uint8_t>(start + alpha * (static_cast<float>(end) - start));
            };
            
            frameColor.r = lerpChannel(currentKey->color.r, nextKey->color.r);
            frameColor.g = lerpChannel(currentKey->color.g, nextKey->color.g);
            frameColor.b = lerpChannel(currentKey->color.b, nextKey->color.b);
            frameColor.a = lerpChannel(currentKey->color.a, nextKey->color.a);
            
            rendererPtr->setColor(frameColor);
        } else {
            // Snap behavior (if not lerping, or if it's the last frame)
            if (currentKey) {
                rendererPtr->setColor(currentKey->color);
            }
        }
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

void AnimationPlayer::play(const AnimationClip* clip, bool loop){
    if(!clip){currentClip = nullptr; stop(); return;}

    if (currentClip){
        if(currentClip->getName() == clip->getName() && playing) return;
    }
    
    currentClip = clip;
    time = 0.0f;
    onLoop = loop;

    if(currentClip){
        playing = true;
    } else{
        playing = false;
    }
}

void AnimationPlayer::play(const std::string& name, bool loop){
    if (currentClip){
        if(currentClip->getName() == name && playing) return;
    }
    currentClip = owner->scene->assets()->get<AnimationClip>(name);
    time = 0.0f;
    onLoop = loop;

    if(currentClip){
        playing = true;
    } else{
        playing = false;
    }
}

void AnimationPlayer::stop(){
    playing = false;
    time = 0.0f;
}

void AnimationPlayer::resume(){
    if(currentClip){
        playing = true;
    }
}

void AnimationPlayer::pause(){
    playing = false;
}

std::string AnimationPlayer::getCurrentClipName() const {
    if(currentClip) {return currentClip->getName();}
    return "";
}

std::string AnimationPlayer::serialize(){
    json data = json::parse(Component::serialize());
    data["time"] = time;
    data["playing"] = playing;
    data["onLoop"] = onLoop;

    data["lerpTransform"] = lerpTransform;
    data["lerpColor"] = lerpColor;

    if(rendererPtr){
        data["rendererPtr"] = rendererPtr->getAlias();
    }

    if(currentClip){
        data["currentClip"] = currentClip->getName();
    }

    return data.dump();
}

void AnimationPlayer::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    playing = data.value("playing", false); 
    onLoop = data.value("onLoop", false); 
    time = data.value("time", 0.0f);
    
    lerpTransform = data.value("lerpTransform", false); 
    lerpColor = data.value("lerpColor", false);

    rendererPtr = nullptr;
    if(data.contains("rendererPtr")){
        std::string rendererStr = data.value("rendererPtr", "");
        owner->scene->addPostLoadJob([this, rendererStr]() {
            this->setRenderer(
                this->owner->scene->getComponent<SpriteRenderer>(this->owner->id, rendererStr)
            );
        });
    }

    currentClip = nullptr;
    if(data.contains("currentClip")){
        std::string clipName = data.value("currentClip", "");
        currentClip = owner->scene->assets()->get<AnimationClip>(clipName);
    }
}

void AudioPlayer::play(const std::string& audio, bool loop, int channel, float startAt) {
    const AudioClip* newAudio = owner->scene->assets()->get<AudioClip>(audio);
    if(!newAudio) {stop(); currentClip = nullptr; return;}

    
    if (newAudio->getType() == AudioClip::AudioType::STREAM_MUSIC && newAudio != currentClip) {
        currentClip = newAudio;
        currentChannel = channel;
        owner->scene->audio()->playClip(currentClip, loop, channel);
    } else if (newAudio->getType() == AudioClip::AudioType::STATIC_SFX) {
        owner->scene->audio()->playClip(newAudio, loop, channel);
    }
}

void AudioPlayer::play(const AudioClip* audio, bool loop, int channel, float startAt){
    if(!audio) {stop(); currentClip = nullptr; return;}

    
    if (audio->getType() == AudioClip::AudioType::STREAM_MUSIC && audio != currentClip) {
        currentClip = audio;
        currentChannel = channel;
        owner->scene->audio()->playClip(currentClip, loop, channel);
    }else if (audio->getType() == AudioClip::AudioType::STATIC_SFX) {
        owner->scene->audio()->playClip(audio, loop, channel);
    }
}

void AudioPlayer::stop() {
    if (currentClip) {
        owner->scene->audio()->stopChannelStream(currentChannel);
        currentClip = nullptr;
    }
}

void AudioPlayer::setVolumeModifier(float volume) {
    volumeMod = volume;
    if (currentClip) {
        owner->scene->audio()->setChannelVolume(currentChannel, volume);
    }
}

bool AudioPlayer::isPlaying() const {
    if (!currentClip) return false;

    return owner->scene->audio()->isChannelPlaying(currentChannel); 
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
        currentClip = owner->scene->assets()->getByPath<AudioClip>(audioPath);
    }
}