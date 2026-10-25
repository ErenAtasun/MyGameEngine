#pragma once
#include <string>

namespace mme {
    enum class SoundGroup { SFX, Music };
}

// Ses (miniaudio). Yollar assets klasörüne görelidir; WAV, MP3, FLAC desteklenir.
// Ses cihazı yoksa tüm çağrılar sessizce hiçbir şey yapmaz.
namespace mme::Audio {

    // Application çağırır. headless = true: cihaz açmadan (testler için)
    bool Init(bool headless = false);
    void Shutdown();
    void Update();                     // biten sesleri temizler (her frame)
    bool IsAvailable();

    // Tek seferlik efekt. pitch 1 = normal, 2 = bir oktav tiz
    void Play(const std::string& relativePath, float volume = 1.0f, float pitch = 1.0f);

    // Akışlı müzik (aynı anda tek parça)
    void PlayMusic(const std::string& relativePath, bool loop = true, float volume = 1.0f);
    void StopMusic();
    bool IsMusicPlaying();

    void SetMasterVolume(float volume);
    float GetMasterVolume();
    void SetGroupVolume(SoundGroup group, float volume);
    float GetGroupVolume(SoundGroup group);

    int GetActiveVoiceCount();        // çalan efekt sayısı (debug/test)
}
