#ifndef UTIL_H

#include <vector>

std::vector<float> GetUnitCubeVertices();
std::vector<unsigned int> GetUnitCubeIndices();
std::vector<float> GetNDCQuadVertices();
std::vector<unsigned int> GetNDCQuadIndices();

//
// OpenGL debugging stuff.
//

#define Assert(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearErrors();\
x;\
Assert(GLLogCall(#x, __FILE__, __LINE__))

void GLClearErrors();
bool GLLogCall(const char* function, const char* file, int line);

#define UTIL_H
#endif
