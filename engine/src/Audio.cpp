#include <MME/Audio.h>
#include <MME/Assets.h>
#include <MME/Log.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif
#define MA_NO_ENCODING        // yalnızca çalma: kaydetme/kodlama gerekmiyor
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

namespace mme::Audio {

    namespace {
        constexpr size_t kMaxVoices = 32;

        ma_engine s_Engine;
        ma_sound_group s_Groups[2];
        bool s_Ready = false;
        float s_GroupVolume[2] = { 1.0f, 1.0f };

        std::vector<std::unique_ptr<ma_sound>> s_Voices;
        std::unique_ptr<ma_sound> s_Music;
        std::unordered_set<std::string> s_FailedPaths; // aynı hata her vuruşta loglanmasın

        ma_sound_group* Group(SoundGroup g) { return &s_Groups[g == SoundGroup::SFX ? 0 : 1]; }

        void Release(std::unique_ptr<ma_sound>& s) {
            if (!s) return;
            ma_sound_uninit(s.get());
            s.reset();
        }

        bool LoadFailed(const std::string& relativePath, ma_result result) {
            if (s_FailedPaths.insert(relativePath).second)
                MME_ERROR("[Audio] Ses yuklenemedi: ", relativePath, " (", ma_result_description(result), ")");
            return false;
        }
    }

    bool Init(bool headless) {
        if (s_Ready) return true;

        ma_engine_config config = ma_engine_config_init();
        if (headless) {
            config.noDevice = MA_TRUE;
            config.channels = 2;
            config.sampleRate = 48000;
        }
        const ma_result result = ma_engine_init(&config, &s_Engine);
        if (result != MA_SUCCESS) {
            MME_WARN("[Audio] Ses cihazi acilamadi (", ma_result_description(result), "); ses kapali");
            return false;
        }
        ma_sound_group_init(&s_Engine, 0, nullptr, &s_Groups[0]);
        ma_sound_group_init(&s_Engine, 0, nullptr, &s_Groups[1]);
        s_Ready = true;

        if (ma_device* device = ma_engine_get_device(&s_Engine))
            MME_INFO("[Audio] Cihaz: ", device->playback.name, " (", ma_engine_get_sample_rate(&s_Engine), " Hz)");
        else
            MME_INFO("[Audio] Cihazsiz modda baslatildi");
        return true;
    }

    void Shutdown() {
        if (!s_Ready) return;
        for (auto& v : s_Voices) Release(v);
        s_Voices.clear();
        Release(s_Music);
        ma_sound_group_uninit(&s_Groups[0]);
        ma_sound_group_uninit(&s_Groups[1]);
        ma_engine_uninit(&s_Engine);
        s_FailedPaths.clear();
        s_Ready = false;
    }

    bool IsAvailable() { return s_Ready; }

    void Update() {
        if (!s_Ready) return;
        for (auto& v : s_Voices)
            if (v && ma_sound_at_end(v.get())) Release(v);
        s_Voices.erase(std::remove(s_Voices.begin(), s_Voices.end(), nullptr), s_Voices.end());
    }

    void Play(const std::string& relativePath, float volume, float pitch) {
        if (!s_Ready || s_FailedPaths.count(relativePath)) return;

        // Sınır aşılırsa en eski efekt kesilir
        if (s_Voices.size() >= kMaxVoices) {
            Release(s_Voices.front());
            s_Voices.erase(s_Voices.begin());
        }

        auto sound = std::make_unique<ma_sound>();
        const std::string path = Assets::GetPath(relativePath);
        // DECODE: resource manager çözülmüş veriyi yola göre önbellekler;
        // aynı efektin sonraki çalınışları dosyayı tekrar okumaz.
        const ma_result result = ma_sound_init_from_file(&s_Engine, path.c_str(), MA_SOUND_FLAG_DECODE,
                                                         Group(SoundGroup::SFX), nullptr, sound.get());
        if (result != MA_SUCCESS) { LoadFailed(relativePath, result); return; }

        ma_sound_set_volume(sound.get(), volume);
        ma_sound_set_pitch(sound.get(), pitch);
        ma_sound_start(sound.get());
        s_Voices.push_back(std::move(sound));
    }

    void PlayMusic(const std::string& relativePath, bool loop, float volume) {
        if (!s_Ready) return;
        Release(s_Music);

        auto music = std::make_unique<ma_sound>();
        const std::string path = Assets::GetPath(relativePath);
        const ma_result result = ma_sound_init_from_file(&s_Engine, path.c_str(), MA_SOUND_FLAG_STREAM,
                                                         Group(SoundGroup::Music), nullptr, music.get());
        if (result != MA_SUCCESS) { LoadFailed(relativePath, result); return; }

        ma_sound_set_looping(music.get(), loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_volume(music.get(), volume);
        ma_sound_start(music.get());
        s_Music = std::move(music);
    }

    void StopMusic() { Release(s_Music); }

    bool IsMusicPlaying() { return s_Music && ma_sound_is_playing(s_Music.get()); }

    void SetMasterVolume(float volume) {
        if (s_Ready) ma_engine_set_volume(&s_Engine, std::max(0.0f, volume));
    }

    float GetMasterVolume() { return s_Ready ? ma_engine_get_volume(&s_Engine) : 0.0f; }

    void SetGroupVolume(SoundGroup group, float volume) {
        const int idx = group == SoundGroup::SFX ? 0 : 1;
        s_GroupVolume[idx] = std::max(0.0f, volume);
        if (s_Ready) ma_sound_group_set_volume(Group(group), s_GroupVolume[idx]);
    }

    float GetGroupVolume(SoundGroup group) { return s_GroupVolume[group == SoundGroup::SFX ? 0 : 1]; }

    int GetActiveVoiceCount() { return (int)s_Voices.size(); }
}
