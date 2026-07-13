// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include "piko/math.hpp"

struct Camera2D;

namespace piko {
    class Cam{
        public: 
            Cam();
            ~Cam();
            void begin();
            void end();

            void setPosition(Vect2 position);

            void setPosX(float x);
            void setPosY(float y);

            void setOffset(Vect2 offset);
            void setZoom(float zoom);
            void rotation(float rotation);

            Vect2 getPosition() const;
            float getZoom() const;

            Vect2 screenToWorld(Vect2 screenPos) const;
            Vect2 worldToScreen(Vect2 worldPos) const;

            Mat4 getProj() const;
            Mat4 getView() const;

            Rect getViewSpaceBubble() const;

        private:
            Camera2D* camera = nullptr;
    };
}

