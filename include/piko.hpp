#pragma once

#include "piko/time.hpp"
#include "piko/scene.hpp"
#include "piko/assets.hpp"
#include "piko/renderer.hpp"
#include "piko/physics.hpp" 
#include "piko/input.hpp"
#include "piko/audio.hpp"
#include "piko/cam.hpp"
#include "piko/event.hpp"
#include "piko/logger.hpp"

struct RenderTexture;
typedef RenderTexture RenderTexture2D;

namespace piko {

    class Engine{
        public:
            Engine();
            ~Engine() = default;
            Engine(const Engine&) = delete;
            Engine& operator=(const Engine&) = delete;

            void init(const char* title, int width, int height, bool fullscreen=false, bool resizeable=false, int targetFPS=60);
            void update();
            void draw();

            void setDrawBufferSize(int width, int height);

            void setWinSize(int width, int height);
            void setFullScreen(bool fullscreen);

            void setTitle(const char* title);
            
            bool shouldCloseWindow();
            bool isRunning(){return running;}

            void terminate();
            void exit();

            const DeltaTime& getDt() { return dt; }

            AssetManager& assets() { return *assetMAN; }
            SceneManager& scenes() { return *sceneMAN; }
            Renderer& renderer() { return *renderMAN; }
            PhysicsEngine& physics() { return *physicsMAN; }
            InputManager& input() { return *inputMAN; }
            AudioManager& audio() { return *audioMAN; }
            Cam& camera()   {return activeCam;}

        private:
            SceneManager* sceneMAN = nullptr;
            AssetManager* assetMAN = nullptr;
            InputManager* inputMAN = nullptr;
            AudioManager* audioMAN = nullptr;
            Renderer* renderMAN = nullptr;
            PhysicsEngine* physicsMAN = nullptr;

            bool running = true;

            RenderTexture2D* drawLayer0 = nullptr;
            RenderTexture2D* drawLayer1 = nullptr;

            RenderShader* activeShader = nullptr;
            EventBroker eventBroker;
            Cam activeCam;

            DeltaTime dt;
    };
}
