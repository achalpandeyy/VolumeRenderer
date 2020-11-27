#include "Mesh.h"

Mesh::Mesh(const std::vector<float>& vertices, unsigned int vertex_component_count, const std::vector<unsigned int>& indices)
{
    glGenVertexArrays(1, &vao);
    BindVAO();

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    GLsizei stride = vertex_component_count * sizeof(float);
    glVertexAttribPointer(0, vertex_component_count, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    glEnableVertexAttribArray(0);

    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    UnbindVAO();
}