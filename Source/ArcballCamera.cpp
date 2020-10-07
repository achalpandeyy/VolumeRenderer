#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const glm::vec3& from, const glm::vec3& to, f32 w, f32 h, f32 sensitivity, f32 fov)
    : look_from(from), look_to(to), width(w), height(h), sensitivity(sensitivity), fov(fov)
{
    view = glm::lookAtRH(from, to, glm::vec3(0.f, 1.f, 0.f));
    projection = glm::perspective(fov, (f32)width / (f32)height, z_near, z_far);
}

void ArcballCamera::Rotate(const glm::vec2& prev_mouse, const glm::vec2& curr_mouse)
{
    glm::vec3 prev = ScreenToArcballSpace(prev_mouse);
    glm::vec3 curr= ScreenToArcballSpace(curr_mouse);

    f32 angle = glm::acos(glm::min(1.f, glm::dot(prev, curr)));
    glm::vec3 axis = glm::cross(prev, curr);

    rotation_arcball_space = glm::rotate(glm::mat4(1.f), 2.5f * angle, axis) * rotation_arcball_space;

    // NOTE: This would only work if the arcball is centered around origin, if it isn't you need to append a tranform
    // to the right, to make sure that first we transform to the arcball space.
    view = glm::lookAtRH(look_from, look_to, glm::vec3(0.f, 1.f, 0.f)) * rotation_arcball_space;

#if 1
    for (unsigned int y = 0; y < 4; ++y)
    {
        for (unsigned int x = 0; x < 4; ++x)
            assert(!glm::isnan(view[y][x]));
    }
#endif

    // I probably won't be needing this mechanism to figure out look_from member of the camera because we're no longer
    // shooting rays from that point but generating entry and exit point textures instead.
    // glm::mat4 view_inverse = glm::inverse(view);
    // look_from = glm::vec3(view_inverse[3][0], view_inverse[3][1], view_inverse[3][2]);
}

void ArcballCamera::SetFOV(int value)
{
    f32 strength = 0.05f;
    fov -= strength * value;
    fov = glm::clamp(fov, PI_32 / 180.f, PI_32 / 2.f);

    projection = glm::perspective(fov, (f32)width / (f32)height, z_near, z_far);
}