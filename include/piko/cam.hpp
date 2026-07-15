// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include "piko/math.hpp"

// Forward declare raylib's Camera2D.
// Allow us to avoid including the entirity of raylib in our headers.
struct Camera2D;

namespace piko {

    /*
        Cam, 2D Camera controller. Manages view/projection matrices 
        and provides utility methods for screen-to-world coordinate mapping.
    */
    class Cam{
        public: 
            Cam();
            ~Cam();

            // Wraps underlying raylib's camera lifecycle methods
            void begin();
            void end();

            // Updates the camera's focus position.
            void setPosition(Vect2 position);
            void setPosX(float x);
            void setPosY(float y);

            // Sets the camera's anchor point (e.g., center of the screen).
            void setOffset(Vect2 offset);

            // Adjusts the camera scale. 1.0f is default.
            void setZoom(float zoom);

            // Sets camera rotation in degrees.
            void rotation(float rotation);

            Vect2 getPosition() const;
            float getZoom() const;

            // Converts screen coordinates (mouse) to world coordinates.
            Vect2 screenToWorld(Vect2 screenPos) const;
            // Converts world coordinates to screen space coordinates.
            Vect2 worldToScreen(Vect2 worldPos) const;

            // Returns the projection matrix (orthographic).
            Mat4 getProj() const;
            // Returns the current view matrix based on camera state.
            Mat4 getView() const;

            // Returns what the camera can see in world coordinates.
            Rect getViewSpaceBubble() const;

        private:
            Camera2D* camera = nullptr;
    };
}

