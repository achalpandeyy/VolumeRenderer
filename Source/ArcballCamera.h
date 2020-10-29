#ifndef ARCBALL_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

// TODO: Move this to math
#define PI_32 3.141592f

struct ArcballCamera
{
    ArcballCamera(const glm::vec3& from, const glm::vec3& to, float w, float h, float sensitivity = 0.5f, float fov = PI_32 / 4.f);

    void Rotate(const glm::vec2& prev_mouse, const glm::vec2& curr_mouse);
    void SetFOV(int value);

    glm::mat4 view;
    glm::mat4 projection;
    float fov;

private:
    glm::vec3 ScreenToArcballSpace(const glm::vec2& scr)
    {
        // Screen to NDC
        glm::vec3 p = glm::vec3(2.f * ((float)scr.x / (float)width) - 1.f, 1.f - 2.f * ((float)scr.y / (float)height), 0.f);

        float distance_sq = glm::dot(p, p);
        if (distance_sq <= 1.f)
        {
            return glm::vec3(p[0], p[1], glm::sqrt(1.f - distance_sq));
        }
        else
        {
            return glm::normalize(p);
        }
    }

    const glm::vec3 look_from;
    const glm::vec3 look_to;
    float sensitivity;
    float z_near = 0.1f;
    float z_far = 100.f;
    float width, height;

    // NOTE: You need to persist rotation matrix in arcball space otherwise you're losing the rotational information
    // from previous frames.
    // Basically, the cube needs to remember its previous coordinates, which can't happen because every frame we
    // start from the cube in model space with [0, 0, 0] to [1, 1, 1] and we transform from there.
    glm::mat4 rotation_arcball_space = glm::mat4(1.f);
};

#define ARCBALL_CAMERA_H
#endif
