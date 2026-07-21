#include "piko.hpp"
#include "piko/shader.hpp"
#include "global.hpp"

#include "raylib.h"

#include <string>

using namespace piko;

Engine::Engine(){
    PBOX_INFO("PIKOBOX INITIALIZING........");
    assetMAN = std::unique_ptr<AssetManager>(new AssetManager());
    audioMAN = std::unique_ptr<AudioManager>(new AudioManager());
    renderMAN = std::unique_ptr<Renderer>(new Renderer());
    sceneMAN = std::unique_ptr<SceneManager>(new SceneManager());
    physicsMAN = std::unique_ptr<PhysicsEngine>(new PhysicsEngine());
    inputMAN = std::unique_ptr<InputManager>(new InputManager());
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

    InitWindow(width, height, title);
    if (!IsWindowReady()){
        PBOX_ERROR("FAILED TO INITIALIZE WINDOW.");
        return;
    }
    PBOX_INFO("WINDOW SUCCESSFULLY INITIALIZED");

    Global::GetVar().windowTitle = title;

    SetTargetFPS(targetFPS);
    Global::GetVar().fullscreen = IsWindowFullscreen();

    audioMAN->init();
    assetMAN->init();
    inputMAN->init();

    drawCanvas = new RenderTexture2D;
    
    PBOX_INFO("LOADING RENDER TEXTURE......");
    *drawCanvas = LoadRenderTexture(width, height);

    Global::GetVar().drawLayer0 = drawCanvas;
    Global::GetVar().canvasWidth = width; 
    Global::GetVar().canvasHeight = height;
    
    PBOX_INFO("SETTING CAMERA........");
    activeCam.setOffset({GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f});
    #ifdef __EMSCRIPTEN__
        PBOX_INFO("SETTING SHADER TO 'default' (WebGL2 GLES).");
        // WebGL 2 requires '#version 300 es' which you will supply via your fallback strings
        activeShader = assetMAN->addShaderFromMemory("default_web", DEFAULT_WEBGL_VERTEX_SHADER, DEFAULT_WEBGL_FRAGMENT_SHADER);
    #else
        PBOX_INFO("SETTING SHADER TO 'default' (Desktop GL).");
        activeShader = assetMAN->addShaderFromMemory("default", DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    #endif
    
    renderMAN->init();
    renderMAN->setCamera(&activeCam);
    renderMAN->setShader(activeShader);
    
    sceneMAN->init();
    sceneMAN->setEventBroker(&eventBroker);
    sceneMAN->setAssetsManager(assetMAN.get());
    sceneMAN->setInputManager(inputMAN.get());
    sceneMAN->setAudioManager(audioMAN.get());
    sceneMAN->setGameCamera(&activeCam);
    physicsMAN->init();

    updateWindowInfo();

    PBOX_INFO("PIKOBOX ENGINE SUCCESSFULLY INITIALIZED!");
}

void Engine::terminate() {
    PBOX_INFO("CLEANING UP.......");

    audioMAN->terminate();
    physicsMAN->terminate();
    inputMAN->terminate();
    sceneMAN->terminate();
    renderMAN->terminate();
    assetMAN->terminate();

    if(drawCanvas){
        if (drawCanvas->id != 0) { UnloadRenderTexture(*drawCanvas);}
        delete drawCanvas;
    }
   
    CloseWindow();
    PBOX_INFO("CLOSED THE WINDOW SUCCESSFULLY");
}

void Engine::update(){
    dt.raw = GetFrameTime();
    dt.physics = std::min(dt.raw, 0.05f);

    float camSpeed = 300.0f * dt.raw;

    if (IsKeyPressed(KEY_F11)) setFullScreen(!Global::GetVar().fullscreen);

    if (IsKeyPressed(KEY_F2)) TakeScreenshot("screenshot.png");

    assetMAN->flushDeletionQueue();
    
    if (sceneMAN->currentScene) {
        Rect simBounds = activeCam.getViewSpaceBubble();
        inputMAN->update();

        sceneMAN->updateScene(dt);

        physicsMAN->update(dt.physics, sceneMAN->currentScene, simBounds);

        sceneMAN->lateUpdateScene(dt);
    }

    audioMAN->update();
    sceneMAN->flushSceneDeferredCmds();
}

void Engine::drawBegin(){
    if(IsWindowResized() || IsWindowMaximized()){
        updateWindowInfo();
    }

    BeginDrawing();
    ClearBackground(BLACK);
}

void Engine::drawScene(){
    BeginTextureMode(*drawCanvas);
    ClearBackground(BLACK);

    activeCam.begin();
    BeginBlendMode(BLEND_ALPHA);

    sceneMAN->drawScene(*renderMAN.get());
    renderMAN->flush();

    activeCam.end();
    EndBlendMode();
    EndTextureMode();

    DrawTexturePro(
        drawCanvas->texture,
        (Rectangle){0, 0, (float)drawCanvas->texture.width,
                    (float)-drawCanvas->texture.height},
        (Rectangle)Global::GetVar().screenDest,
        (Vector2){0, 0}, 0.0f, WHITE);
    
    if(drawFPS){
        DrawFPS(5, 5);
    }
}

void Engine::drawEnd(){ 
    EndDrawing();
}

bool Engine::shouldCloseWindow(){
    #ifdef __EMSCRIPTEN__
        return exitWindow; 
    #else
        return exitWindow || WindowShouldClose();
    #endif
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

    if(IsWindowReady()){updateWindowInfo();}

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
    if (IsWindowReady()) {
        SetWindowSize(width, height);
        PBOX_INFO("SUCCESSFULLY SET WINDOW SIZE TO: %d x %d.", width, height);
        updateWindowInfo();
    }
}

void Engine::updateWindowInfo(){
    Global::GetVar().windowWidth = GetScreenWidth();
    Global::GetVar().windowHeight = GetScreenHeight();

    float screenW = (float) Global::GetVar().windowWidth;
    float screenH = (float) Global::GetVar().windowHeight;

    float canvasW = screenW;
    float canvasH = screenH;
    
    if(drawCanvas){
        canvasW = (float)drawCanvas->texture.width;
        canvasH = (float)drawCanvas->texture.height;
    }
   
    // 2. Calculate scaling factor to fit within the screen
    float scale = std::min(screenW / canvasW, screenH / canvasH);

    // 3. Calculate destination rectangle (centered)
    float scaledW = canvasW * scale;
    float scaledH = canvasH * scale;
    float offsetX = (screenW - scaledW) / 2.0f;
    float offsetY = (screenH - scaledH) / 2.0f;

    Global::GetVar().screenDest = { offsetX, offsetY, scaledW, scaledH };
}

void Engine::setFullScreen(bool fullscreen) {
    Global::GetVar().fullscreen = fullscreen;
    if (IsWindowReady()) {
        if (IsWindowFullscreen() != fullscreen) {
            ToggleFullscreen();
        }
        updateWindowInfo();
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
