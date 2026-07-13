// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

namespace piko {
    // Contains raw frame delta time and physics' delta time.
    struct DeltaTime {
        float raw = 0.0f;    
        float physics = 0.0f; 
    };
} 
