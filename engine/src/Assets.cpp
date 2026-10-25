#include <MME/Assets.h>
#include <MME/Log.h>

#include <glad/glad.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static_assert(sizeof(mme::TextureID) == sizeof(GLuint), "TextureID GLuint ile ayni boyutta olmali");

namespace fs = std::filesystem;

namespace mme::Assets {

    namespace {
        struct FileTexture {
            GLuint id = 0;
            fs::file_time_type writeTime{};
        };

        std::string gBase;
        std::string gProject;
        std::unordered_map<std::string, FileTexture> gTextures; // key: "texture.png"
        std::unordered_map<GLuint, TextureInfo> gTextureInfo;   // tüm canlı dokular
        std::unordered_map<std::string, GLuint> gPrograms;      // key: "vs|fs"

        std::string ReadFile(const fs::path& p) {
            std::ifstream f(p, std::ios::binary);
            if (!f) { MME_ERROR("[Assets] Dosya acilamadi: ", p.string()); return {}; }
            std::ostringstream ss; ss << f.rdbuf(); return ss.str();
        }

        fs::file_time_type WriteTime(const fs::path& p) {
            std::error_code ec;
            const auto t = fs::last_write_time(p, ec);
            return ec ? fs::file_time_type{} : t;
        }

        GLuint Compile(GLenum type, const char* src, const std::string& debugName) {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[2048]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
                MME_ERROR("[Assets] Shader derleme hatasi (", debugName, "):\n", log);
                glDeleteShader(s);
                return 0;
            }
            return s;
        }

        GLuint Link(GLuint vs, GLuint fs, const std::string& debugName) {
            GLuint p = glCreateProgram();
            glAttachShader(p, vs); glAttachShader(p, fs);
            glLinkProgram(p);
            glDeleteShader(vs); glDeleteShader(fs);
            GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[2048]; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
                MME_ERROR("[Assets] Program link hatasi (", debugName, "):\n", log);
                glDeleteProgram(p);
                return 0;
            }
            return p;
        }

        std::string KeyShader(const std::string& vs, const std::string& fs) {
            return vs + "|" + fs;
        }

        // existing != 0 ise aynı GL nesnesinin içeriği değiştirilir (hot-reload)
        GLuint Upload(GLuint existing, int w, int h, const void* rgba, TextureFilter filter, bool mipmaps) {
            GLuint t = existing;
            if (!t) glGenTextures(1, &t);
            glBindTexture(GL_TEXTURE_2D, t);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

            const bool nearest = (filter == TextureFilter::Nearest);
            GLint minFilter = nearest ? GL_NEAREST : GL_LINEAR;
            if (mipmaps) {
                glGenerateMipmap(GL_TEXTURE_2D);
                minFilter = nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            gTextureInfo[t] = TextureInfo{ w, h, filter };
            return t;
        }

        // Diskten okuyup yükler; existing verilirse onun içine. Başarısızsa 0.
        GLuint LoadFromDisk(const std::string& relativePath, GLuint existing, TextureFilter filter) {
            const fs::path p = GetPath(relativePath);
            int w = 0, h = 0, n = 0;
            unsigned char* data = stbi_load(p.string().c_str(), &w, &h, &n, 4);
            if (!data) {
                MME_ERROR("[Assets] Texture yuklenemedi: ", p.string(), " (", stbi_failure_reason(), ")");
                return 0;
            }
            const GLuint t = Upload(existing, w, h, data, filter, true);
            stbi_image_free(data);
            return t;
        }
    }

    void Init(const std::string& baseDir) {
        gBase = baseDir;
        // OpenGL'de t=0 dokunun altıdır; dünya uzayında (+y yukarı) görseller düz görünsün.
        stbi_set_flip_vertically_on_load(true);
    }

    void Shutdown() {
        for (auto& [k, tex] : gTextures) if (tex.id) { glDeleteTextures(1, &tex.id); gTextureInfo.erase(tex.id); }
        for (auto& [k, prg] : gPrograms) if (prg) glDeleteProgram(prg);
        gTextures.clear(); gPrograms.clear();
        if (!gTextureInfo.empty())
            MME_WARN("[Assets] Kapanista serbest birakilmamis ", gTextureInfo.size(), " doku var");
        gTextureInfo.clear();
    }

    std::string GetPath(const std::string& relativePath) {
        if (gProject.empty()) return (fs::path(gBase) / relativePath).string();
        const fs::path inProject = fs::path(gProject) / relativePath;
        std::error_code ec;
        if (fs::exists(inProject, ec)) return inProject.string();
        const fs::path inEngine = fs::path(gBase) / relativePath;
        if (fs::exists(inEngine, ec)) return inEngine.string();
        return inProject.string(); // henüz yok: yeni dosya projeye yazılır
    }

    const std::string& GetBaseDir() { return gBase; }

    void SetProjectDir(const std::string& projectDir) {
        gProject = projectDir;
        if (!gProject.empty()) MME_INFO("[Assets] Proje: ", fs::absolute(gProject).string());
    }

    const std::string& GetProjectDir() { return gProject; }

    // ------------------ Texture ------------------
    TextureID GetTexture(const std::string& relativePath, TextureFilter filter) {
        auto it = gTextures.find(relativePath);
        if (it != gTextures.end()) return it->second.id;

        const GLuint t = LoadFromDisk(relativePath, 0, filter);
        if (!t) return 0;
        gTextures[relativePath] = FileTexture{ t, WriteTime(GetPath(relativePath)) };
        return t;
    }

    TextureID ReloadTexture(const std::string& relativePath) {
        auto it = gTextures.find(relativePath);
        if (it == gTextures.end()) return GetTexture(relativePath);

        FileTexture& tex = it->second;
        const TextureFilter filter = gTextureInfo[tex.id].filter;
        if (LoadFromDisk(relativePath, tex.id, filter))
            tex.writeTime = WriteTime(GetPath(relativePath));
        return tex.id;
    }

    TextureInfo GetTextureInfo(TextureID texture) {
        auto it = gTextureInfo.find(texture);
        return it != gTextureInfo.end() ? it->second : TextureInfo{};
    }

    void SetTextureFilter(TextureID texture, TextureFilter filter) {
        auto it = gTextureInfo.find(texture);
        if (it == gTextureInfo.end() || it->second.filter == filter) return;
        it->second.filter = filter;
        const bool nearest = filter == TextureFilter::Nearest;
        glBindTexture(GL_TEXTURE_2D, texture);
        // Dosyadan yüklenenlerin mipmap'i var
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
    }

    std::string GetTexturePath(TextureID texture) {
        if (!texture) return {};
        for (const auto& [path, tex] : gTextures)
            if (tex.id == texture) return path;
        return {};
    }

    TextureID CreateTexture(int width, int height, const void* rgbaPixels, TextureFilter filter, bool mipmaps) {
        // Girdi satır 0 = görüntünün üstü. Diskten yüklenenlerle aynı kural geçerli
        // olsun diye (v=0 görüntünün altı) satırlar ters çevrilerek yüklenir.
        const size_t rowBytes = (size_t)width * 4;
        const auto* src = static_cast<const unsigned char*>(rgbaPixels);
        std::vector<unsigned char> flipped(rowBytes * (size_t)height);
        for (int y = 0; y < height; ++y)
            std::copy_n(src + (size_t)y * rowBytes, rowBytes, flipped.data() + (size_t)(height - 1 - y) * rowBytes);
        return Upload(0, width, height, flipped.data(), filter, mipmaps);
    }

    void DestroyTexture(TextureID texture) {
        if (!texture) return;
        GLuint t = texture;
        glDeleteTextures(1, &t);
        gTextureInfo.erase(t);
    }

    int CheckForChanges() {
        int reloaded = 0;
        for (auto& [path, tex] : gTextures) {
            const auto now = WriteTime(GetPath(path));
            if (now == fs::file_time_type{} || now == tex.writeTime) continue;
            // Dosya yazılırken okunursa yükleme başarısız olur; zaman damgası
            // güncellenmez, sonraki kontrolde tekrar denenir.
            if (LoadFromDisk(path, tex.id, gTextureInfo[tex.id].filter)) {
                tex.writeTime = now;
                ++reloaded;
                MME_INFO("[Assets] Doku yeniden yuklendi: ", path);
            }
        }
        return reloaded;
    }

    // ------------------ Shader Program ------------------
    uint32_t CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc, const std::string& debugName) {
        GLuint vs = Compile(GL_VERTEX_SHADER, vertexSrc, debugName);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, fragmentSrc, debugName);
        if (!vs || !fs) {
            if (vs) glDeleteShader(vs);
            if (fs) glDeleteShader(fs);
            return 0;
        }
        return Link(vs, fs, debugName);
    }

    uint32_t GetShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative) {
        std::string key = KeyShader(vsRelative, fsRelative);
        auto it = gPrograms.find(key);
        if (it != gPrograms.end()) return it->second;

        std::string vsrc = ReadFile(GetPath(vsRelative));
        std::string fsrc = ReadFile(GetPath(fsRelative));
        if (vsrc.empty() || fsrc.empty()) return 0;

        GLuint pr = CreateShaderProgram(vsrc.c_str(), fsrc.c_str(), key);
        if (pr) gPrograms[key] = pr;
        return pr;
    }

    uint32_t ReloadShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative) {
        std::string key = KeyShader(vsRelative, fsRelative);
        auto it = gPrograms.find(key);
        if (it != gPrograms.end()) {
            GLuint old = it->second;
            if (old) glDeleteProgram(old);
            gPrograms.erase(it);
        }
        return GetShaderFromFiles(vsRelative, fsRelative);
    }
}
