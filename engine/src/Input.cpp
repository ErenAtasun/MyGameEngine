#include <MME/Input.h>
#include "InputInternal.h"

#include <GLFW/glfw3.h>
#include <array>

// Motor tuş kodlarının GLFW ile aynı olduğunu derleme zamanında doğrula
static_assert(mme::Key::Space == GLFW_KEY_SPACE);
static_assert(mme::Key::A == GLFW_KEY_A && mme::Key::Z == GLFW_KEY_Z);
static_assert(mme::Key::Escape == GLFW_KEY_ESCAPE && mme::Key::Enter == GLFW_KEY_ENTER);
static_assert(mme::Key::Up == GLFW_KEY_UP && mme::Key::Down == GLFW_KEY_DOWN);
static_assert(mme::Key::Left == GLFW_KEY_LEFT && mme::Key::Right == GLFW_KEY_RIGHT);
static_assert(mme::Key::F1 == GLFW_KEY_F1 && mme::Key::F12 == GLFW_KEY_F12);
static_assert(mme::Key::KP0 == GLFW_KEY_KP_0 && mme::Key::KP9 == GLFW_KEY_KP_9);
static_assert(mme::Key::LeftShift == GLFW_KEY_LEFT_SHIFT && mme::Key::RightAlt == GLFW_KEY_RIGHT_ALT);
static_assert(mme::Key::Last == GLFW_KEY_LAST);
static_assert(mme::Mouse::Left == GLFW_MOUSE_BUTTON_LEFT && mme::Mouse::Right == GLFW_MOUSE_BUTTON_RIGHT);
static_assert(mme::Mouse::Last == GLFW_MOUSE_BUTTON_LAST);

namespace mme::Input {

    namespace {
        struct ButtonState { bool down = false, pressed = false, released = false; };

        std::array<ButtonState, Key::Last + 1> s_Keys{};
        std::array<ButtonState, Mouse::Last + 1> s_Mouse{};
        glm::vec2 s_MousePos{ 0.0f };
        float s_Scroll = 0.0f;

        template<size_t N>
        ButtonState* Find(std::array<ButtonState, N>& arr, int code) {
            return (code >= 0 && code < (int)N) ? &arr[(size_t)code] : nullptr;
        }

        void Press(ButtonState* s) {
            if (!s) return;
            if (!s->down) s->pressed = true;
            s->down = true;
        }
        void Release(ButtonState* s) {
            if (!s) return;
            if (s->down) s->released = true;
            s->down = false;
        }
    }

    bool KeyDown(KeyCode key)     { auto* s = Find(s_Keys, key); return s && s->down; }
    bool KeyPressed(KeyCode key)  { auto* s = Find(s_Keys, key); return s && s->pressed; }
    bool KeyReleased(KeyCode key) { auto* s = Find(s_Keys, key); return s && s->released; }

    bool MouseDown(MouseButton b)     { auto* s = Find(s_Mouse, b); return s && s->down; }
    bool MousePressed(MouseButton b)  { auto* s = Find(s_Mouse, b); return s && s->pressed; }
    bool MouseReleased(MouseButton b) { auto* s = Find(s_Mouse, b); return s && s->released; }

    glm::vec2 MousePosition() { return s_MousePos; }
    float ScrollDelta() { return s_Scroll; }

    namespace Internal {
        void BeginFrame() {
            for (auto& k : s_Keys)  k.pressed = k.released = false;
            for (auto& m : s_Mouse) m.pressed = m.released = false;
            s_Scroll = 0.0f;
        }

        void ProcessEvent(const Event& e) {
            switch (e.type) {
                case EventType::KeyPressed:          Press(Find(s_Keys, e.key)); break;
                case EventType::KeyReleased:         Release(Find(s_Keys, e.key)); break;
                case EventType::MouseButtonPressed:  Press(Find(s_Mouse, e.button)); break;
                case EventType::MouseButtonReleased: Release(Find(s_Mouse, e.button)); break;
                case EventType::MouseMoved:          s_MousePos = { e.x, e.y }; break;
                case EventType::MouseScrolled:       s_Scroll += e.y; break;
                default: break;
            }
        }
    }
}
