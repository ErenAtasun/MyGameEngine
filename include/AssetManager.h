#pragma once
#include <string>
#include <unordered_map>
#include <glad/glad.h>

namespace Assets {
    void   Init(const std::string& baseDir);
    void   Shutdown();

    // Textures
    GLuint GetTexture(const std::string& relativePath);
    GLuint ReloadTexture(const std::string& relativePath);
    GLuint GetWhiteTexture();                // <- BEYAZ DOKU

    // Shaders
    GLuint GetShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative);
    GLuint ReloadShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative);
}