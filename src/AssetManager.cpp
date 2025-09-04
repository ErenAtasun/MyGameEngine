#include "AssetManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

namespace fs = std::filesystem;

namespace {
    std::string gBase;
    std::unordered_map<std::string, GLuint> gTextures; // key: "texture.png"
    std::unordered_map<std::string, GLuint> gPrograms; // key: "vs|fs"
    GLuint gWhiteTexture = 0;                          // <- 1x1 beyaz doku cache

    std::string ReadFile(const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        if (!f) { std::cerr << "[Assets] Dosya acilamadi: " << p.string() << "\n"; return {}; }
        std::ostringstream ss; ss << f.rdbuf(); return ss.str();
    }
    GLuint Compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type); glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048]; glGetShaderInfoLog(s, 2048, nullptr, log);
            std::cerr << "[Assets] Shader derleme hatasi:\n" << log << "\n";
        }
        return s;
    }
    GLuint Link(GLuint vs, GLuint fs) {
        GLuint p = glCreateProgram(); glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
        GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048]; glGetProgramInfoLog(p, 2048, nullptr, log);
            std::cerr << "[Assets] Program link hatasi:\n" << log << "\n";
        }
        glDeleteShader(vs); glDeleteShader(fs);
        return p;
    }
    std::string KeyShader(const std::string& vs, const std::string& fs) {
        return vs + "|" + fs;
    }
}

namespace Assets {

    void Init(const std::string& baseDir) {
        gBase = baseDir;
        stbi_set_flip_vertically_on_load(true);
    }

    void Shutdown() {
        for (auto& [k, tex] : gTextures) if (tex) glDeleteTextures(1, &tex);
        for (auto& [k, prg] : gPrograms) if (prg) glDeleteProgram(prg);
        gTextures.clear(); gPrograms.clear();

        // <- beyaz dokuyu da býrak
        if (gWhiteTexture) { glDeleteTextures(1, &gWhiteTexture); gWhiteTexture = 0; }
    }

    // ------------------ Texture ------------------
    GLuint GetTexture(const std::string& relativePath) {
        auto it = gTextures.find(relativePath);
        if (it != gTextures.end()) return it->second;

        fs::path p = fs::path(gBase) / relativePath;
        int w, h, n; unsigned char* data = stbi_load(p.string().c_str(), &w, &h, &n, 4);
        if (!data) { std::cerr << "[Assets] Texture yuklenemedi: " << p << ", sebep: " << stbi_failure_reason() << "\n"; return 0; }

        GLuint t; glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        stbi_image_free(data);

        gTextures[relativePath] = t;
        return t;
    }

    GLuint ReloadTexture(const std::string& relativePath) {
        auto it = gTextures.find(relativePath);
        if (it != gTextures.end()) {
            GLuint old = it->second;
            if (old) glDeleteTextures(1, &old);
            gTextures.erase(it);
        }
        return GetTexture(relativePath);
    }

    // 1x1 beyaz doku
    GLuint GetWhiteTexture() {
        if (gWhiteTexture) return gWhiteTexture;

        const unsigned int WHITE = 0xFFFFFFFFu; // RGBA8
        glGenTextures(1, &gWhiteTexture);
        glBindTexture(GL_TEXTURE_2D, gWhiteTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &WHITE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return gWhiteTexture;
    }

    // ------------------ Shader Program ------------------
    GLuint GetShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative) {
        std::string key = KeyShader(vsRelative, fsRelative);
        auto it = gPrograms.find(key);
        if (it != gPrograms.end()) return it->second;

        auto vsPath = fs::path(gBase) / vsRelative;
        auto fsPath = fs::path(gBase) / fsRelative;
        std::string vsrc = ReadFile(vsPath);
        std::string fsrc = ReadFile(fsPath);
        if (vsrc.empty() || fsrc.empty()) return 0;

        GLuint vs = Compile(GL_VERTEX_SHADER, vsrc.c_str());
        GLuint fs = Compile(GL_FRAGMENT_SHADER, fsrc.c_str());
        GLuint pr = Link(vs, fs);

        glUseProgram(pr);
        GLint loc = glGetUniformLocation(pr, "uTextures");
        if (loc >= 0) {
            int samplers[8] = { 0,1,2,3,4,5,6,7 };
            glUniform1iv(loc, 8, samplers);
        }

        gPrograms[key] = pr;
        return pr;
    }

    GLuint ReloadShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative) {
        std::string key = KeyShader(vsRelative, fsRelative);
        auto it = gPrograms.find(key);
        if (it != gPrograms.end()) {
            GLuint old = it->second;
            if (old) glDeleteProgram(old);
            gPrograms.erase(it);
        }
        return GetShaderFromFiles(vsRelative, fsRelative);
    }

} // namespace Assets