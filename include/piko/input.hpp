#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "piko/math.hpp"

namespace piko {
    class Engine;
    
    namespace KEYS {
        constexpr int SPACE = 32;
        constexpr int MINUS = 45;
        constexpr int PERIOD = 46;
        constexpr int SLASH = 47;
        constexpr int ZERO = 48;
        constexpr int ONE = 49;
        constexpr int TWO = 50;
        constexpr int THREE = 51;
        constexpr int FOUR = 52;
        constexpr int FIVE = 53;
        constexpr int SIX = 54;
        constexpr int SEVEN = 55;
        constexpr int EIGHT = 56;
        constexpr int NINE = 57;
        constexpr int SEMICOLON = 59;
        constexpr int EQUAL = 61;
        constexpr int A = 65;
        constexpr int B = 66;
        constexpr int C = 67;
        constexpr int D = 68;
        constexpr int E = 69;
        constexpr int F = 70;
        constexpr int G = 71;
        constexpr int H = 72;
        constexpr int I = 73;
        constexpr int J = 74;
        constexpr int K = 75;
        constexpr int L = 76;
        constexpr int M = 77;
        constexpr int N = 78;
        constexpr int O = 79;
        constexpr int P = 80;
        constexpr int Q = 81;
        constexpr int R = 82;
        constexpr int S = 83;
        constexpr int T = 84;
        constexpr int U = 85;
        constexpr int V = 86;
        constexpr int W = 87;
        constexpr int X = 88;
        constexpr int Y = 89;
        constexpr int Z = 90;

        constexpr int F1 = 290;
        constexpr int F2 = 291;
        constexpr int F3 = 292;
        constexpr int F4  = 293;
        constexpr int F5 = 294;
        constexpr int F6 = 295;
        constexpr int F7 = 296;
        constexpr int F8 = 297;
        constexpr int F9 = 298;
        constexpr int F10 = 299;
        constexpr int F11 = 300;
        constexpr int F12 = 301;
        constexpr int LEFT_SHIFT = 340;
        constexpr int LEFT_CONTROL = 341;
        constexpr int LEFT_ALT = 342;
        constexpr int RIGHT_SHIFT = 344;
        constexpr int RIGHT_CONTROL = 345;
        constexpr int RIGHT_ALT = 346;
        constexpr int ENTER = 257;
        constexpr int TAB = 258;
        constexpr int LEFT  = 263;
        constexpr int RIGHT = 262;
        constexpr int UP    = 265;
        constexpr int DOWN  = 264;
    }

    namespace MOUSE {
        constexpr int LEFT   = 0;
        constexpr int RIGHT  = 1;
        constexpr int MIDDLE = 2;
    }

    struct MouseState {
        float x = 0.0f;
        float y = 0.0f;
        float deltaX = 0.0f;
        float deltaY = 0.0f;
    };

    class InputManager {
        public:
            ~InputManager() = default;
            InputManager(const InputManager&) = delete;
            InputManager& operator=(const InputManager&) = delete;

            // Configuration API
            // Bind a single chord/combo to an action (appends to existing configurations)
            void bindKey(const std::string& action, const std::vector<int>& rawKeyCodes);
            void bindMouseBtn(const std::string& action, const std::vector<int>& rawButtonCodes);

            // Quick helper override for a single key binding (no vector wrapper needed)
            void bindKey(const std::string& action, int singleKeyCode) {
                bindKey(action, std::vector<int>{ singleKeyCode });
            }

            void bindMouseBtn(const std::string& action, int singleKeyCode) {
                bindMouseBtn(action, std::vector<int>{singleKeyCode});
            }

            bool unbindKeyAct(const std::string& action);
            bool unbindMouseAct(const std::string& action);

            void update();

            bool isActionDown(const std::string& action) const;
            bool isActionPressed(const std::string& action) const;

            bool isMouseDown(const int& btn);
            bool isMousePressed(const int& btn);

            Vect2 getMousePos() const { return {mouse.x, mouse.y};}
            Vect2 getMouseDelta() const { return {mouse.deltaX, mouse.deltaY};}

            float getMouseX() const { return mouse.x; }
            float getMouseY() const { return mouse.y; }
            
            float getMouseDeltaX() const { return mouse.deltaX; }
            float getMouseDeltaY() const { return mouse.deltaY; }

        private:
            InputManager();
            // Action -> List of Combos -> List of Keys in that combo
            std::unordered_map<std::string, std::vector<std::vector<int>>> keyboardBindings;
            std::unordered_map<std::string, std::vector<std::vector<int>>> mouseBindings;

            MouseState mouse;

            friend class Engine;
    };

}