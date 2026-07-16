// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <string>
#include <unordered_map>

// Forward declare raylib's Shader struct.
// Allow us to avoid including the entirity of raylib in our headers.
struct Shader;

namespace piko {
    /*
        RenderShader asset. Handles the lifecycle of a GLSL shader.
    */
    class RenderShader{
        public:
            // Creates a shader program from raw source code strings.
            RenderShader(const std::string& name, const std::string& vertexCode, const std::string& fragCode);

            // Releases shader resources from the GPU.
            ~RenderShader();

            // Shaders are non-copyable to prevent redundant GPU program allocation.
            RenderShader(const RenderShader&) = delete;
            RenderShader& operator=(const RenderShader&) = delete;

            // RenderShader proper move semantics.
            RenderShader(RenderShader&& other) noexcept;
            RenderShader& operator=(RenderShader&& other) noexcept;

            // Comparison based on shader identity.
            bool operator==(const RenderShader& other) const;

            // Activates this shader for subsequent draw calls.
            void begin() const;

            // Deactivates this shader and reverts to the default.
            void end() const;

            // Retrieves the location ID for a uniform variable. Uses a cache to optimize lookup.
            int getLocationID(const std::string& varName) const;

            // Returns the underlying raylib Shader struct.
            inline const Shader& getProgram() const noexcept {return *program;}

            // Returns the unique identifier for this RenderShader asset.
            inline const std::string getName() const noexcept { return name; }

            // Metadata for serialization.
            std::string verPath = "";
            std::string fragPath = "";
            
        private:
            Shader* program = nullptr;
            std::string name = "";
            mutable std::unordered_map<std::string, int> locs; // Cache for uniform locations.
    };

    // pikoBox's default vertex shader for desktop.
    inline const char* DEFAULT_VERTEX_SHADER = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
layout(location=2) in vec2 aTexCoords;
layout(location=3) in float aTexID;

uniform mat4 uProjection;
uniform mat4 uView;

out vec4 fColor;
out vec2 fTexCoords;
flat out int fTexID;

void main() {
    fColor = aColor;
    fTexCoords = aTexCoords;
    fTexID = int(aTexID);
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
)";

    // pikoBox's default fragment shader for desktop.
    inline const char* DEFAULT_FRAGMENT_SHADER = R"(#version 330 core
in vec4 fColor;
in vec2 fTexCoords;
flat in int fTexID;

uniform sampler2D uTextures[8];

out vec4 FragColor;

void main() {
    vec4 texColor = vec4(1.0);
    switch (fTexID) {
        case 0: texColor = vec4(1.0); break;
        case 1: texColor = texture(uTextures[1], fTexCoords); break;
        case 2: texColor = texture(uTextures[2], fTexCoords); break;
        case 3: texColor = texture(uTextures[3], fTexCoords); break;
        case 4: texColor = texture(uTextures[4], fTexCoords); break;
        case 5: texColor = texture(uTextures[5], fTexCoords); break;
        case 6: texColor = texture(uTextures[6], fTexCoords); break;
        case 7: texColor = texture(uTextures[7], fTexCoords); break;
    }
    FragColor = fColor * texColor;
}
)";

    // pikoBox's default vertex shader for WEBGL.
    inline const char* DEFAULT_WEBGL_VERTEX_SHADER = R"(#version 300 es
precision mediump float;
precision mediump int;

layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
layout(location=2) in vec2 aTexCoords;
layout(location=3) in float aTexID;

uniform mat4 uProjection;
uniform mat4 uView;

out vec4 fColor;
out vec2 fTexCoords;
flat out int fTexID;

void main() {
    fColor = aColor;
    fTexCoords = aTexCoords;
    fTexID = int(aTexID);
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
)";

    // pikoBox's default fragment shader for WEBGL.
    inline const char* DEFAULT_WEBGL_FRAGMENT_SHADER = R"(#version 300 es
precision mediump float;
precision mediump int;

in vec4 fColor;
in vec2 fTexCoords;
flat in int fTexID;

uniform sampler2D uTextures[8];

out vec4 FragColor;

void main() {
    vec4 texColor = vec4(1.0);

    if (fTexID == 1)      texColor = texture(uTextures[1], fTexCoords);
    else if (fTexID == 2) texColor = texture(uTextures[2], fTexCoords);
    else if (fTexID == 3) texColor = texture(uTextures[3], fTexCoords);
    else if (fTexID == 4) texColor = texture(uTextures[4], fTexCoords);
    else if (fTexID == 5) texColor = texture(uTextures[5], fTexCoords);
    else if (fTexID == 6) texColor = texture(uTextures[6], fTexCoords);
    else if (fTexID == 7) texColor = texture(uTextures[7], fTexCoords);

    FragColor = fColor * texColor;
}
)";
}
