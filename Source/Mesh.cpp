#include "Mesh.h"

Mesh::Mesh(const std::vector<float>& vertices, unsigned int vertex_component_count, const std::vector<unsigned int>& indices)
{
    GLCall(glGenVertexArrays(1, &vao));
    BindVAO();

    GLuint vbo;
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

    GLsizei stride = vertex_component_count * sizeof(float);
    GLCall(glVertexAttribPointer(0, vertex_component_count, GL_FLOAT, GL_FALSE, stride, (const void*)0));
    GLCall(glEnableVertexAttribArray(0));

    GLuint ibo;
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW));

    UnbindVAO();
}