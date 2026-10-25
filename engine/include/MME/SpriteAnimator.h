#pragma once
#include <MME/Component.h>

#include <functional>
#include <string>
#include <unordered_map>

namespace mme {

    // Sprite sheet animasyonu: aynı entity'deki SpriteComponent'in UV'lerini
    // her frame günceller. Kare indeksleri soldan sağa, yukarıdan aşağı sayılır.
    //
    //   auto& anim = e.AddComponent<SpriteAnimator>();
    //   anim.columns = 8; anim.rows = 1;
    //   anim.AddClip("spin", 0, 8, 12.0f);
    //   anim.Play("spin");
    class SpriteAnimator : public Component {
    public:
        struct Clip {
            int firstFrame = 0;
            int frameCount = 1;
            float fps = 10.0f;
            bool loop = true;
        };

        void OnCreate() override;
        void OnUpdate(float dt) override;

        void AddClip(const std::string& name, int firstFrame, int frameCount, float fps, bool loop = true);
        // Klibi başlatır; zaten oynuyorsa restart = false iken baştan almaz
        void Play(const std::string& clipName, bool restart = false);
        void Stop() { playing = false; }
        void SetFrame(int frameInClip);

        const std::string& GetClipName() const { return m_ClipName; }
        int GetFrameInClip() const { return m_Frame; }
        int GetSheetFrame() const { return firstFrame + m_Frame; }

        // Sheet düzeni
        int columns = 1;
        int rows = 1;

        // Geçerli klip (serileştirilir; isimli klipler koddan eklenir)
        int firstFrame = 0;
        int frameCount = 1;
        float fps = 10.0f;
        bool loop = true;
        bool playing = true;
        float speed = 1.0f;   // oynatma hızı çarpanı

        std::function<void()> onFinished; // döngüsüz klip bitince

    private:
        void ApplyFrame();

        std::unordered_map<std::string, Clip> m_Clips;
        std::string m_ClipName;
        int m_Frame = 0;
        float m_Time = 0.0f;
    };
}
