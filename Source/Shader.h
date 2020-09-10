#ifndef SHADER_H

#include "Core/OpenGL.h"

struct Shader
{
    Shader(const char* vertex_shader_path, const char* fragment_shader_path);

    inline void Use() const
    {
        GLCall(glUseProgram(id));
    }

// private:
    enum class ShaderType
    {
        VERTEX,
        FRAGMENT,
        PROGRAM
    };

    GLuint ReadShader(const char* shader_path, ShaderType type) const;
    void CheckErrors(GLuint shader, ShaderType type) const;

    GLuint id;
};

#define SHADER_H
#endif
