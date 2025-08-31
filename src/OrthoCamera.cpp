#include "OrthoCamera.h"
#include <glm/gtc/matrix_transform.hpp>

OrthoCamera::OrthoCamera(float l, float r, float b, float t) {
    SetProjection(l, r, b, t);
    RecalcView();
}

void OrthoCamera::SetProjection(float l, float r, float b, float t) {
    m_Proj = glm::ortho(l, r, b, t);
}

void OrthoCamera::RecalcView() {
    // ölçek ? dönüþ ? pozisyon (ekran piksel uzayý: +x sað, +y yukarý)
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(-m_Position, 0.0f));
    transform = glm::rotate(transform, glm::radians(-m_Rotation), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, glm::vec3(1.0f / m_Zoom, 1.0f / m_Zoom, 1.0f));
    m_View = transform;
}
