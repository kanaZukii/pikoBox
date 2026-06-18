#pragma once

#include <string>
#include <unordered_map>

struct Shader;

namespace piko {
    class RenderShader{
        public:
            RenderShader(const std::string& vertexCode, const std::string& fragCode, const std::string& name);
            ~RenderShader();

            RenderShader(const RenderShader&) = delete;
            RenderShader& operator=(const RenderShader&) = delete;

            RenderShader(RenderShader&& other) noexcept;
            RenderShader& operator=(RenderShader&& other) noexcept;

            bool operator==(const RenderShader& other) const;

            void begin() const;
            void end() const;

            int getLocationID(const std::string& varName);
            inline const Shader& getProgram() const noexcept {return *program;}

            inline const std::string getName() const noexcept { return name; }

            std::string verPath = "";
            std::string fragPath = "";
            
        private:
            Shader* program = nullptr;
            std::string name = "";
            std::unordered_map<std::string, int> locs;
    };


    inline const char* DEFAULT_VERTEX_SHADER = R"(
#version 330 core

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

    inline const char* DEFAULT_FRAGMENT_SHADER = R"(
#version 330 core

in vec4 fColor;
in vec2 fTexCoords;
flat in int fTexID;

uniform sampler2D uTextures[8]; // 8 texture slots per batch

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
}
