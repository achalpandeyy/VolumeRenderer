#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const glm::vec3& center, const glm::vec3& pos, f32 sensitivity, f32 fov)
    : arcball_center(center), position(pos), sensitivity(sensitivity), fov(fov)
{
    glm::vec3 cam_z = glm::normalize(position - center);
    glm::vec3 cam_x = glm::normalize(glm::cross(cam_z, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 cam_y = glm::cross(cam_x, cam_z);

    arcball_rotation = glm::mat4(glm::vec4(cam_x, 0.f), glm::vec4(cam_y, 0.f), glm::vec4(cam_z, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
    view = GetArcballRotationToWorld();
}

void ArcballCamera::Rotate(const glm::vec3& prev, const glm::vec3& curr)
{
    f32 angle = glm::acos(glm::min(1.f, glm::dot(prev, curr)));
    glm::vec3 axis = glm::cross(prev, curr);

    Update(angle, axis);
}

void ArcballCamera::UpdateFOV(int value)
{
    f32 strength = 0.05f;
    fov -= strength * value;
    fov = glm::clamp(fov, PI_32 / 180.f, PI_32 / 2.f);
}

void ArcballCamera::Update(f32 angle, const glm::vec3& axis)
{
    arcball_rotation = glm::rotate(glm::mat4(1.f), sensitivity * angle, axis) * arcball_rotation;
    view = GetArcballRotationToWorld();

    glm::mat4 view_inverse = glm::inverse(view);

    position = glm::vec3(view_inverse[3][0], view_inverse[3][1], view_inverse[3][2]);
}