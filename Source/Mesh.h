#ifndef MESH_H

#include "Util.h"

#include <glad/glad.h>

struct Mesh
{
    Mesh(const std::vector<float>& vertices, unsigned int vertex_component_count, const std::vector<unsigned int>& indices);

    inline void BindVAO() const { GLCall(glBindVertexArray(vao)); }
    inline void UnbindVAO() const { GLCall(glBindVertexArray(0)); }

private:
    GLuint vao;
};

#define MESH_H
#endif
