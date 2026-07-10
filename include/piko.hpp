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
#include <memory>

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
            void showFPS(bool enabled){ drawFPS = enabled;}
            
            bool shouldCloseWindow();

            void terminate();
            void exit();

            const DeltaTime& getDt() { return dt; }
            const RenderTexture2D& getDrawCanvas() {return *drawCanvas;}

            int getWindowW();
            int getWindowH();
            
            int getDrawCanvasW();
            int getDrawCanvasH();

            AssetManager& assets() { return *assetMAN.get(); }
            SceneManager& scenes() { return *sceneMAN.get(); }
            Renderer& renderer() { return *renderMAN.get(); }
            PhysicsEngine& physics() { return *physicsMAN.get(); }
            InputManager& inputs() { return *inputMAN.get(); }
            AudioManager& audio() { return *audioMAN.get(); }
            Cam& camera()   {return activeCam;}

        private:
            std::unique_ptr<AssetManager> assetMAN;
            std::unique_ptr<AudioManager> audioMAN;
            std::unique_ptr<InputManager> inputMAN;
            std::unique_ptr<PhysicsEngine> physicsMAN;
            std::unique_ptr<Renderer> renderMAN;
            std::unique_ptr<SceneManager> sceneMAN;
            
            bool exitWindow = false;
            bool drawFPS = false;

            RenderTexture2D* drawCanvas = nullptr;

            const RenderShader* activeShader = nullptr;
            EventBroker eventBroker;
            Cam activeCam;

            DeltaTime dt;
    };
}
