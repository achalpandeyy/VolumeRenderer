#ifndef TEXTURE_3D_H

#include <glad/glad.h>

struct Texture3D
{
    Texture3D(GLsizei width, GLsizei height, GLsizei depth, GLint internal_format, GLenum format, GLenum type, void* data);

    void inline Bind() const { glBindTexture(GL_TEXTURE_3D, id); }
    void inline Unbind() const { glBindTexture(GL_TEXTURE_3D, 0); }

    GLuint id;
};

#define TEXTURE_3D_H
#endif
