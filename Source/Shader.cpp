#include "Shader.h"
#include "Util.h"
#include "Core/Win32.h"

#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const char* vertex_shader_path, const char* fragment_shader_path)
{
    GLuint vertex_shader = ReadShader(vertex_shader_path, ShaderType::VERTEX);
    GLuint fragment_shader = ReadShader(fragment_shader_path, ShaderType::FRAGMENT);

    GLCall(id = glCreateProgram());
    GLCall(glAttachShader(id, vertex_shader));
    GLCall(glAttachShader(id, fragment_shader));

    GLCall(glLinkProgram(id));

    CheckErrors(id, ShaderType::PROGRAM);

    GLCall(glDeleteShader(vertex_shader));
    GLCall(glDeleteShader(fragment_shader));
}

void Shader::SetUniform1i(const char* name, int v)
{
    GLCall(glUniform1i(GetUniformLocation(name), v));
}

void Shader::SetUniform3i(const char* name, int v0, int v1, int v2)
{
    GLCall(glUniform3i(GetUniformLocation(name), v0, v1, v2));
}

void Shader::SetUniform1f(const char* name, float val)
{
    GLCall(glUniform1f(GetUniformLocation(name), val));
}

void Shader::SetUniform3f(const char* name, float v0, float v1, float v2)
{
    GLCall(glUniform3f(GetUniformLocation(name), v0, v1, v2));
}

void Shader::SetUniformMatrix4fv(const char* name, float* v)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, false, v));
}

GLint Shader::GetUniformLocation(const char* name)
{
    if (uniform_location_cache.find(name) != uniform_location_cache.end())
        return uniform_location_cache[name];

    GLCall(GLint location = glGetUniformLocation(id, name));
    if (location == -1)
    {
        std::ostringstream oss;
        oss << "Warning: " << name << " uniform isn't found!" << std::endl;
        OutputDebugStringA(oss.str().c_str());
    }
    uniform_location_cache[name] = location;

    return location;
}

GLuint Shader::ReadShader(const char* shader_path, ShaderType type) const
{
    GLuint shader;
    std::ifstream in_file(shader_path);
    if (in_file.is_open())
    {
        std::stringstream shader_buffer;
        shader_buffer << in_file.rdbuf();
        const std::string& shader_code = shader_buffer.str();
        const char* shader_source = shader_code.c_str();

        GLCall(shader = glCreateShader(type == ShaderType::VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER));
        GLCall(glShaderSource(shader, 1, &shader_source, NULL));
        GLCall(glCompileShader(shader));

        CheckErrors(shader, type);
    }
    else
    {
        std::ostringstream oss;
        const char* shader_type = (type == ShaderType::VERTEX) ? "vertex" : "fragment";
        oss << "Unable to open " << shader_type << " shader file at path " << shader_path << std::endl;
        OutputDebugStringA(oss.str().c_str());
        exit(1);
    }
    in_file.close();

    return shader;
}

void Shader::CheckErrors(GLuint shader, ShaderType type) const
{
    GLint success;
    GLchar info_log[1024];

    switch (type)
    {
        case ShaderType::VERTEX:
        case ShaderType::FRAGMENT:
        {
            GLCall(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
            if (!success)
            {
                GLCall(glGetShaderInfoLog(shader, 1024, NULL, info_log));

                const char* shader_type = (type == ShaderType::VERTEX) ? "vertex" : "fragment";
                std::ostringstream oss;
                oss << "OpenGL Error: Failed to compile " << shader_type << " shader!" <<  std::endl << info_log;
                OutputDebugStringA(oss.str().c_str());
                exit(1);
            }
        } break;

        case ShaderType::PROGRAM:
        {
            GLCall(glGetProgramiv(id, GL_LINK_STATUS, &success));
            if (!success)
            {
                GLCall(glGetProgramInfoLog(id, 1024, NULL, info_log));
                std::ostringstream oss;
                oss << "OpenGL Error: Failed to link shader program!" << std::endl << info_log;
                OutputDebugStringA(oss.str().c_str());
                exit(1);
            }
        } break;
    }
}