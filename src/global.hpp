#pragma once

#include "piko/math.hpp"

struct RenderTexture;
typedef RenderTexture RenderTexture2D;


class Global {
public:
    struct Variables {
        int windowWidth = 1280;
        int windowHeight = 720;

        int canvasWidth = 1280;
        int canvasHeight = 720;
        bool fullscreen = false;

        const char* windowTitle = "PikoBox Game";

        piko::Rect screenDest = {0.0f, 0.0f, (float)windowWidth, (float)windowHeight};

        RenderTexture2D* drawLayer0 = nullptr;
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
