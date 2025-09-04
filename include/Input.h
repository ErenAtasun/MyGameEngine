#pragma once
#include <GLFW/glfw3.h>

struct MouseState {
    double x = 0.0, y = 0.0;
    bool leftDown = false, rightDown = false;
    bool leftPressed = false, rightPressed = false; // bir frame'lik
    void newFrame() { leftPressed = rightPressed = false; }
};

namespace Input {
    void Init(GLFWwindow* win);
    void NewFrame(); // frame baþýnda çaðýr
    bool KeyDown(int key);       // GLFW_KEY_*
    bool KeyPressed(int key);    // bir frame'lik
    const MouseState& Mouse();
}
