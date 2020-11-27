#include "Volume.h"
#include "Core/Win32.h"

#include <fstream>
#include <sstream>

Volume::Volume(const char* path, const glm::ivec3& dims, const glm::vec3& sp)
    : dimensions(dims), spacing(sp)
{
    // Read in the volume data
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        // Note: Position of the last character == size of the file
        std::streampos pos = file.tellg();
        char* data = new char[pos];
        file.seekg(0, std::ios::beg);
        file.read(data, pos);
        file.close();

        // Upload to the GPU
        texture = std::make_unique<Texture3D>(dimensions.x, dimensions.y, dimensions.z, GL_R8, GL_RED, GL_UNSIGNED_BYTE, data);

        delete[] data;
    }
    else
    {
        std::ostringstream oss;
        oss << "Unable to open file at path: " << path << std::endl;
        OutputDebugStringA(oss.str().c_str());
    }
}