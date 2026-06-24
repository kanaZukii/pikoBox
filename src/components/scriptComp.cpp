#include "piko/components/scriptComp.hpp"
#include "piko/scene.hpp"
#include "piko/input.hpp" 
#include "piko/assets.hpp"
#include "piko/cam.hpp"

#include "json.hpp"
#include "raylib.h"

using json = nlohmann::json;
using namespace piko;

void CollisionResScript::onStart() {
    // Automatically hook into the event pipeline on initialization!
    owner->scene->listenToEvent<CollisionEvent>([this](const CollisionEvent& event) {
        bool isA = (event.colA->getOwnerID() == ownerID);
        bool isB = (event.colB->getOwnerID() == ownerID);
        if (!isA && !isB) return;

        Collidable* outCollider = isA ? event.colB : event.colA;

        // Optional filter: Only fire if we match a specific entity alias or component class
        if (!targetEntity.empty() && 
            outCollider->getOwner().alias != targetEntity && 
            outCollider->getClassName() != targetEntity) {
            return;
        }

        // Fire the user's custom logic pass
        if (responseCallback) {
            responseCallback(this, outCollider);
        }
    });
}

std::string CollisionResScript::serialize(){
    json data = json::parse(Component::serialize());
    data["targetEntity"] = targetEntity;
    return data.dump();
}

void CollisionResScript::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);
    targetEntity = data.value("targetEntity", "");
}

void PlayerMoveScript::setPlayerBody(uint32_t pBody){
    bodyId = pBody;
    this->pBody = owner->scene->getComponent<PhysicsBody>(bodyId);
}

void PlayerMoveScript::setPlayerBody(PhysicsBody* pBody){
    this->pBody = pBody;
    if(pBody){
        bodyId = pBody->getID();
    }
}

void PlayerMoveScript::setPlayerBody(const std::string& bodyname){
    this->pBody = owner->scene->getComponent<PhysicsBody>(ownerID, bodyname);
    if(pBody){
        bodyId = pBody->getID();
    }
}

void PlayerMoveScript::onEarlyUpdate(float dt) {
    // 1. Grab the sibling physics body component
    if (!pBody) return;

    // 2. Safely reach up to the Scene to get our decoupled Input Manager
    InputManager* input = owner->scene->getInput();
    if (!input) return;

    // 3. Horizontal Input Loop via Action Mapping Names
    if (input->isActionDown("player_move_left")) {
        pBody->velocity.x = -speed;
    } else if (input->isActionDown("player_move_right")) {
        pBody->velocity.x = speed;
    } else {
        pBody->velocity.x = 0.0f; // Stop moving when actions are released
    }

    // 4. Vertical / Platformer Input Loop
    if (topDownMode) {
        if (input->isActionDown("player_move_up")) {
            pBody->velocity.y = -speed;
        } else if (input->isActionDown("player_move_down")) {
            pBody->velocity.y = speed;
        } else {
            pBody->velocity.y = 0.0f;
        }
    } else {
        // Platformer Jump Logic (Using your fixed manifold tracking!)
        if (input->isActionPressed("player_jump") && pBody->isGrounded) {
            pBody->velocity.y = -350.0f; // Jump impulse force
            pBody->isGrounded = false;
        }
    }
}

std::string PlayerMoveScript::serialize() {
    json data = json::parse(Component::serialize());
    data["speed"] = speed;
    data["topDownMode"] = topDownMode;
    if(pBody){
        data["pBody"] = pBody->getAlias();
    }
    return data.dump();
}

void PlayerMoveScript::deserialize(const std::string& rawJson) {
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);
    speed = data.value("speed", 200.0f);
    topDownMode = data.value("topDownMode", false);
     if(data.contains("pBody")){
        std::string pbodyStr = data.value("pBody", "");
        owner->scene->addPostLoadJob([this, pbodyStr]() {
            this->setPlayerBody(pbodyStr);
        });
    }
}

void ButtonScript::attachSpriteRenderer(uint32_t sprrenderer){
    bgId = sprrenderer;
    bgrenderer = owner->scene->getComponent<SpriteRenderer>(sprrenderer);
    if(colorControl){bgrenderer->setColor(bgColor);}
}

void ButtonScript::attachTextBoxRenderer(uint32_t txtrenderer){
    txtId = txtrenderer;
    textrenderer = owner->scene->getComponent<TextBoxRenderer>(txtrenderer);
    if(colorControl){textrenderer->setColor(textColor);}
}

void ButtonScript::attachSpriteRenderer(SpriteRenderer* sprrenderer) {
    bgrenderer = sprrenderer;
    if (bgrenderer) {
        bgId = bgrenderer->getID();
        if(colorControl){bgrenderer->setColor(bgColor);}
    }
}

void ButtonScript::attachTextBoxRenderer(TextBoxRenderer* txtrenderer) {
    textrenderer = txtrenderer;
    if (textrenderer) {
        txtId = textrenderer->getID();
        if(colorControl){textrenderer->setColor(textColor);}
    }
}

void ButtonScript::attachSpriteRenderer(const std::string& sprrenderer){
    bgrenderer = owner->scene->getComponent<SpriteRenderer>(ownerID, sprrenderer);
    if (bgrenderer) {
        bgId = bgrenderer->getID();
        if(colorControl){bgrenderer->setColor(bgColor);}
    }
}

void ButtonScript::attachTextBoxRenderer(const std::string& txtrenderer){
    textrenderer = owner->scene->getComponent<TextBoxRenderer>(ownerID, txtrenderer);
    if (textrenderer) {
        txtId = textrenderer->getID();
        if(colorControl){textrenderer->setColor(textColor);}
    }
}

void ButtonScript::setLabel(const std::string& newLabel) {
    this->label = newLabel;
    if (textrenderer) {
        textrenderer->setText(label);
    }
}

void ButtonScript::onEarlyUpdate(float dt) {
    InputManager* input = owner->scene->getInput();
    if (!input) return;

    if(bgrenderer && !owner->scene->componentExist(bgId)){
        bgrenderer = nullptr;
    }

    if(textrenderer && !owner->scene->componentExist(txtId)){
        textrenderer = nullptr;
    }

    bool hovered = isMouseOver(input->getMousePos());
    float targetScale = 1.0f;

    if (hovered) {
        // 1. Dispatch Engine Events
        Scene* scene = owner->scene;
        scene->publishEvent<ButtonEvent>({this, ButtonEvent::STATE::HOVER});
        
        if (input->isMouseDown(MOUSE::LEFT)) {
            scene->publishEvent<ButtonEvent>({this, ButtonEvent::STATE::HOLD});
        }
        if (input->isMousePressed(MOUSE::LEFT)) {
            scene->publishEvent<ButtonEvent>({this, ButtonEvent::STATE::CLICK});
        }

        // 2. Set Visual Targets
        if (hoverEffect) {
            targetScale = hoverScale;
            if (bgrenderer && colorControl)   bgrenderer->setColor(bgHover);
            if (textrenderer && colorControl) textrenderer->setColor(textHover);
        }
    } else {
        // Reset Visual Targets on Leave Bounds
        targetScale = 1.0f;
        if (bgrenderer && colorControl)   bgrenderer->setColor(bgColor);
        if (textrenderer && colorControl) textrenderer->setColor(textColor);
    }

    // 3. Smooth Frame Interpolation (Lerp Math)
    if(!bgrenderer && hoverEffect) return;
    if (std::abs(currentScale - targetScale) > 0.001f) {
        currentScale += (targetScale - currentScale) * scaleSpeed * dt;
        
        const Rect& ownerT = owner->transform;
        
        Vect2 newSize = { ownerT.w * currentScale, ownerT.h * currentScale };

        if (bgrenderer) {
            bgrenderer->setSize(newSize);
        }
    }
}

std::string ButtonScript::serialize(){
    json data = json::parse(Component::serialize());
    data["label"] = label;
    data["hoverEffect"] = hoverEffect;
    data["colorControl"] = colorControl;
    data["bgColor"] = {{"r", bgColor.r},{"g", bgColor.g},{"b", bgColor.b},{"a", bgColor.a}};
    data["textColor"] = {{"r", textColor.r},{"g", textColor.g},{"b", textColor.b},{"a", textColor.a}};
    if(hoverEffect){
        data["hoverScale"] = hoverScale;
        data["bgHover"] = {{"r", bgHover.r},{"g", bgHover.g},{"b", bgHover.b},{"a", bgHover.a}};
        data["textHover"] = {{"r", textHover.r},{"g", textHover.g},{"b", textHover.b},{"a", textHover.a}};
    }

    if(bgrenderer){
        data["bgrenderer"] = bgrenderer->getAlias();
    }

    if(textrenderer){
        data["textrenderer"] = textrenderer->getAlias();
    }

    return data.dump();
}

void ButtonScript::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    label = data.value("label", "");
    hoverEffect = data.value("hoverEffect", true);
    colorControl = data.value("colorControl", true);
    hoverScale = data.value("hoverScale", 1.05f);

    if (data.contains("bgColor") && data["bgColor"].is_object()){
        auto& bgJson = data["bgColor"];
        bgColor.r = bgJson.value("r", uint8_t(255));
        bgColor.g = bgJson.value("g", uint8_t(255));
        bgColor.b = bgJson.value("b", uint8_t(255));
        bgColor.a = bgJson.value("a", uint8_t(255));
    }

    if (data.contains("textColor") && data["textColor"].is_object()){
        auto& txtJson = data["textColor"];
        textColor.r = txtJson.value("r", uint8_t(255));
        textColor.g = txtJson.value("g", uint8_t(255));
        textColor.b = txtJson.value("b", uint8_t(255));
        textColor.a = txtJson.value("a", uint8_t(255));
    }

    if (data.contains("bgHover") && data["bgHover"].is_object()){
        auto& bgHJson = data["bgHover"];
        bgHover.r = bgHJson.value("r", uint8_t(255));
        bgHover.g = bgHJson.value("g", uint8_t(255));
        bgHover.b = bgHJson.value("b", uint8_t(255));
        bgHover.a = bgHJson.value("a", uint8_t(255));
    }

    if (data.contains("textHover") && data["textHover"].is_object()){
        auto& txtHJson = data["textHover"];
        textHover.r = txtHJson.value("r", uint8_t(255));
        textHover.g = txtHJson.value("g", uint8_t(255));
        textHover.b = txtHJson.value("b", uint8_t(255));
        textHover.a = txtHJson.value("a", uint8_t(255));
    }

    bgrenderer = nullptr;
    if(data.contains("bgrenderer")){
        std::string rendererStr = data.value("bgrenderer", "");
        owner->scene->addPostLoadJob([this, rendererStr]() {
            this->attachSpriteRenderer(rendererStr);
        });
    }

    textrenderer = nullptr;
    if(data.contains("textrenderer")){
        std::string rendererStr = data.value("textrenderer", "");
        owner->scene->addPostLoadJob([this, rendererStr]() {
            this->attachTextBoxRenderer(rendererStr);
        });
    }
    
    if(!label.empty()){
        owner->scene->addPostLoadJob([this]() {
            this->setLabel(this->label);
        });
    }

}

void CameraMoveScript::setTarget(const Entity* target) { 
    targetEntity = target; 
    if(targetEntity){
        targetId = targetEntity->id;
    }
}

void CameraMoveScript::setTarget(uint32_t id){
    targetEntity = owner->scene->getEntity(id);
    targetId = id;
}

void CameraMoveScript::setTarget(const std::string& targetname){
    targetEntity = owner->scene->getEntity(targetname); 
    if(targetEntity){
        targetId = targetEntity->id;
    }
}

void CameraMoveScript::setCamPos(Vect2 position){
    Cam* camera = owner->scene->getCamera();
    if(!camera) return;
    camera->setPosition(position);
}

void CameraMoveScript::setCamOffset(Vect2 offset){
    Cam* camera = owner->scene->getCamera();
    if(!camera) return;
    camera->setOffset(offset);
}

void CameraMoveScript::onUpdate(float dt) {
    Cam* camera = owner->scene->getCamera();
    if(!camera) return;

    if(targetEntity && !owner->scene->entityExist(targetId)){
        targetEntity = nullptr;
    }

    Vect2 currentCamPos = camera->getPosition();
    if (mode == MODE::FOLLOW) {
        if (!targetEntity) return;

        // Target coordinates centered
        Vect2 targetPos = { 
                        targetEntity->transform.x + targetOffset.x, 
                        targetEntity->transform.y + targetOffset.y
                    };

        // Smooth linear interpolation (Lerp) toward target
        Vect2 newCamPos = {
            currentCamPos.x + (targetPos.x - currentCamPos.x) * lerpSpeed * dt,
            currentCamPos.y + (targetPos.y - currentCamPos.y) * lerpSpeed * dt
        };

        camera->setPosition(newCamPos);

    } else if (mode == MODE::MANUAL) {
        InputManager* input = owner->scene->getInput();
        if (!input) return;

        if (input->isActionDown("camera_move_up")) {
            camera->setPosY(currentCamPos.y - (panSpeed * dt));
        } 
        if (input->isActionDown("camera_move_down")) {
            camera->setPosY(currentCamPos.y + (panSpeed * dt));
        }

        if (input->isActionDown("camera_move_left")) {
            camera->setPosX(currentCamPos.x - (panSpeed * dt));
        } 
        if (input->isActionDown("camera_move_right")) {
            camera->setPosX(currentCamPos.x + (panSpeed * dt));
        }
    }
}

std::string CameraMoveScript::serialize(){
    json data = json::parse(Component::serialize());
    
    data["mode"] = static_cast<int>(mode);
    data["lerpSpeed"] = lerpSpeed;
    data["panSpeed"] = panSpeed;
    data["targetOffset"] = {{"x", targetOffset.x},{"y", targetOffset.y}};

    if(targetEntity){
        data["targetEntity"] = targetEntity->alias;
    }

    return data.dump();
}

void CameraMoveScript::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    mode = static_cast<MODE>(data.value("mode", 0));
    lerpSpeed = data.value("lerpSpeed", 5.0f);
    panSpeed = data.value("panSpeed", 300.0f);

    if(data.contains("targetOffset") && data["targetOffset"].is_object()){
        auto& offJson = data["targetOffset"];
        targetOffset.x = offJson.value("x", 0.0f);
        targetOffset.y = offJson.value("y", 0.0f);
    }
    targetEntity = nullptr;
    if(data.contains("targetEntity")){
        std::string targetStr = data.value("targetEntity", "");
        owner->scene->addPostLoadJob([this, targetStr]() {
            this->setTarget(targetStr);
        });
    }
}