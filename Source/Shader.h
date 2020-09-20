#ifndef SHADER_H

#include "Core/OpenGL.h"

#include <string>
#include <unordered_map>

struct Shader
{
    Shader(const char* vertex_shader_path, const char* fragment_shader_path);

    void SetUniform1i(const char* name, int v);
    void SetUniform3i(const char* name, int v0, int v1, int v2);
    void SetUniform3f(const char* name, f32 v0, f32 v1, f32 v2);
    void SetUniformMatrix4fv(const char* name, f32* v);

    inline void Use() const
    {
        GLCall(glUseProgram(id));
    }

private:
    enum class ShaderType
    {
        VERTEX,
        FRAGMENT,
        PROGRAM
    };

    GLint GetUniformLocation(const char* name);
    GLuint ReadShader(const char* shader_path, ShaderType type) const;
    void CheckErrors(GLuint shader, ShaderType type) const;

    GLuint id;
    std::unordered_map<std::string, GLint> uniform_location_cache;
};

#define SHADER_H
#endif
