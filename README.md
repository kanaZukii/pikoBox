# pikoBox v1.0

> **A data-driven 2D game framework built on raylib for rapid game development.**

pikoBox is a component-based 2D game framework for C++ designed to make building games fast, modular, and approachable. It combines an ECS-inspired workflow, JSON serialization, built-in asset management, batched rendering, simple physics, and an event-driven architecture behind a single engine interface.

The goal of pikoBox is to let you focus on making games instead of writing boilerplate.

---

# Features

- **Component-based Entities** for flexible, modular game objects.
- **JSON Scene & Asset Serialization** for saving and loading projects.
- **Unified Engine Interface** exposing all major subsystems from a single object.
- **Built-in Asset Manager** with shared resource management.
- **Batched 2D Renderer** for reduced draw calls and improved performance.
- **Simple AABB Physics & Collision Detection**.
- **Action-based Input System** supporting keyboard, mouse, and key combinations.
- **Event System** for decoupled gameplay logic.
- **Cross-platform Support**
  - Windows
  - Linux
  - macOS
  - Web (Emscripten / WebAssembly)

---

# Dependencies

| Library | License |
|----------|---------|
| raylib | zlib/libpng |
| nlohmann/json | MIT |

---

# License

Released under the **MIT License**.

---

# Building

## Clone the Repository

```bash
git clone --recursive https://github.com/kanazukii/pikoBox.git
```

If you forgot to clone the submodules:

```bash
git submodule update --init --recursive
```

---

## Example Project Structure

```text
MyGame/
│
├── assets/
│   ├── textures/
│   ├── audio/
│   └── fonts/
│
├── extern/
│   └── pikoBox/
│       ├── include/
│       ├── src/
│       ├── extern/
│       │   ├── raylib/
│       │   └── nlohmann/
│       └── CMakeLists.txt
│
├── src/
│   └── main.cpp
│
└── CMakeLists.txt
```

---

## Minimal CMake Project

```cmake
cmake_minimum_required(VERSION 3.16)

project(MyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build pikoBox
add_subdirectory(extern/pikoBox)

# Build your game
add_executable(MyGame
    src/main.cpp
)

# Link the engine
target_link_libraries(MyGame PRIVATE pikoBox)
```

That's all that's required to use pikoBox in a desktop application.

---

## Build

Generate the project:

```bash
cmake -B build
```

Compile:

```bash
cmake --build build
```

The executable will be generated inside the build directory.

---

## WebAssembly (Emscripten)

pikoBox also supports compiling to the web.

```bash
emcmake cmake -B build-web
cmake --build build-web
```

To package game assets:

```cmake
target_link_options(MyGame PRIVATE
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
)
```

Additional JSON or save files may also be preloaded.

---

## Engine Integration

pikoBox is intended to be embedded directly into your own CMake project.

The engine automatically:

- Builds raylib as a submodule.
- Exposes required include directories.
- Links platform dependencies.
- Builds as a static library.
- Supports desktop and WebAssembly builds.

Most projects only require:

```cmake
add_subdirectory(extern/pikoBox)

target_link_libraries(MyGame PRIVATE pikoBox)
```

No additional configuration is necessary.

---

# Hello World

Creating your first game requires only a single engine instance.

```cpp
#include <piko.hpp>

// Creates an off-centered red square.
int main()
{
    piko::Engine game;

    // Initialize engine.
    game.init("pikoBox", 1280, 720);

    // Access subsystems.
    piko::AssetManager& assets = game.assets();
    piko::SceneManager& scenes = game.scenes();
    piko::InputManager& inputs = game.inputs();
    piko::PhysicsEngine& physics = game.physics();
    piko::Cam& camera = game.camera();

    // Create scene.
    piko::Scene* game_scene =
        scenes.createScene("game_scene");

    // Create entity.
    piko::Entity* box =
        game_scene->createEntity("box");

    box->transform =
    {
        0.0f,
        0.0f,
        100.0f,
        100.0f
    };

    // Give it something to render.
    auto* spr =
        game_scene->addComponent<piko::SpriteRenderer>(
            box->id,
            "spr");

    spr->setSize({100.0f,100.0f});
    spr->setColor({255,0,0,255});

    scenes.setScene("game_scene");
    scenes.initScene();

    while (!game.shouldCloseWindow())
    {
        game.update();

        game.drawBegin();
        game.drawScene();
        game.drawEnd();
    }

    game.terminate();

    return 0;
}
```

This produces a window displaying a single red square.

---

# Engine Architecture

The engine exposes each major subsystem through a single `Engine` instance.

```text
Engine
├── AssetManager
├── SceneManager
├── PhysicsEngine
├── InputManager
├── AudioManager
├── Camera
└── Renderer
```

This keeps initialization simple while still providing full access to each subsystem.

---

# Components

Entities gain functionality by attaching components.

Making the previous box movable only requires a few additional components.

```cpp
// Physics body.
auto* body =
    game_scene->addComponent<piko::PhysicsBody>(
        "box",
        "pbody");

// Collider.
auto* collider =
    game_scene->addComponent<piko::BoxCollider>(
        "box",
        "collider");

collider->setBody(body->getID());

// Built-in movement script.
auto* move =
    game_scene->addComponent<piko::PlayerMoveScript>(
        "box",
        "pmove");

move->setPlayerBody(body->getID());
move->topDownMode = true;

// Required input bindings.
inputs.bindKey("player_move_up", piko::KEYS::UP);
inputs.bindKey("player_move_down", piko::KEYS::DOWN);
inputs.bindKey("player_move_left", piko::KEYS::LEFT);
inputs.bindKey("player_move_right", piko::KEYS::RIGHT);
```

---

# Asset Management

## Loading Assets

```cpp
assets.addTexture(
    "texture_name",
    "your_texture_filepath.png");

assets.addAudioClip(
    "audio_name",
    "your_audio_filepath.mp3",
    piko::AudioClip::AudioType::STREAM_MUSIC);

assets.addFontAtlas(
    "font_name",
    "your_font_filepath.ttf",
    32);
```

---

## Retrieving Assets

```cpp
const piko::TextureIMG* texture =
    assets.get<piko::TextureIMG>("texture_name");

const piko::AudioClip* audio =
    assets.get<piko::AudioClip>("audio_name");

const piko::FontAtlas* font =
    assets.get<piko::FontAtlas>("font_name");
```

Or by filepath:

```cpp
const piko::TextureIMG* texture =
    assets.getByPath<piko::TextureIMG>(
        "your_texture_filepath.png");
```

---

## Sprite Sheets

```cpp
assets.addSpriteSheet(
    "my_sheet",
    texture,
    {16,16},
    8);
```

---

## Animation Clips

```cpp
std::vector<piko::SpriteKey> anim_sprKeys = {
    {assets.getSpriteFromSheet("my_sheet",0),0.0f},
    {assets.getSpriteFromSheet("my_sheet",1),0.25f},
    {assets.getSpriteFromSheet("my_sheet",2),0.5f},
    {assets.getSpriteFromSheet("my_sheet",3),0.75f},
    {assets.getSpriteFromSheet("my_sheet",5),1.0f}
};

std::vector<piko::TransformKey> anim_tranKeys = {
    {{0,0,100,100},0.0f}
};

std::vector<piko::ColorKey> anim_colorKeys = {
    {{255,255,255,0},0.0f},
    {{255,255,255,255},1.0f}
};

assets.addAnimationClip(
    "my_animation",
    anim_sprKeys,
    anim_tranKeys,
    anim_colorKeys);
```

> **Note:** Custom render shaders are currently experimental and may interfere with the built-in batching renderer.

---

# Scene Serialization

```cpp
scenes.saveSceneToFile(
    "scene.json",
    "scene_name");

scenes.loadSceneFromFile(
    "scene.json");
```

---

# Asset Serialization

```cpp
assets.saveAssetsRefToFile(
    "assets.json");

assets.loadAssetsFromRefFile(
    "assets.json");
```

---

# Input System

Bind actions:

```cpp
inputs.bindKey("player_move_up", piko::KEYS::UP);
inputs.bindMouseBtn("player_aim", piko::MOUSE::RIGHT);

inputs.bindKey(
    "combo_action",
    {
        piko::KEYS::LEFT_SHIFT,
        piko::KEYS::SPACE
    });
```

Query actions:

```cpp
bool moving =
    inputs.isActionPressed("player_move_up");

bool aiming =
    inputs.isActionDown("player_aim");
```

Hardware polling is also supported:

```cpp
inputs.isKeyDown(piko::KEYS::W);
inputs.isMousePressed(piko::MOUSE::LEFT);
inputs.getMousePos();
```

---

# Event System

Listen for events:

```cpp
scene->listenToEvent<ButtonEvent>(
[](const ButtonEvent& event)
{
    // ...
});

scene->listenToEvent<CollisionEvent>(
[](const CollisionEvent& event)
{
    // ...
});
```

Publish events:

```cpp
scene->publishEvent<ButtonEvent>({
    this,
    ButtonEvent::STATE::CLICK
});
```

---

# Roadmap

Planned additions include:

- Particle system
- 2D lighting
- Additional built-in gameplay scripts
- More configurable engine subsystems
- Rendering optimizations
- Physics improvements
- Additional performance tuning

---

# Philosophy

pikoBox is designed around one simple idea:

> **Build games quickly without sacrificing flexibility.**

The engine favors composition and data-driven workflows and APIs that remain approachable while still providing direct access to every subsystem for all your game development needs.
