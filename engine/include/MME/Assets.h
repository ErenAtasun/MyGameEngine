#pragma once
#include <cstdint>
#include <string>

namespace mme {

    // GPU doku tanıtıcısı. 0 = doku yok (Renderer2D bunu düz beyaz olarak çizer).
    using TextureID = uint32_t;

    enum class TextureFilter {
        Linear,   // yumuşak ölçekleme (fotoğraf, büyük görseller)
        Nearest,  // keskin pikseller (pixel art, tilemap, bitmap font)
    };

    struct TextureInfo {
        int width = 0;
        int height = 0;
        TextureFilter filter = TextureFilter::Linear;
    };
}

// Dosya tabanlı varlık önbelleği. Yollar Init'e verilen klasöre görelidir.
namespace mme::Assets {

    void Init(const std::string& baseDir);
    void Shutdown();

    // "maps/level1.csv" -> "<assets>/maps/level1.csv"
    std::string GetPath(const std::string& relativePath);
    const std::string& GetBaseDir();

    // Textures (ilk yükleme hangi filtreyle yapıldıysa önbellekte o kalır)
    TextureID GetTexture(const std::string& relativePath, TextureFilter filter = TextureFilter::Linear);
    TextureID ReloadTexture(const std::string& relativePath);
    TextureInfo GetTextureInfo(TextureID texture);
    // Dosyadan yüklenmiş bir dokunun göreli yolu; bellekten oluşturulduysa boş
    std::string GetTexturePath(TextureID texture);

    // Bellekten doku (RGBA8, satır 0 = görüntünün üstü). Sahibi çağırandır.
    // Tüm dokularda kural aynı: v=0 görüntünün altı, v=1 üstü.
    TextureID CreateTexture(int width, int height, const void* rgbaPixels, TextureFilter filter, bool mipmaps = false);
    void DestroyTexture(TextureID texture);

    // Hot-reload: diskte değişen dokuları aynı TextureID ile yeniden yükler.
    // Yeniden yüklenen doku sayısını döner. (Application periyodik çağırır.)
    int CheckForChanges();

    // Shader programı (motor içi kullanım). Hata olursa 0 döner.
    uint32_t CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc, const std::string& debugName);
    uint32_t GetShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative);
    uint32_t ReloadShaderFromFiles(const std::string& vsRelative, const std::string& fsRelative);
}
