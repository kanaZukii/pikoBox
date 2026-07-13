/*
    pikoBox v1.0 - A data-driven 2D game framework built on raylib for rapid game development.

    FEATURES:
    - Component-based entities for flexible, modular behavior.
    - JSON-driven scene and entity serialization.
    - Single-object engine interface for streamlined setup.
    - Built-in asset management focused on memory efficiency.
    - Built-in batched renderer for improved draw-call performance.
    - Built-in simple AABB collision detection.
    - Cross-platform support (Windows, Linux, macOS, and Web via WASM).

    DEPENDENCIES:
    - raylib (zlib/libpng license)
    - nlohmann/json (MIT license)

    LICENSE: MIT License

    Copyright (c) 2026 kanaZukii (GelBanana)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

// Include all crucial pikoBox managers and engine objects.
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

// Forward declare raylib's RenderTexture for the draw canvas.
// Allow us to avoid including the entirity of raylib in our headers.
struct RenderTexture;
typedef RenderTexture RenderTexture2D;

namespace piko {

    /*
        Engine class that acts as the central interface for pikoBox.
        It manages the lifecycle (initialization, update, terminate), 
        provides access to core systems, and controls the main game loop.
    */
    class Engine{
        public:
            Engine();
            ~Engine() = default;
            Engine(const Engine&) = delete;
            Engine& operator=(const Engine&) = delete;

            /*
                Initializes the window and all core engine managers. 
                Must be called before any other engine interactions.
            */
            void init(const char* title, int width, int height, bool fullscreen=false, bool resizeable=false, int targetFPS=60);
            
            /*
                Executes a single frame update. Processes input, updates physics, 
                update scene component logic and performs internal cleanup.
            */
            void update();

            // Prepares the frame for rendering. Must be called before drawing.
            void drawBegin();

            // Renders the active scene to the canvas and draws the canvas to the screen.
            void drawScene();

            // Finalizes the frame and handles buffer swapping.
            void drawEnd();

            // Sets a new internal canvas size.
            void setDrawCanvasSize(int width, int height);

            // Sets a new window size and resizes the window.
            void setWindowSize(int width, int height);

            // Set the window to fullscreen mode.
            void setFullScreen(bool fullscreen);

            // Set a new window title.
            void setTitle(const char* title);

            // Shows the FPS at the top left corner every frame.
            void showFPS(bool enabled){ drawFPS = enabled;}
            
            /*
                Check if the engine should end the main game loop.
                Must be included in the game loop's while condition.
            */
            bool shouldCloseWindow();

            // Execute all of the engine's cleanup methods and terminate the window instance.
            void terminate();

            // Signals the engine to end the main game loop.
            void exit();

            /*
                Returns the current DeltaTime struct, containing both raw frame 
                delta time and the physics-capped delta time.
            */
            const DeltaTime& getDt() { return dt; }
            
            // Returns the internal offscreen render texture used for drawing.
            const RenderTexture2D& getDrawCanvas() {return *drawCanvas;}

            // Returns the current window dimensions.
            int getWindowW();
            int getWindowH();
            
            // Returns the dimensions of the internal drawing canvas.
            int getDrawCanvasW();
            int getDrawCanvasH();

            // Accessors for core engine managers.
            AssetManager& assets() { return *assetMAN.get(); }
            SceneManager& scenes() { return *sceneMAN.get(); }
            Renderer& renderer() { return *renderMAN.get(); }
            PhysicsEngine& physics() { return *physicsMAN.get(); }
            InputManager& inputs() { return *inputMAN.get(); }
            AudioManager& audio() { return *audioMAN.get(); }
            Cam& camera()   {return activeCam;}

        private:
            // Unique pointers for core engine managers.
            std::unique_ptr<AssetManager> assetMAN;
            std::unique_ptr<AudioManager> audioMAN;
            std::unique_ptr<InputManager> inputMAN;
            std::unique_ptr<PhysicsEngine> physicsMAN;
            std::unique_ptr<Renderer> renderMAN;
            std::unique_ptr<SceneManager> sceneMAN;
            
            // The close window flag.
            bool exitWindow = false;

            // The flag to show FPS via raylib's DrawFPS method.
            bool drawFPS = false;

            // Canvas Texture where we draw everything to before we draw it to the screen.
            RenderTexture2D* drawCanvas = nullptr;

            // Current active shader to be use in rendering.
            const RenderShader* activeShader = nullptr;

            // The engine's event broker that allows communication between subsystems.
            EventBroker eventBroker;

            // Current active camera.
            Cam activeCam;

            // Engine's DeltaTime struct.
            DeltaTime dt;
    };
}
