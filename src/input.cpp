#include "piko/input.hpp"
#include "piko/logger.hpp"
#include "global.hpp"

#include "raylib.h"

using namespace piko;

InputManager::InputManager(){
    PBOX_INFO("INPUT_MAN: Input Manager Initialized.");
}

void InputManager::bindKey(const std::string& action, const std::vector<int>& rawKeyCodes) {
    // Appends this combination as an alternative configuration path
    keyboardBindings[action].push_back(rawKeyCodes);
}

void InputManager::bindMouseBtn(const std::string& action, const std::vector<int>& rawButtonCodes) {
    mouseBindings[action].push_back(rawButtonCodes);
}

bool InputManager::unbindKeyAct(const std::string& action){
    auto it = keyboardBindings.find(action);
    if(it == keyboardBindings.end()) return false;
    keyboardBindings.erase(it);
    return true;
}

bool InputManager::unbindMouseAct(const std::string& action){
    auto it = mouseBindings.find(action);
    if(it == mouseBindings.end()) return false;
    mouseBindings.erase(it);
    return true;
}

// Helper: Returns true ONLY if every single key in this specific combo vector is held down
bool isSubComboDown(const std::vector<int>& keys) {
    if (keys.empty()) return false;
    for (int key : keys) {
        if (!IsKeyDown(static_cast<KeyboardKey>(key))) return false;
    }
    return true;
}

bool InputManager::isActionDown(const std::string& action) const {
    // 1. Evaluate Keyboard Alternatives
    auto kIt = keyboardBindings.find(action);
    if (kIt != keyboardBindings.end()) {
        // Loop through each alternative combination setup
        for (const auto& combo : kIt->second) {
            if (isSubComboDown(combo)) return true; // Found a valid active path!
        }
    }

    // 2. Evaluate Mouse Alternatives
    auto mIt = mouseBindings.find(action);
    if (mIt != mouseBindings.end()) {
        for (const auto& combo : mIt->second) {
            bool allButtonsDown = true;
            for (int btn : combo) {
                if (!IsMouseButtonDown(static_cast<MouseButton>(btn))) {
                    allButtonsDown = false;
                    break;
                }
            }
            if (!combo.empty() && allButtonsDown) return true;
        }
    }
    return false;
}

bool InputManager::isMouseDown(const int& btn){
    return IsMouseButtonDown(static_cast<MouseButton>(btn));
}

bool InputManager::isMousePressed(const int& btn){
    return IsMouseButtonPressed(static_cast<MouseButton>(btn));
}

bool InputManager::isActionPressed(const std::string& action) const {
    // Evaluate Keyboard Press
    auto kIt = keyboardBindings.find(action);
    if (kIt != keyboardBindings.end()) {
        for (const auto& combo : kIt->second) {
            if (isSubComboDown(combo)) {
                // Make sure at least one key in the combo was newly tapped on this exact frame
                for (int key : combo) {
                    if (IsKeyPressed(static_cast<KeyboardKey>(key))) return true;
                }
            }
        }
    }

    // Evaluate Mouse Press
    auto mIt = mouseBindings.find(action);
    if (mIt != mouseBindings.end()) {
        for (const auto& combo : mIt->second) {
            bool allDown = true;
            for (int btn : combo) {
                if (!IsMouseButtonDown(static_cast<MouseButton>(btn))) {
                    allDown = false;
                    break;
                }
            }
            if (allDown && !combo.empty()) {
                for (int btn : combo) {
                    if (IsMouseButtonPressed(static_cast<MouseButton>(btn))) return true;
                }
            }
        }
    }
    return false;
}

void InputManager::update() {
    Vector2 rawPos = GetMousePosition();
    Vector2 delta = GetMouseDelta();

    // 1. Fetch physical OS window metrics
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // 2. Fetch your internal design canvas metrics (1920 x 1080)
    float canvasW = (float)Global::GetVar().canvasWidth;
    float canvasH = (float)Global::GetVar().canvasHeight;

    // 3. Map window space coordinates cleanly into canvas asset space!
    if (screenW > 0 && screenH > 0) {
        mouse.x = rawPos.x * (canvasW / screenW);
        mouse.y = rawPos.y * (canvasH / screenH);
        
        // Scale deltas as well to keep mouse gestures accurate
        mouse.deltaX = delta.x * (canvasW / screenW);
        mouse.deltaY = delta.y * (canvasH / screenH);
    } else {
        mouse.x = rawPos.x;
        mouse.y = rawPos.y;
        mouse.deltaX = delta.x;
        mouse.deltaY = delta.y;
    }
}