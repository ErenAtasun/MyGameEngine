#include "Input.h"
#include <unordered_set>

static GLFWwindow* s_Window = nullptr;
static std::unordered_set<int> s_Down;
static std::unordered_set<int> s_Pressed;
static MouseState s_Mouse;

static void KeyCb(GLFWwindow*, int key, int sc, int action, int mods) {
    if (action == GLFW_PRESS) {
        s_Down.insert(key);
        s_Pressed.insert(key);
    }
    else if (action == GLFW_RELEASE) {
        s_Down.erase(key);
    }
}
static void CursorCb(GLFWwindow*, double x, double y) {
    s_Mouse.x = x; s_Mouse.y = y;
}
static void MouseBtnCb(GLFWwindow*, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) { s_Mouse.leftDown = true; s_Mouse.leftPressed = true; }
        else if (action == GLFW_RELEASE) { s_Mouse.leftDown = false; }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) { s_Mouse.rightDown = true; s_Mouse.rightPressed = true; }
        else if (action == GLFW_RELEASE) { s_Mouse.rightDown = false; }
    }
}

namespace Input {
    void Init(GLFWwindow* win) {
        s_Window = win;
        glfwSetKeyCallback(win, KeyCb);
        glfwSetCursorPosCallback(win, CursorCb);
        glfwSetMouseButtonCallback(win, MouseBtnCb);
    }
    void NewFrame() { s_Pressed.clear(); s_Mouse.newFrame(); }
    bool KeyDown(int key) { return s_Down.count(key) != 0; }
    bool KeyPressed(int key) { return s_Pressed.count(key) != 0; }
    const MouseState& Mouse() { return s_Mouse; }
}
