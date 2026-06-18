#include "piko.hpp"
#include "piko/shader.hpp"
#include "global.hpp"

#include "raylib.h"

#include <string>

using namespace piko;

Engine::Engine() {
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

void Engine::init(const char *title, int width, int height, bool fullscreen,
                  bool resizeable, int targetFPS) {
    PBOX_INFO("PANDORA BOX INITIALIZING........");
    PBOX_INFO("TITLE: %s", title);
    PBOX_INFO("DIMENSION: %dpx x %dpx", width, height);
    PBOX_INFO("TARGET FPS: %d", targetFPS);
    
    SetTraceLogLevel(LOG_NONE);

    if (fullscreen)
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    if (resizeable)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    setWinSize(width, height);
    setTitle(title);

    InitWindow(Global::GetVar().windowWidth, Global::GetVar().windowHeight,
                Global::GetVar().windowTitle);
    if (!IsWindowReady())
    PBOX_ERROR("FAILED TO INITIALIZE WINDOW.");
    return;

    SetTargetFPS(targetFPS);
    Global::GetVar().fullscreen = IsWindowFullscreen();

    drawLayer0 = new RenderTexture2D;
    drawLayer1 = new RenderTexture2D;
    
    PBOX_INFO("LOADING RENDER TEXTURES......");
    *drawLayer0 = LoadRenderTexture(width, height);
    *drawLayer1 = LoadRenderTexture(width, height);

    Global::GetVar().drawLayer0 = drawLayer0;
    Global::GetVar().drawLayer1 = drawLayer1;
    
    PBOX_INFO("SETTING CAMERA........");
    activeCam.setOffset({GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f});
    
    PBOX_INFO("SETTING SHADER TO 'default'.");
    activeShader = assetMAN->addShaderFromMemory("default", DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);

    renderMAN->init(activeShader, &activeCam);
    audioMAN->init();
}

void Engine::terminate() {
    PBOX_INFO("CLEANING UP.......");
    running = false;

    audioMAN->terminate();

    delete sceneMAN;
    delete renderMAN;
    delete assetMAN;
    delete inputMAN;
    delete physicsMAN;
    delete audioMAN;

    if (drawLayer0->id != 0) {
    UnloadRenderTexture(*drawLayer0);
    }
    if (drawLayer1->id != 0) {
    UnloadRenderTexture(*drawLayer1);
    }

    delete drawLayer0;
    delete drawLayer1;

    CloseWindow();
    PBOX_INFO("CLOSED THE WINDOW SUCCESSFULLY");
}

void Engine::update(){
    dt.raw = GetFrameTime();
    dt.physics = std::min(dt.raw, 0.05f);

    float camSpeed = 300.0f * dt.raw;

    if (IsKeyPressed(KEY_F11))
        setFullScreen(!Global::GetVar().fullscreen);
    if (IsKeyPressed(KEY_F2))
        TakeScreenshot("screenshot.png");

    
    if (sceneMAN->currentScene) {
        Rect simBounds = activeCam.getViewSpaceBubble();
        inputMAN->update();

        sceneMAN->updateScene(dt);

        physicsMAN->update(dt.physics, sceneMAN->currentScene, simBounds);

        for (Script* s : sceneMAN->currentScene->getScripts()) {
            if (s) s->lateUpdate(dt.physics);
        }
    }

    audioMAN->update();
    sceneMAN->flushSceneDeferredCmds();
}

void Engine::draw(){
    BeginTextureMode(*drawLayer0);
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

    BeginDrawing();
    ClearBackground(BLACK);

    DrawTexturePro(
        drawLayer0->texture,
        (Rectangle){0, 0, (float)drawLayer0->texture.width,
                    (float)-drawLayer0->texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    DrawTexturePro(
        drawLayer1->texture,
        (Rectangle){0, 0, (float)drawLayer1->texture.width,
                    (float)-drawLayer1->texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    EndDrawing();
}

bool Engine::shouldCloseWindow(){
    return WindowShouldClose();
}

void Engine::setDrawBufferSize(int width, int height) {
    PBOX_INFO("SETTING DRAW BUFFER SIZE TO %d x %d.....", width, height);
    if (drawLayer0->id != 0) {
    UnloadRenderTexture(*drawLayer0);
    }
    if (drawLayer1->id != 0) {
    UnloadRenderTexture(*drawLayer1);
    }

    *drawLayer0 = LoadRenderTexture(width, height);
    *drawLayer1 = LoadRenderTexture(width, height);

    Global::GetVar().drawLayer0 = drawLayer0;
    Global::GetVar().drawLayer1 = drawLayer1;

    Global::GetVar().canvasWidth = width;
    Global::GetVar().canvasHeight = height;

    activeCam.setOffset(
        {drawLayer0->texture.width * 0.5f, drawLayer0->texture.height * 0.5f});
    PBOX_INFO("SUCCESSFULLY SET THE DRAW BUFFER SIZE.");
}

void Engine::setTitle(const char *title) {
    Global::GetVar().windowTitle = title;
    if (IsWindowReady()) {
    SetWindowTitle(title);
    PBOX_INFO("SUCCESSFULLY SET TITLE TO: '%s'.", title);
    }
}

void Engine::setWinSize(int width, int height) {
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

void Engine::exit() { running = false; }
