#ifndef VOLUME_H

#include "Texture3D.h"

#include <glm/glm.hpp>
#include <memory>

// Note: I think there is no point in separating reading and uploading of volume, since, in almost
// all cases we don't want to keep the volume around in CPU memory. Maybe this would change in the
// future if I decide to implement texture streaming (virtual texturing) to deal with huge volumes.
struct Volume
{
    Volume(const char* path, const glm::ivec3& dims, const glm::vec3& sp);

    const glm::ivec3 dimensions;
    const glm::vec3 spacing;
    std::unique_ptr<Texture3D> texture = nullptr;
};

#define VOLUME_H
#endif
