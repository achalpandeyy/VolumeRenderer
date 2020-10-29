#ifndef TEXTURE_2D_H

#include "Util.h"

#include <glad/glad.h>

// TODO: Is is required to free up resources in the destructor?
struct Texture2D
{
    Texture2D(GLsizei width, GLsizei height, GLint internal_format, GLenum format, GLenum type);

    void inline Bind() const { GLCall(glBindTexture(GL_TEXTURE_2D, id)); }
    void inline Unbind() const { GLCall(glBindTexture(GL_TEXTURE_2D, 0)); }

    GLuint id;
};

#define TEXTURE_2D_H
#endif
