#include "piko/cam.hpp"
#include "global.hpp"

#include "raylib.h"
#include "raymath.h"

using namespace piko;

Cam::Cam(){
    camera = new Camera2D();
    camera->offset = {0.0f, 0.0f};
    camera->target = {0.0f, 0.0f};
    camera->zoom = 1.0f;
    camera->rotation = 0.0f;
}

Cam::~Cam(){
    delete camera;
}

void Cam::begin(){
    BeginMode2D(*camera);
}

void Cam::end(){
    EndMode2D();
}

void Cam::setPosition(Vect2 position){
    camera->target = position;
}

void Cam::setPosX(float x){
    camera->target.x = x;
}

void Cam::setPosY(float y){
    camera->target.y = y;
}

void Cam::setOffset(Vect2 offset){
    camera->offset = offset;
}

void Cam::setZoom(float zoom){
    if (zoom < 0.4f) zoom = 0.4f;
    camera->zoom = zoom;
}

void Cam::rotation(float rotation){
    camera->rotation = rotation;
}

Vect2 Cam::getPosition() const { return {camera->target.x, camera->target.y}; }
float Cam::getZoom() const { return camera->zoom; }

Vect2 Cam::screenToWorld(Vect2 screenPos) const {
    return TransformVect2(screenPos, getView());
}

Vect2 Cam::worldToScreen(Vect2 worldPos) const {
    return TransformVect2(worldPos, getView());
}

Mat4 Cam::getProj() const {
    float width = Global::GetVar().canvasWidth;
    float height = Global::GetVar().canvasHeight;

    return Mat4::Ortho(0.0f, width, height, 0.0f, -100.0f, 100.0f);
}

Mat4 Cam::getView() const {
    return Mat4::View2D({camera->offset.x, camera->offset.y},{camera->target.x, camera->target.y}, camera->rotation, camera->zoom);
}

Rect Cam::getViewSpaceBubble() const {
    float canvasW = Global::GetVar().canvasWidth;
    float canvasH = Global::GetVar().canvasHeight;

    // Translate top-left corner and screen dimensions into zoom-accurate world bounds
    float worldLeft = camera->target.x - (camera->offset.x / camera->zoom);
    float worldTop  = camera->target.y - (camera->offset.y / camera->zoom);
    float worldWidth  = canvasW / camera->zoom;
    float worldHeight = canvasH / camera->zoom;

    return Rect{ worldLeft, worldTop, worldWidth, worldHeight };
}

