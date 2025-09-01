#pragma once
#include <string>
#include <unordered_map>
#include <glad/glad.h>

// Basit asset y�neticisi: �nbellekli Texture + Shader (Program)
namespace Assets {

    void Init(const std::string& baseDir = "assets"); // "assets" klas�r�
    void Shutdown();

    // ---- TEXTURE ----
    GLuint GetTexture(const std::string& relativePath);          // �rn: "texture.png"
    GLuint ReloadTexture(const std::string& relativePath);       // yeniden y�kle (iste�e ba�l�)

    // ---- SHADER PROGRAM ----
    // Ayn� (vs,fs) ikilisi i�in tek program �retir ve �nbelle�e al�r.
    GLuint GetShaderFromFiles(const std::string& vsRelative,
        const std::string& fsRelative);
    GLuint ReloadShaderFromFiles(const std::string& vsRelative,
        const std::string& fsRelative);

} // namespace Assets
