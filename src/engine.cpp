#include "piko.hpp"
#include "piko/shader.hpp"
#include "global.hpp"

#include "raylib.h"

#include <string>

using namespace piko;

Engine::Engine() {
    PBOX_INFO("PIKOBOX INITIALIZING........");
    assetMAN = new AssetManager();
    renderMAN = new Renderer();
    audioMAN = new AudioManager();
    physicsMAN = new PhysicsEngine();
    inputMAN = new InputManager();
    sceneMAN = new SceneManager();

    sceneMAN->setEventBroker(&eventBroker);
    sceneMAN->setAssetsManager(assetMAN);
    sceneMAN->setInputManager(inputMAN);
    sceneMAN->setAudioManager(audioMAN);
    sceneMAN->setGameCamera(&activeCam);
}

void Engine::init(const char *title, int width, int height, bool fullscreen, bool resizeable, int targetFPS) {
    PBOX_INFO("WINDOW INITIALIZING........");
    PBOX_INFO("TITLE: %s", title);
    PBOX_INFO("DIMENSION: %dpx x %dpx", width, height);
    PBOX_INFO("TARGET FPS: %d", targetFPS);
    
    SetTraceLogLevel(LOG_NONE);

    exitWindow = false; 

    if (fullscreen){ SetConfigFlags(FLAG_FULLSCREEN_MODE);}
    if (resizeable){ SetConfigFlags(FLAG_WINDOW_RESIZABLE);}

    setWindowSize(width, height);
    setTitle(title);

    InitWindow(Global::GetVar().windowWidth, Global::GetVar().windowHeight,
                Global::GetVar().windowTitle);
    if (!IsWindowReady()){
        PBOX_ERROR("FAILED TO INITIALIZE WINDOW.");
        return;
    }
    PBOX_INFO("WINDOW SUCCESSFULLY INITIALIZED");

    SetTargetFPS(targetFPS);
    Global::GetVar().fullscreen = IsWindowFullscreen();

    drawCanvas = new RenderTexture2D;
    
    PBOX_INFO("LOADING RENDER TEXTURE......");
    *drawCanvas = LoadRenderTexture(width, height);

    Global::GetVar().drawLayer0 = drawCanvas;
    
    PBOX_INFO("SETTING CAMERA........");
    activeCam.setOffset({GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f});
    
    PBOX_INFO("SETTING SHADER TO 'default'.");
    activeShader = assetMAN->addShaderFromMemory("default", DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);

    renderMAN->init(activeShader, &activeCam);
    audioMAN->init();

    PBOX_INFO("PIKOBOX ENGINE SUCCESSFULLY INITIALIZED!");
}

void Engine::terminate() {
    PBOX_INFO("CLEANING UP.......");

    audioMAN->terminate();

    delete sceneMAN;
    delete renderMAN;
    delete assetMAN;
    delete inputMAN;
    delete physicsMAN;
    delete audioMAN;

    if (drawCanvas->id != 0) { UnloadRenderTexture(*drawCanvas);}

    delete drawCanvas;

    CloseWindow();
    PBOX_INFO("CLOSED THE WINDOW SUCCESSFULLY");
}

void Engine::update(){
    dt.raw = GetFrameTime();
    dt.physics = std::min(dt.raw, 0.05f);

    float camSpeed = 300.0f * dt.raw;

    if (IsKeyPressed(KEY_F11)) setFullScreen(!Global::GetVar().fullscreen);

    if (IsKeyPressed(KEY_F2)) TakeScreenshot("screenshot.png");
    
    if (sceneMAN->currentScene) {
        Rect simBounds = activeCam.getViewSpaceBubble();
        inputMAN->update();

        sceneMAN->updateScene(dt);

        physicsMAN->update(dt.physics, sceneMAN->currentScene, simBounds);

        for (Script* s : sceneMAN->currentScene->getScripts()) {
            if(s){
                if(!s->isUpdating()){continue;}
                s->lateUpdate(dt.physics);
            }
        }
    }

    audioMAN->update();
    sceneMAN->flushSceneDeferredCmds();
}

void Engine::drawBegin(){
    BeginDrawing();
    ClearBackground(BLACK);
}

void Engine::drawScene(){

    BeginTextureMode(*drawCanvas);
    ClearBackground(BLACK);

    activeCam.begin();
    BeginBlendMode(BLEND_ALPHA);

    sceneMAN->drawScene(*renderMAN);
    renderMAN->flush();

    activeCam.end();
    EndBlendMode();
    EndTextureMode();

    // BeginTextureMode(*drawLayer1);
    // ClearBackground(BLANK);
    // BeginBlendMode(BLEND_ALPHA);
    // DrawFPS(5, 5);
    // EndBlendMode();
    // EndTextureMode();
    ClearBackground(BLACK);

    DrawTexturePro(
        drawCanvas->texture,
        (Rectangle){0, 0, (float)drawCanvas->texture.width,
                    (float)-drawCanvas->texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
}

void Engine::drawEnd(){ 
    EndDrawing();
}

bool Engine::shouldCloseWindow(){
    return exitWindow || WindowShouldClose();
}

void Engine::setDrawCanvasSize(int width, int height) {
    PBOX_INFO("SETTING DRAW BUFFER SIZE TO %d x %d.....", width, height);
    if (drawCanvas->id != 0) { UnloadRenderTexture(*drawCanvas); }

    *drawCanvas = LoadRenderTexture(width, height);

    Global::GetVar().drawLayer0 = drawCanvas;

    Global::GetVar().canvasWidth = width;
    Global::GetVar().canvasHeight = height;

    activeCam.setOffset(
        {drawCanvas->texture.width * 0.5f, drawCanvas->texture.height * 0.5f}
    );

    PBOX_INFO("SUCCESSFULLY SET THE DRAW BUFFER SIZE.");
}

void Engine::setTitle(const char *title) {
    Global::GetVar().windowTitle = title;
    if (IsWindowReady()) {
        SetWindowTitle(title);
        PBOX_INFO("SUCCESSFULLY SET TITLE TO: '%s'.", title);
    }
}

void Engine::setWindowSize(int width, int height) {
    Global::GetVar().windowWidth = width;
    Global::GetVar().windowHeight = height;
    if (IsWindowReady()) {
        SetWindowSize(width, height);
        PBOX_INFO("SUCCESSFULLY SET WINDOW SIZE TO: %d x %d.", width, height);
    }
}

void Engine::setFullScreen(bool fullscreen) {
    Global::GetVar().fullscreen = fullscreen;
    if (IsWindowReady()) {
        if (IsWindowFullscreen() != fullscreen) {
            ToggleFullscreen();
        }
    }
}

int Engine::getWindowW(){
    return Global::GetVar().windowWidth;
}

int Engine::getWindowH(){
    return Global::GetVar().windowHeight;
}

int Engine::getDrawCanvasW(){
    return Global::GetVar().canvasWidth;
}

int Engine::getDrawCanvasH(){
    return Global::GetVar().canvasHeight;
}

void Engine::exit() { exitWindow = true; }
