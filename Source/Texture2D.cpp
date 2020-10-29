#include "Texture2D.h"

Texture2D::Texture2D(GLsizei width, GLsizei height, GLint internal_format, GLenum format, GLenum type)
{
    GLCall(glGenTextures(1, &id));
    Bind();

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // NOTE: We can assume that these textures doesn't have any row alignment, since we ourselves are creating them
    GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, nullptr));
}