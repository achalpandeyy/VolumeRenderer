#ifndef ARCBALL_CAMERA_H

#include "Core/Types.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// TODO: Move this to math
#define PI_32 3.141592f

struct ArcballCamera
{
    ArcballCamera(const glm::vec3& center, const glm::vec3& pos, f32 sensitivity = 2.5f, f32 fov = PI_32 / 4.f);

    void Rotate(const glm::vec3& prev, const glm::vec3& curr);
    void UpdateFOV(int value);

    glm::vec3 position;
    f32 sensitivity;
    glm::mat4 view;
    f32 fov;

private:
    void Update(f32 angle, const glm::vec3& axis);

    inline glm::mat4 GetArcballRotationToWorld()
    {
        glm::mat4 center_translation = glm::translate(glm::mat4(1.f), -arcball_center);

        f32 view_distance = glm::distance(arcball_center, position);
        glm::mat4 position_translation = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -view_distance));

        return position_translation * arcball_rotation * center_translation;
    }

    const glm::vec3 arcball_center;
    glm::mat4 arcball_rotation;
};

#define ARCBALL_CAMERA_H
#endif
