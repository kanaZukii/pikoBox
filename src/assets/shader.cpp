#include "piko/shader.hpp"
#include "piko/logger.hpp"

#include "raylib.h"

#include <filesystem>

using namespace piko;

RenderShader::RenderShader(const std::string& vertexCode, const std::string& fragCode, const std::string& name){
    program = new Shader;
    
    // Compile directly from memory strings
    *program = LoadShaderFromMemory(vertexCode.c_str(), fragCode.c_str());

    if (program->id == 0) {
        UnloadShader(*program);
        delete program;
        program = nullptr;
        throw std::runtime_error("SHADER: Failed to compile GLSL shader program from memory strings.");
    }

    this->name = name;

    // Extract core uniform locations safely
    locs["uProjection"] = GetShaderLocation(*program, "uProjection");
    locs["uView"]       = GetShaderLocation(*program, "uView");
    locs["uTextures"]   = GetShaderLocation(*program, "uTextures");
}

bool RenderShader::operator==(const RenderShader& other) const {
    return (this->name == other.name 
            && this->program->id == other.program->id
            && this->verPath == other.verPath
            && this->fragPath == other.fragPath);
}

RenderShader::~RenderShader(){
    if(program){
        if(program->id > 0){
            UnloadShader(*program);
        }

        delete program;
        program = nullptr;
    }
}

RenderShader::RenderShader(RenderShader&& other) noexcept {
    this->program = other.program;
    this->name = std::move(other.name);
    this->locs = std::move(other.locs);
    this->verPath = std::move(other.verPath);
    this->fragPath = std::move(other.fragPath);

    other.program = nullptr; 
}

RenderShader& RenderShader::operator=(RenderShader&& other) noexcept {
    if (this != &other) {
        delete this->program;

        this->program = other.program;
        this->name = std::move(other.name);
        this->locs = std::move(other.locs);
        this->verPath = std::move(other.verPath);
        this->fragPath = std::move(other.fragPath);

        other.program = nullptr;
    }
    return *this;
}

int RenderShader::getLocationID(const std::string& varName) const {
    auto loc_exist = locs.find(varName);
    if (loc_exist != locs.end()) {
        if(loc_exist->second == -1){
            PBOX_WARN("RENDER_SHADER: Variable Location '%s' not found or optimized out.", varName.c_str());
        }
        return loc_exist->second;
    }

    int location = GetShaderLocation(*program, varName.c_str());
    
    // Store it (even if it's -1, so we don't keep checking missing variables)
    locs[varName] = location;

    if (location == -1) {
        PBOX_WARN("RENDER_SHADER: Variable Location '%s' not found or optimized out.", varName.c_str());
    }

    return location; 
}

void RenderShader::begin() const{
    BeginShaderMode(*program);
}
void RenderShader::end() const{
    EndShaderMode();
}