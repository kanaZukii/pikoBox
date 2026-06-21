#pragma once

struct RenderTexture;
typedef RenderTexture RenderTexture2D;

class Global {
public:
    struct Variables {
        int windowWidth = 1280;
        int windowHeight = 720;

        int canvasWidth = 1280;
        int canvasHeight = 720;

        RenderTexture2D* drawLayer0 = nullptr;

        float fontSize = 20.0f;
        float volume = 50.0f;
        const char* windowTitle = "PikoBox Game";

        bool fullscreen = false;
    };

    static Variables& GetVar() {
        static Variables instance; 
        return instance;
    }

    Global(const Global&) = delete;
    Global& operator=(const Global&) = delete;

private:
    Global(){}
};
