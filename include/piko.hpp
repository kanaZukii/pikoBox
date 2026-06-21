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
            void drawBegin();
            void drawScene();
            void drawEnd();

            void setDrawCanvasSize(int width, int height);

            void setWindowSize(int width, int height);
            void setFullScreen(bool fullscreen);

            void setTitle(const char* title);
            
            bool shouldCloseWindow();

            void terminate();
            void exit();

            const DeltaTime& getDt() { return dt; }

            int getWindowW();
            int getWindowH();
            
            int getDrawCanvasW();
            int getDrawCanvasH();

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

            bool exitWindow = false;

            RenderTexture2D* drawCanvas = nullptr;

            RenderShader* activeShader = nullptr;
            EventBroker eventBroker;
            Cam activeCam;

            DeltaTime dt;
    };
}
