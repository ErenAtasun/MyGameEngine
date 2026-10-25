#include <MME/SpriteAnimator.h>
#include <MME/Entity.h>
#include <MME/Log.h>
#include <MME/Renderer2D.h>
#include <MME/SpriteComponent.h>

#include <algorithm>

namespace mme {

    void SpriteAnimator::OnCreate() {
        ApplyFrame();
    }

    void SpriteAnimator::AddClip(const std::string& name, int first, int count, float clipFps, bool clipLoop) {
        m_Clips[name] = Clip{ first, std::max(count, 1), clipFps, clipLoop };
    }

    void SpriteAnimator::Play(const std::string& clipName, bool restart) {
        auto it = m_Clips.find(clipName);
        if (it == m_Clips.end()) {
            MME_WARN("[SpriteAnimator] '", GetEntity().GetName(), "' icin klip yok: ", clipName);
            return;
        }
        if (!restart && playing && m_ClipName == clipName) return;

        m_ClipName = clipName;
        firstFrame = it->second.firstFrame;
        frameCount = it->second.frameCount;
        fps = it->second.fps;
        loop = it->second.loop;
        playing = true;
        m_Frame = 0;
        m_Time = 0.0f;
        ApplyFrame();
    }

    void SpriteAnimator::SetFrame(int frameInClip) {
        m_Frame = std::clamp(frameInClip, 0, std::max(frameCount - 1, 0));
        m_Time = 0.0f;
        ApplyFrame();
    }

    void SpriteAnimator::OnUpdate(float dt) {
        if (!playing || fps <= 0.0f || frameCount <= 0) { ApplyFrame(); return; }

        m_Time += dt * speed;
        const float frameTime = 1.0f / fps;
        while (m_Time >= frameTime) {
            m_Time -= frameTime;
            if (m_Frame + 1 < frameCount) {
                ++m_Frame;
            } else if (loop) {
                m_Frame = 0;
            } else {
                playing = false;
                m_Time = 0.0f;
                if (onFinished) onFinished();
                break;
            }
        }
        ApplyFrame();
    }

    void SpriteAnimator::ApplyFrame() {
        auto* sprite = GetEntity().GetComponent<SpriteComponent>();
        if (!sprite) return;
        m_Frame = std::clamp(m_Frame, 0, std::max(frameCount - 1, 0));
        GridCellUV(columns, rows, firstFrame + m_Frame, sprite->uvMin, sprite->uvMax);
    }
}
