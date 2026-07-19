#include "piko/components/collidableComp.hpp"
#include "piko/scene.hpp"

#include "json.hpp"

using json = nlohmann::json;
using namespace piko;

void Collidable::setBody(const std::string& bodyname){
    PhysicsBody* pBody = owner->scene->getComponent<PhysicsBody>(ownerID, bodyname);
    if(pBody){
        pBody->attachCollider(this);
        dynamic = true;
        pbodyID = pBody->getID();
    } 
    parentBody = pBody;
}

void Collidable::setBody(uint32_t id){
    PhysicsBody* pBody = owner->scene->getComponent<PhysicsBody>(id);
    if(pBody){
        pBody->attachCollider(this);
        dynamic = true;
        pbodyID = pBody->getID();
    }
    parentBody = pBody;
}

void Collidable::removeBody(){
    if(parentBody){
        parentBody->detachCollider(this);
    }
    parentBody = nullptr;
}

void Collidable::terminate(){
    removeBody();

    Component::terminate();
}


std::string Collidable::serialize(){
    json data = json::parse(Component::serialize());
    data["trigger"] = trigger;
    data["dynamic"] = dynamic;
    data["kinematic"] = kinematic;
    if(parentBody){
        data["parentBody"] = parentBody->getAlias();
    }

    return data.dump();
}

void Collidable::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);
    trigger = data.value("trigger", false);
    dynamic = data.value("dynamic", false);
    kinematic = data.value("kinematic", false);
    parentBody = nullptr;
    if (data.contains("parentBody")) {
        std::string parentBodyName = data.value("parentBody", "");
        owner->scene->addPostLoadJob([this, parentBodyName]() {
            this->setBody(parentBodyName);
        });
    }
}

void PhysicsBody::terminate(){
    std::vector<Collidable*> coll_copy = colliders;
    for (Collidable* col : coll_copy) {
        if (col) {
            col->removeBody();
        }
    }
    colliders.clear();

    Component::terminate();
}

std::string PhysicsBody::serialize(){
    json data = json::parse(Component::serialize());
    
    data["velocity"] = {
        {"x", velocity.x},
        {"y", velocity.y}
    };
   
    data["gravityScale"] = gravityScale;
    data["frictionScale"] = frictionScale;
    data["dragScale"] = dragScale;

    return data.dump();
}

void PhysicsBody::deserialize(const std::string& rawJson){
    Component::deserialize(rawJson);
    json data = json::parse(rawJson);

    if (data.contains("velocity") && data["velocity"].is_object()) {
        auto& velJson = data["velocity"];
        velocity.x = velJson.value("x", 0.0f);
        velocity.y = velJson.value("y", 0.0f);
    }
    
    gravityScale = data.value("gravityScale", 1.0f);
    frictionScale = data.value("frictionScale", 1.0f);
    dragScale = data.value("dragScale", 1.0f);
}