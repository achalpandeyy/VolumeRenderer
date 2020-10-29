#include "Util.h"
#include "Core/Win32.h"

#include <glad/glad.h>
#include <sstream>

std::vector<float> GetUnitCubeVertices()
{
    return std::vector<float>
    ({
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        0.f, 1.f, 1.f,
        1.f, 1.f, 1.f
    });
}

std::vector<unsigned int> GetUnitCubeIndices()
{
    return std::vector<unsigned int>({ 0, 1, 4, 5, 7, 1, 3, 0, 2, 4, 6, 7, 2, 3 });
}

std::vector<float> GetNDCQuadVertices()
{
    return std::vector<float>
    ({
       -1.f, -1.f,
        1.f, -1.f,
        1.f,  1.f,
       -1.f,  1.f
    });
}

std::vector<unsigned int> GetNDCQuadIndices()
{
    return std::vector<unsigned int>({ 0, 1, 2, 2, 3, 0 });
}

void GLClearErrors()
{
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
    GLenum error = glGetError();
    while (error != GL_NO_ERROR)
    {
        std::ostringstream oss;
        oss << "OpenGL Error: (" << error << "): " << function << std::endl
            << file << " " << line << std::endl;
        OutputDebugStringA(oss.str().c_str());
        return false;
    }
    return true;
}