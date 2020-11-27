#ifndef MESH_H

#include <glad/glad.h>
#include <vector>

struct Mesh
{
    Mesh(const std::vector<float>& vertices, unsigned int vertex_component_count, const std::vector<unsigned int>& indices);

    inline void BindVAO() const { glBindVertexArray(vao); }
    inline void UnbindVAO() const { glBindVertexArray(0); }

private:
    GLuint vao;
};

#define MESH_H
#endif
