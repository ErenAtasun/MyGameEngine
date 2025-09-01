#pragma once
#include <string>
#include <unordered_map>
#include <glad/glad.h>

// Basit asset yöneticisi: önbellekli Texture + Shader (Program)
namespace Assets {

    void Init(const std::string& baseDir = "assets"); // "assets" klasörü
    void Shutdown();

    // ---- TEXTURE ----
    GLuint GetTexture(const std::string& relativePath);          // örn: "texture.png"
    GLuint ReloadTexture(const std::string& relativePath);       // yeniden yükle (isteðe baðlý)

    // ---- SHADER PROGRAM ----
    // Ayný (vs,fs) ikilisi için tek program üretir ve önbelleðe alýr.
    GLuint GetShaderFromFiles(const std::string& vsRelative,
        const std::string& fsRelative);
    GLuint ReloadShaderFromFiles(const std::string& vsRelative,
        const std::string& fsRelative);

} // namespace Assets
